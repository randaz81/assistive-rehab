/******************************************************************************
 *                                                                            *
 * Copyright (C) 2018 Fondazione Istituto Italiano di Tecnologia (IIT)        *
 * All Rights Reserved.                                                       *
 *                                                                            *
 ******************************************************************************/

/**
 * @file main.cpp
 * @authors: Valentina Vasco <valentina.vasco@iit.it>
 */

#include <yarp/os/RFModule.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/os/Network.h>
#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/RpcClient.h>

#include <mutex>
#include <cmath>

#include <AssistiveRehab/skeleton.h>

#include "src/managerTUG_IDL.h"

using namespace std;
using namespace yarp::os;
using namespace yarp::sig;
using namespace assistive_rehab;

class SpeechInterpApi : public BufferedPort<Bottle>
{
    string module_name;
    BufferedPort<Bottle> speechPortOut;
    unordered_map<string,string> speak_map;
    double t;
    string answer;

public:

    /********************************************************/
    SpeechInterpApi(const string &module_name, const unordered_map<string,string> &speak_map)
    {
        this->module_name=module_name;
        this->speak_map=speak_map;
    }

    /********************************************************/
    ~SpeechInterpApi()
    {
    };

    /********************************************************/
    bool open()
    {
        this->useCallback();
        BufferedPort<yarp::os::Bottle >::open("/"+module_name+"/speech-interp:i");
        speechPortOut.open("/"+module_name+"/speech-interp:o");
        return true;
    }

    /********************************************************/
    void close()
    {
        BufferedPort<yarp::os::Bottle >::close();
        speechPortOut.close();
    }

    /********************************************************/
    void interrupt()
    {
        BufferedPort<yarp::os::Bottle >::interrupt();
        speechPortOut.interrupt();
    }

    /********************************************************/
    void setTime(const double &t)
    {
        this->t=t;
    }

    /********************************************************/
    string getAnswer() const
    {
        return answer;
    }

    /********************************************************/
    void onRead( yarp::os::Bottle &speech_interpretation )
    {
        if(!speech_interpretation.isNull())
        {
            string keyword=speech_interpretation.get(0).asString();
            answer.clear();
            if(keyword=="speed" || keyword=="aid" || keyword=="repetition"
                    || keyword=="unclear" || keyword=="not-known")
            {
                answer=speak_map[keyword];
            }
            else if(keyword=="feedback")
            {
                if(t<=10.0)
                {
                    answer=speak_map[keyword+"-high"];
                }
                else if(t<=20.0)
                {
                    answer=speak_map[keyword+"-medium"];
                }
                else if(t<=30.0)
                {
                    answer=speak_map[keyword+"-low"];
                }
            }

            Bottle &output=speechPortOut.prepare();
            output.clear();
            output.addString(answer);
            speechPortOut.writeStrict();
        }
    }

};

/****************************************************************/
class SpeechParam
{
    ostringstream ss;
public:
    SpeechParam(const int d) { ss<<d; }
    SpeechParam(const double g) { ss<<g; }
    SpeechParam(const string &s) { ss<<s; }
    string get() const { return ss.str(); }
};

/****************************************************************/
class Manager : public RFModule, public managerTUG_IDL
{
    //params
    string module_name;
    string speak_file;
    double period;
    double finish_line_thresh,standing_thresh;

    const int ok=Vocab::encode("ok");
    const int fail=Vocab::encode("fail");
    enum class State { stopped, idle, seek_line, seek_skeleton, follow, engaged, assess_standing, assess_crossing, line_crossed, not_passed, finished } state;
    string tag;
    double t0,tstart,t;
    int encourage_cnt;
    unordered_map<string,string> speak_map;
    bool interrupting;
    mutex mtx;

    //ports
    RpcClient analyzerPort;
    RpcClient speechRpcPort;
    RpcClient attentionPort;
    BufferedPort<Bottle> speechStreamPort;
    RpcServer cmdPort;

    SpeechInterpApi *speech_interp;

    /****************************************************************/
    bool attach(RpcServer &source) override
    {
        return yarp().attachAsServer(source);
    }

    /****************************************************************/
    bool load_speak(const string &context, const string &speak_file)
    {
        ResourceFinder rf_speak;
        rf_speak.setDefaultContext(context);
        rf_speak.setDefaultConfigFile(speak_file.c_str());
        rf_speak.configure(0,nullptr);

        Bottle &bGroup=rf_speak.findGroup("general");
        if (bGroup.isNull())
        {
            yError()<<"Unable to find group \"general\"";
            return false;
        }
        if (!bGroup.check("num-sections"))
        {
            yError()<<"Unable to find key \"num-sections\"";
            return false;
        }
        int num_sections=bGroup.find("num-sections").asInt();
        for (int i=0; i<num_sections; i++)
        {
            ostringstream section;
            section<<"section-"<<i;
            Bottle &bSection=rf_speak.findGroup(section.str());
            if (bSection.isNull())
            {
                string msg="Unable to find section";
                msg+="\""+section.str()+"\"";
                yError()<<msg;
                return false;
            }
            if (!bSection.check("key") || !bSection.check("value"))
            {
                yError()<<"Unable to find key \"key\" and/or \"value\"";
                return false;
            }
            string key=bSection.find("key").asString();
            string value=bSection.find("value").asString();
            speak_map[key]=value;
        }

        return true;
    }

    /****************************************************************/
    bool speak(const string &key, const bool wait,
               const vector<SpeechParam> &p=vector<SpeechParam>())
    {
        auto it=speak_map.find(key);
        string value=(it!=end(speak_map)?it->second:speak_map["ouch"]);

        string value_ext;
        if (!p.empty())
        {
            for (size_t i=0;;)
            {
                size_t pos=value.find("%");
                value_ext+=value.substr(0,pos);
                if (i<p.size())
                {
                    value_ext+=p[i++].get();
                }
                if (pos==string::npos)
                {
                    break;
                }
                value.erase(0,pos+1);
            }
        }
        else
        {
            value_ext=value;
        }

        Bottle &payload=speechStreamPort.prepare();
        payload.clear();
        payload.addString(value_ext);
        speechStreamPort.writeStrict();

        while (wait && !interrupting && (speechRpcPort.getOutputCount()>0))
        {
            Time::delay(getPeriod());
            Bottle cmd,rep;
            cmd.addVocab(Vocab::encode("stat"));
            if (speechRpcPort.write(cmd,rep))
            {
                if (rep.get(0).asString()=="quiet")
                {
                    break;
                }
            }
        }
        return (it!=end(speak_map));
    }

    /****************************************************************/
    bool disengage()
    {
        bool ret=false;
        state=State::idle;

        Bottle cmd,rep;
        cmd.addString("stop");
        if (attentionPort.write(cmd,rep))
        {
            if (rep.get(0).asVocab()==ok)
            {
                cmd.clear();
                cmd.addString("set_auto");
                cmd.addInt(1);
                if (attentionPort.write(cmd,rep))
                {
                    if (rep.get(0).asVocab()==ok)
                    {
                        ret=true;
                    }
                }
            }
        }
        t0=Time::now();

        return ret;
    }

    /****************************************************************/
    bool start() override
    {
        lock_guard<mutex> lg(mtx);
        return disengage();
    }

    /****************************************************************/
    bool stop() override
    {
        lock_guard<mutex> lg(mtx);
        bool ret=false;

        Bottle cmd,rep;
        cmd.addString("stop");
        if (attentionPort.write(cmd,rep))
        {
            if (rep.get(0).asVocab()==ok)
            {
                ret=true;
            }
        }
        state=State::stopped;
        return ret;
    }

    /****************************************************************/
    bool configure(ResourceFinder &rf) override
    {
        module_name=rf.check("name",Value("managerTUG")).asString();
        period=rf.check("period",Value(0.1)).asDouble();
        speak_file=rf.check("speak-file",Value("speak-it")).asString();
        finish_line_thresh=rf.check("finish-line-thresh",Value(0.2)).asDouble();
        standing_thresh=rf.check("standing-thresh",Value(0.3)).asDouble();

        this->setName(module_name.c_str());

        if (!load_speak(rf.getContext(),speak_file))
        {
            string msg="Unable to locate file";
            msg+="\""+speak_file+"\"";
            yError()<<msg;
            return false;
        }

        analyzerPort.open("/"+module_name+"/analyzer:rpc");
        speechRpcPort.open("/"+module_name+"/speech:rpc");
        attentionPort.open("/"+module_name+"/attention:rpc");
        speechStreamPort.open("/"+module_name+"/speech:o");
        cmdPort.open("/"+module_name+"/cmd:rpc");
        attach(cmdPort);

        speech_interp=new SpeechInterpApi(module_name,speak_map);
        speech_interp->open();

        state=State::idle;
        interrupting=false;
        t0=tstart=Time::now();

        return true;
    }

    /****************************************************************/
    double getPeriod() override
    {
        return period;
    }

    /****************************************************************/
    bool updateModule() override
    {
        lock_guard<mutex> lg(mtx);
        if((analyzerPort.getOutputCount()==0) || (speechStreamPort.getOutputCount()==0) ||
                (speechRpcPort.getOutputCount()==0) || (attentionPort.getOutputCount()==0))
        {
            yInfo()<<"not connected";
            return true;
        }

        string follow_tag("");
        {
            Bottle cmd,rep;
            cmd.addString("is_following");
            if (attentionPort.write(cmd,rep))
            {
                follow_tag=rep.get(0).asString();
            }
        }

        if (state==State::idle)
        {
            if (Time::now()-t0>10.0)
            {
                state=State::seek_line;
            }
        }

        if (state>=State::follow)
        {
            if (follow_tag!=tag)
            {
                Bottle cmd,rep;
                cmd.addString("stop");
                analyzerPort.write(cmd,rep);
                speak("disengaged",true);
                disengage();
                return true;
            }
        }

        if (state==State::seek_line)
        {
            //search for finish line
            Bottle cmd,rep;
            cmd.addString("get_line_pose");
            if(attentionPort.write(cmd,rep))
            {
                if(Bottle* line_pose_bottle=rep.get(0).asList())
                {
                    if(line_pose_bottle->size()>=7)
                    {
                        yInfo()<<"Found finish line at"<<line_pose_bottle->toString();
                        Vector line_pose(7);
                        line_pose[0]=line_pose_bottle->get(0).asDouble();
                        line_pose[1]=line_pose_bottle->get(1).asDouble();
                        line_pose[2]=line_pose_bottle->get(2).asDouble();
                        line_pose[3]=line_pose_bottle->get(3).asDouble();
                        line_pose[4]=line_pose_bottle->get(4).asDouble();
                        line_pose[5]=line_pose_bottle->get(5).asDouble();
                        line_pose[6]=line_pose_bottle->get(6).asDouble();
                        cmd.clear();
                        cmd.addString("setLinePose");
                        cmd.addList().read(line_pose);
                        rep.clear();
                        if(analyzerPort.write(cmd,rep))
                        {
                            state=State::seek_skeleton;
                        }
                    }
                }
            }
        }

        if (state==State::seek_skeleton)
        {
            if (!follow_tag.empty())
            {
                tag=follow_tag;
                Bottle cmd,rep;
                cmd.addString("look");
                cmd.addString(tag);
                cmd.addString(KeyPointTag::shoulder_center);
                if (attentionPort.write(cmd,rep))
                {
                    yInfo()<<"Following"<<follow_tag;
                    vector<SpeechParam> p;
                    p.push_back(SpeechParam(tag[0]!='#'?tag:string("")));
                    speak("invite-start",true,p);
                    speak("engage",true);
                    state=State::follow;
                    encourage_cnt=0;
                }
            }
        }

        if (state==State::follow)
        {
            Bottle cmd,rep;
            cmd.addString("is_with_raised_hand");
            cmd.addString(tag);
            if (attentionPort.write(cmd,rep))
            {
                if (rep.get(0).asVocab()==ok)
                {
                    speak("accepted",true);
                    state=State::engaged;
                }
            }
        }

        if (state==State::engaged)
        {
            Bottle cmd,rep;
            cmd.addString("loadExercise");
            cmd.addString("tug");
            if (analyzerPort.write(cmd,rep))
            {
                bool ack=rep.get(0).asBool();
                if (ack)
                {
                    cmd.clear();
                    rep.clear();
                    cmd.addString("listMetrics");
                    if (analyzerPort.write(cmd,rep))
                    {
                        Bottle &metrics=*rep.get(0).asList();
                        if (metrics.size()>0)
                        {
                            string metric="step_0";//select_randomly(metrics);
                            yInfo()<<"Selected metric:"<<metric;

                            cmd.clear();
                            rep.clear();
                            cmd.addString("selectMetric");
                            cmd.addString(metric);
                            if (analyzerPort.write(cmd,rep))
                            {
                                ack=rep.get(0).asBool();
                                if(ack)
                                {
                                    cmd.clear();
                                    rep.clear();
                                    cmd.addString("listMetricProps");
                                    if (analyzerPort.write(cmd,rep))
                                    {
                                        Bottle &props=*rep.get(0).asList();
                                        if (props.size()>0)
                                        {
                                            string prop="step_length";//select_randomly(props);
                                            yInfo()<<"Selected prop:"<<prop;

                                            cmd.clear();
                                            rep.clear();
                                            cmd.addString("selectMetricProp");
                                            cmd.addString(prop);
                                            if (analyzerPort.write(cmd,rep))
                                            {
                                                ack=rep.get(0).asBool();
                                                if(ack)
                                                {
                                                    cmd.clear();
                                                    rep.clear();
                                                    cmd.addString("selectSkel");
                                                    cmd.addString(tag);
                                                    yInfo()<<"Selecting skeleton"<<tag;
                                                    if (analyzerPort.write(cmd,rep))
                                                    {
                                                        if (rep.get(0).asVocab()==ok)
                                                        {
                                                            speak("sit",true);
                                                            cmd.clear();
                                                            rep.clear();
                                                            cmd.addString("look");
                                                            cmd.addString(tag);
                                                            cmd.addString(KeyPointTag::hip_center);
                                                            if (attentionPort.write(cmd,rep))
                                                            {
                                                                speak("explain",true);
                                                                speak("questions",true);
                                                                double t1=Time::now();
                                                                while(speech_interp->getAnswer().empty())
                                                                {
                                                                    double timeout=Time::now()-t1;
                                                                    if(timeout>10.0)
                                                                        break;
                                                                }
                                                                speak("start",true);
                                                                cmd.clear();
                                                                cmd.addString("start");
                                                                cmd.addInt(1);
                                                                if (analyzerPort.write(cmd,rep))
                                                                {
                                                                    if (rep.get(0).asVocab()==ok)
                                                                    {
                                                                        state=State::assess_standing;
                                                                        t0=tstart=Time::now();
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        speech_interp->setTime(Time::now()-tstart);
        if (state==State::assess_standing)
        {
            //check if the person stands up
            Bottle cmd,rep;
            cmd.addString("isStanding");
            cmd.addDouble(standing_thresh);
            if (analyzerPort.write(cmd,rep))
            {
                if(rep.get(0).asVocab()==ok)
                {
                    yInfo()<<"Person standing";
                    state=State::assess_crossing;
                    encourage_cnt=0;
                    t0=Time::now();
                }
                else
                {
                    if((Time::now()-t0)>10.0)
                    {
                        if(++encourage_cnt<=1)
                        {
                            speak("encourage",false);
                            t0=Time::now();
                        }
                        else
                        {
                            state=State::not_passed;
                        }
                    }
                }
            }
        }

        if (state==State::assess_crossing)
        {
            Bottle cmd,rep;
            cmd.addString("hasCrossedFinishLine");
            cmd.addDouble(finish_line_thresh);
            if (analyzerPort.write(cmd,rep))
            {
                if(rep.get(0).asVocab()==ok)
                {
                    yInfo()<<"Line crossed!";
                    state=State::line_crossed;
                    encourage_cnt=0;
                    t0=Time::now();
                }
                else
                {
                    cmd.clear();
                    rep.clear();
                    cmd.addString("isSitting");
                    cmd.addDouble(standing_thresh);
                    if (analyzerPort.write(cmd,rep))
                    {
                        if(rep.get(0).asVocab()==ok)
                        {
                            yInfo()<<"Test finished but line not crossed";
                            speak("not-crossed",true);
                            state=State::not_passed;
                        }
                        else
                        {
                            if((Time::now()-t0)>30.0)
                            {
                                if(++encourage_cnt<=1)
                                {
                                    speak("encourage",false);
                                    t0=Time::now();
                                }
                                else
                                {
                                    state=State::not_passed;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (state==State::line_crossed)
        {
            //detect when the person seats down
            Bottle cmd,rep;
            cmd.addString("isSitting");
            cmd.addDouble(standing_thresh);
            if (analyzerPort.write(cmd,rep))
            {
                if(rep.get(0).asVocab()==ok)
                {
                    t=Time::now()-tstart;
                    yInfo()<<"Test finished in"<<t<<"seconds";
                    state=State::finished;
                }
                else
                {
                    if((Time::now()-t0)>30.0)
                    {
                        if(++encourage_cnt<=1)
                        {
                            speak("encourage",false);
                            t0=Time::now();
                        }
                        else
                        {
                            state=State::not_passed;
                        }
                    }
                }
            }
        }

        if (state==State::finished)
        {
            vector<SpeechParam> p;
            p.push_back(round(t*10.0)/10.0);
            Bottle cmd,rep;
            cmd.addString("stop");
            analyzerPort.write(cmd,rep);
            speak("assess-high",true,p);
            speak("greetings",true);
            disengage();
        }

        if (state==State::not_passed)
        {
            Bottle cmd,rep;
            cmd.addString("stop");
            analyzerPort.write(cmd,rep);
            speak("end",true);
            speak("assess-low",true);
            speak("greetings",true);
            disengage();
        }

        return true;
    }

    /****************************************************************/
    bool interruptModule() override
    {
        interrupting=true;
        return true;
    }

    /****************************************************************/
    bool close() override
    {
        speech_interp->interrupt();
        speech_interp->close();
        delete speech_interp;
        analyzerPort.close();
        speechRpcPort.close();
        attentionPort.close();
        speechStreamPort.close();
        cmdPort.close();
        return true;
    }
};


/****************************************************************/
int main(int argc, char *argv[])
{
    Network yarp;
    if (!yarp.checkNetwork())
    {
        yError()<<"Unable to find Yarp server!";
        return EXIT_FAILURE;
    }

    ResourceFinder rf;
    rf.setDefaultContext("managerTUG");
    rf.setDefaultConfigFile("config-it.ini");
    rf.configure(argc,argv);

    Manager manager;
    return manager.runModule(rf);
}

