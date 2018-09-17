/******************************************************************************
 *                                                                            *
 * Copyright (C) 2018 Fondazione Istituto Italiano di Tecnologia (IIT)        *
 * All Rights Reserved.                                                       *
 *                                                                            *
 ******************************************************************************/

/**
 * @file Manager.cpp
 * @authors: Valentina Vasco <valentina.vasco@iit.it>
 */

#include <iostream>
#include <vector>
#include <list>

#include <yarp/os/all.h>
#include <yarp/sig/Vector.h>
#include <yarp/sig/Matrix.h>

#include "Manager.h"
#include "Metric.h"
#include "Processor.h"

#include "src/motionAnalyzer_IDL.h"

#include "matio.h"

//#define LOW 0.0
//#define MEDIUM 0.5
//#define HIGH 1.0

using namespace std;
using namespace yarp::os;
using namespace yarp::sig;
using namespace assistive_rehab;

void Manager::init()
{
    numKeypoints = 15;

    //tag2key initial configuration
    elbowLeft_init.resize(3);
    elbowRight_init.resize(3);
    handLeft_init.resize(3);
    handRight_init.resize(3);
    head_init.resize(3);
    shoulderCenter_init.resize(3);
    shoulderLeft_init.resize(3);
    shoulderRight_init.resize(3);
    hipLeft_init.resize(3);
    hipRight_init.resize(3);
    kneeLeft_init.resize(3);
    kneeRight_init.resize(3);
    ankleLeft_init.resize(3);
    ankleRight_init.resize(3);
    hipCenter_init.resize(3);

    initial_keypoints.resize(numKeypoints);
    new_keypoints.resize(numKeypoints);

    elbowLeft.resize(3);
    elbowRight.resize(3);
    handLeft.resize(3);
    handRight.resize(3);
    head.resize(3);
    shoulderCenter.resize(3);
    shoulderLeft.resize(3);
    shoulderRight.resize(3);
    hipLeft.resize(3);
    hipRight.resize(3);
    kneeLeft.resize(3);
    kneeRight.resize(3);
    ankleLeft.resize(3);
    ankleRight.resize(3);

    cameraposinit.resize(3);
    focalpointinit.resize(3);

    keypoints2conf[KeyPointTag::shoulder_center] = make_pair("static", 0.0);
    keypoints2conf[KeyPointTag::head] = make_pair("static", 0.0);
    keypoints2conf[KeyPointTag::shoulder_left] = make_pair("static", 0.0);
    keypoints2conf[KeyPointTag::elbow_left] = make_pair("static", 0.0);
    keypoints2conf[KeyPointTag::hand_left] = make_pair("static", 0.0);
    keypoints2conf[KeyPointTag::shoulder_right] = make_pair("static", 0.0);
    keypoints2conf[KeyPointTag::elbow_right] = make_pair("static", 0.0);
    keypoints2conf[KeyPointTag::hand_right] = make_pair("static", 0.0);
    keypoints2conf[KeyPointTag::hip_left] = make_pair("static", 0.0);
    keypoints2conf[KeyPointTag::knee_left] = make_pair("static", 0.0);
    keypoints2conf[KeyPointTag::ankle_left] = make_pair("static", 0.0);
    keypoints2conf[KeyPointTag::hip_right] = make_pair("static", 0.0);
    keypoints2conf[KeyPointTag::knee_right] = make_pair("static", 0.0);
    keypoints2conf[KeyPointTag::ankle_right] = make_pair("static", 0.0);

}

bool Manager::loadInitialConf()
{
    ResourceFinder rf;
    rf.setVerbose();
    rf.setDefaultContext(this->rf->getContext().c_str());
    rf.setDefaultConfigFile(this->rf->find("from").asString().c_str()); //(this->rf->find("motion-repertoire").asString().c_str());

//    string confFile = this->rf->findFileByName(motion_repertoire_file);
//    rf.setDefaultConfigFile(confFile.c_str()); //(this->rf->find("configuration-file").asString().c_str());
    rf.configure(0, NULL);

    Bottle &bGeneral = rf.findGroup("GENERAL");

    if(!bGeneral.isNull())
    {
//        nmovements = bGeneral.find("number_movements").asInt();
        if(Bottle *bCamerapos_init = bGeneral.find("cameraposinit").asList())
        {
            cameraposinit[0] = bCamerapos_init->get(0).asDouble();
            cameraposinit[1] = bCamerapos_init->get(1).asDouble();
            cameraposinit[2] = bCamerapos_init->get(2).asDouble();
        }
        else
            yError() << "Could not load initial camera pos";

        if(Bottle *bFocalpoint_init = bGeneral.find("focalpointinit").asList())
        {
            focalpointinit[0] = bFocalpoint_init->get(0).asDouble();
            focalpointinit[1] = bFocalpoint_init->get(1).asDouble();
            focalpointinit[2] = bFocalpoint_init->get(2).asDouble();
        }
        else
            yError() << "Could not load initial focal point";

        if(Bottle *bElbowLeft_init = bGeneral.find("elbow_left_init_pose").asList())
        {
            elbowLeft_init[0] = bElbowLeft_init->get(0).asDouble();
            elbowLeft_init[1] = bElbowLeft_init->get(1).asDouble();
            elbowLeft_init[2] = bElbowLeft_init->get(2).asDouble();
            initial_keypoints.push_back(make_pair(KeyPointTag::elbow_left,elbowLeft_init));
        }
        else
            yError() << "Could not load initial pose for elbow left";

        if(Bottle *bElbowRight_init = bGeneral.find("elbow_right_init_pose").asList())
        {
            elbowRight_init[0] = bElbowRight_init->get(0).asDouble();
            elbowRight_init[1] = bElbowRight_init->get(1).asDouble();
            elbowRight_init[2] = bElbowRight_init->get(2).asDouble();
            initial_keypoints.push_back(make_pair(KeyPointTag::elbow_right,elbowRight_init));
        }
        else
            yError() << "Could not load initial pose for elbow right";

        if(Bottle *bHandLeft_init = bGeneral.find("hand_left_init_pose").asList())
        {
            handLeft_init[0] = bHandLeft_init->get(0).asDouble();
            handLeft_init[1] = bHandLeft_init->get(1).asDouble();
            handLeft_init[2] = bHandLeft_init->get(2).asDouble();
            initial_keypoints.push_back(make_pair(KeyPointTag::hand_left,handLeft_init));
        }
        else
            yError() << "Could not load initial pose for hand left";

        if(Bottle *bHandRight_init = bGeneral.find("hand_right_init_pose").asList())
        {
            handRight_init[0] = bHandRight_init->get(0).asDouble();
            handRight_init[1] = bHandRight_init->get(1).asDouble();
            handRight_init[2] = bHandRight_init->get(2).asDouble();
            initial_keypoints.push_back(make_pair(KeyPointTag::hand_right,handRight_init));
        }
        else
            yError() << "Could not load initial pose for hand right";

        if(Bottle *bHead_init = bGeneral.find("head_init_pose").asList())
        {
            head_init[0] = bHead_init->get(0).asDouble();
            head_init[1] = bHead_init->get(1).asDouble();
            head_init[2] = bHead_init->get(2).asDouble();
            initial_keypoints.push_back(make_pair(KeyPointTag::head,head_init));
        }
        else
            yError() << "Could not load initial pose for hand left";

        if(Bottle *bShoulderCenter_init = bGeneral.find("shoulder_center_init_pose").asList())
        {
            shoulderCenter_init[0] = bShoulderCenter_init->get(0).asDouble();
            shoulderCenter_init[1] = bShoulderCenter_init->get(1).asDouble();
            shoulderCenter_init[2] = bShoulderCenter_init->get(2).asDouble();
            initial_keypoints.push_back(make_pair(KeyPointTag::shoulder_center,shoulderCenter_init));
        }
        else
            yError() << "Could not load initial pose for shoulder center";

        if(Bottle *bShoulderLeft_init = bGeneral.find("shoulder_left_init_pose").asList())
        {
            shoulderLeft_init[0] = bShoulderLeft_init->get(0).asDouble();
            shoulderLeft_init[1] = bShoulderLeft_init->get(1).asDouble();
            shoulderLeft_init[2] = bShoulderLeft_init->get(2).asDouble();
            initial_keypoints.push_back(make_pair(KeyPointTag::shoulder_left,shoulderLeft_init));
        }
        else
            yError() << "Could not load initial pose for shoulder left";

        if(Bottle *bShoulderRight_init = bGeneral.find("shoulder_right_init_pose").asList())
        {
            shoulderRight_init[0] = bShoulderRight_init->get(0).asDouble();
            shoulderRight_init[1] = bShoulderRight_init->get(1).asDouble();
            shoulderRight_init[2] = bShoulderRight_init->get(2).asDouble();
            initial_keypoints.push_back(make_pair(KeyPointTag::shoulder_right,shoulderRight_init));
        }
        else
            yError() << "Could not load initial pose for shoulder right";

        if(Bottle *bHipLeft_init = bGeneral.find("hip_left_init_pose").asList())
        {
            hipLeft_init[0] = bHipLeft_init->get(0).asDouble();
            hipLeft_init[1] = bHipLeft_init->get(1).asDouble();
            hipLeft_init[2] = bHipLeft_init->get(2).asDouble();
            initial_keypoints.push_back(make_pair(KeyPointTag::hip_left,hipLeft_init));
        }
        else
            yError() << "Could not load initial pose for hip left";

        if(Bottle *bhipRight_init = bGeneral.find("hip_right_init_pose").asList())
        {
            hipRight_init[0] = bhipRight_init->get(0).asDouble();
            hipRight_init[1] = bhipRight_init->get(1).asDouble();
            hipRight_init[2] = bhipRight_init->get(2).asDouble();
            initial_keypoints.push_back(make_pair(KeyPointTag::hip_right,hipRight_init));
        }
        else
            yError() << "Could not load initial pose for hip right";

        if(Bottle *bKneeLeft_init = bGeneral.find("knee_left_init_pose").asList())
        {
            kneeLeft_init[0] = bKneeLeft_init->get(0).asDouble();
            kneeLeft_init[1] = bKneeLeft_init->get(1).asDouble();
            kneeLeft_init[2] = bKneeLeft_init->get(2).asDouble();
            initial_keypoints.push_back(make_pair(KeyPointTag::knee_left,kneeLeft_init));
        }
        else
            yError() << "Could not load initial pose for knee left";

        if(Bottle *bKneeRight_init = bGeneral.find("knee_right_init_pose").asList())
        {
            kneeRight_init[0] = bKneeRight_init->get(0).asDouble();
            kneeRight_init[1] = bKneeRight_init->get(1).asDouble();
            kneeRight_init[2] = bKneeRight_init->get(2).asDouble();
            initial_keypoints.push_back(make_pair(KeyPointTag::knee_right,kneeRight_init));
        }
        else
            yError() << "Could not load initial pose for knee right";

        if(Bottle *bAnkleLeft_init = bGeneral.find("ankle_left_init_pose").asList())
        {
            ankleLeft_init[0] = bAnkleLeft_init->get(0).asDouble();
            ankleLeft_init[1] = bAnkleLeft_init->get(1).asDouble();
            ankleLeft_init[2] = bAnkleLeft_init->get(2).asDouble();
            initial_keypoints.push_back(make_pair(KeyPointTag::ankle_left,ankleLeft_init));
        }
        else
            yError() << "Could not load initial pose for ankle left";

        if(Bottle *bAnkleRight_init = bGeneral.find("ankle_right_init_pose").asList())
        {
            ankleRight_init[0] = bAnkleRight_init->get(0).asDouble();
            ankleRight_init[1] = bAnkleRight_init->get(1).asDouble();
            ankleRight_init[2] = bAnkleRight_init->get(2).asDouble();
            initial_keypoints.push_back(make_pair(KeyPointTag::ankle_right,ankleRight_init));
        }
        else
            yError() << "Could not load initial pose for ankle right";
        if(Bottle *bHipCenter_init = bGeneral.find("hip_center_init_pose").asList())
        {
            hipCenter_init[0] = bHipCenter_init->get(0).asDouble();
            hipCenter_init[1] = bHipCenter_init->get(1).asDouble();
            hipCenter_init[2] = bHipCenter_init->get(2).asDouble();
            initial_keypoints.push_back(make_pair(KeyPointTag::hip_center,hipCenter_init));
        }
        else
            yError() << "Could not load initial pose for hip center";
    }

    return true;
}

bool Manager::loadInitialConf(const Bottle& b, SkeletonWaist* skeletonInit)
{
    new_keypoints = initial_keypoints;
    if(Bottle *bElbowLeft_init = b.find("elbow_left_init_pose").asList())
    {
        elbowLeft_init[0] = bElbowLeft_init->get(0).asDouble();
        elbowLeft_init[1] = bElbowLeft_init->get(1).asDouble();
        elbowLeft_init[2] = bElbowLeft_init->get(2).asDouble();

        for(int i=0; i<new_keypoints.size(); i++)
        {
            if(new_keypoints[i].first == KeyPointTag::elbow_left)
                new_keypoints[i] = make_pair(KeyPointTag::elbow_left,elbowLeft_init);
        }
    }

    if(Bottle *bElbowRight_init = b.find("elbow_right_init_pose").asList())
    {
        elbowRight_init[0] = bElbowRight_init->get(0).asDouble();
        elbowRight_init[1] = bElbowRight_init->get(1).asDouble();
        elbowRight_init[2] = bElbowRight_init->get(2).asDouble();

        for(int i=0; i<new_keypoints.size(); i++)
        {
            if(new_keypoints[i].first == KeyPointTag::elbow_right)
                new_keypoints[i] = make_pair(KeyPointTag::elbow_right,elbowRight_init);
        }
    }

    if(Bottle *bHandLeft_init = b.find("hand_left_init_pose").asList())
    {
        handLeft_init[0] = bHandLeft_init->get(0).asDouble();
        handLeft_init[1] = bHandLeft_init->get(1).asDouble();
        handLeft_init[2] = bHandLeft_init->get(2).asDouble();

        for(int i=0; i<new_keypoints.size(); i++)
        {
            if(new_keypoints[i].first == KeyPointTag::hand_left)
                new_keypoints[i] = make_pair(KeyPointTag::hand_left,handLeft_init);
        }
    }

    if(Bottle *bHandRight_init = b.find("hand_right_init_pose").asList())
    {
        handRight_init[0] = bHandRight_init->get(0).asDouble();
        handRight_init[1] = bHandRight_init->get(1).asDouble();
        handRight_init[2] = bHandRight_init->get(2).asDouble();

        for(int i=0; i<new_keypoints.size(); i++)
        {
            if(new_keypoints[i].first == KeyPointTag::hand_right)
                new_keypoints[i] = make_pair(KeyPointTag::hand_right,handRight_init);
        }
    }

    if(Bottle *bHead_init = b.find("head_init_pose").asList())
    {
        head_init[0] = bHead_init->get(0).asDouble();
        head_init[1] = bHead_init->get(1).asDouble();
        head_init[2] = bHead_init->get(2).asDouble();

        for(int i=0; i<new_keypoints.size(); i++)
        {
            if(new_keypoints[i].first == KeyPointTag::head)
                new_keypoints[i] = make_pair(KeyPointTag::head,head_init);
        }
    }

    if(Bottle *bShoulderCenter_init = b.find("shoulder_center_init_pose").asList())
    {
        shoulderCenter_init[0] = bShoulderCenter_init->get(0).asDouble();
        shoulderCenter_init[1] = bShoulderCenter_init->get(1).asDouble();
        shoulderCenter_init[2] = bShoulderCenter_init->get(2).asDouble();

        for(int i=0; i<new_keypoints.size(); i++)
        {
            if(new_keypoints[i].first == KeyPointTag::shoulder_center)
                new_keypoints[i] = make_pair(KeyPointTag::shoulder_center,shoulderCenter_init);
        }
    }

    if(Bottle *bShoulderLeft_init = b.find("shoulder_left_init_pose").asList())
    {
        shoulderLeft_init[0] = bShoulderLeft_init->get(0).asDouble();
        shoulderLeft_init[1] = bShoulderLeft_init->get(1).asDouble();
        shoulderLeft_init[2] = bShoulderLeft_init->get(2).asDouble();

        for(int i=0; i<new_keypoints.size(); i++)
        {
            if(new_keypoints[i].first == KeyPointTag::shoulder_left)
                new_keypoints[i] = make_pair(KeyPointTag::shoulder_left,shoulderLeft_init);
        }
    }

    if(Bottle *bShoulderRight_init = b.find("shoulder_right_init_pose").asList())
    {
        shoulderRight_init[0] = bShoulderRight_init->get(0).asDouble();
        shoulderRight_init[1] = bShoulderRight_init->get(1).asDouble();
        shoulderRight_init[2] = bShoulderRight_init->get(2).asDouble();

        for(int i=0; i<new_keypoints.size(); i++)
        {
            if(new_keypoints[i].first == KeyPointTag::shoulder_right)
                new_keypoints[i] = make_pair(KeyPointTag::shoulder_right,shoulderRight_init);
        }
    }

    if(Bottle *bHipLeft_init = b.find("hip_left_init_pose").asList())
    {
        hipLeft_init[0] = bHipLeft_init->get(0).asDouble();
        hipLeft_init[1] = bHipLeft_init->get(1).asDouble();
        hipLeft_init[2] = bHipLeft_init->get(2).asDouble();

        for(int i=0; i<new_keypoints.size(); i++)
        {
            if(new_keypoints[i].first == KeyPointTag::hip_left)
                new_keypoints[i] = make_pair(KeyPointTag::hip_left,hipLeft_init);
        }
    }

    if(Bottle *bhipRight_init = b.find("hip_right_init_pose").asList())
    {
        hipRight_init[0] = bhipRight_init->get(0).asDouble();
        hipRight_init[1] = bhipRight_init->get(1).asDouble();
        hipRight_init[2] = bhipRight_init->get(2).asDouble();

        for(int i=0; i<new_keypoints.size(); i++)
        {
            if(new_keypoints[i].first == KeyPointTag::hip_right)
                new_keypoints[i] = make_pair(KeyPointTag::hip_right,hipRight_init);
        }
    }

    if(Bottle *bKneeLeft_init = b.find("knee_left_init_pose").asList())
    {
        kneeLeft_init[0] = bKneeLeft_init->get(0).asDouble();
        kneeLeft_init[1] = bKneeLeft_init->get(1).asDouble();
        kneeLeft_init[2] = bKneeLeft_init->get(2).asDouble();

        for(int i=0; i<new_keypoints.size(); i++)
        {
            if(new_keypoints[i].first == KeyPointTag::knee_left)
                new_keypoints[i] = make_pair(KeyPointTag::knee_left,kneeLeft_init);
        }
    }

    if(Bottle *bKneeRight_init = b.find("knee_right_init_pose").asList())
    {
        kneeRight_init[0] = bKneeRight_init->get(0).asDouble();
        kneeRight_init[1] = bKneeRight_init->get(1).asDouble();
        kneeRight_init[2] = bKneeRight_init->get(2).asDouble();

        for(int i=0; i<new_keypoints.size(); i++)
        {
            if(new_keypoints[i].first == KeyPointTag::knee_right)
                new_keypoints[i] = make_pair(KeyPointTag::knee_right,kneeRight_init);
        }
    }

    if(Bottle *bAnkleLeft_init = b.find("ankle_left_init_pose").asList())
    {
        ankleLeft_init[0] = bAnkleLeft_init->get(0).asDouble();
        ankleLeft_init[1] = bAnkleLeft_init->get(1).asDouble();
        ankleLeft_init[2] = bAnkleLeft_init->get(2).asDouble();

        for(int i=0; i<new_keypoints.size(); i++)
        {
            if(new_keypoints[i].first == KeyPointTag::ankle_left)
                new_keypoints[i] = make_pair(KeyPointTag::ankle_left,ankleLeft_init);
        }
    }

    if(Bottle *bAnkleRight_init = b.find("ankle_right_init_pose").asList())
    {
        ankleRight_init[0] = bAnkleRight_init->get(0).asDouble();
        ankleRight_init[1] = bAnkleRight_init->get(1).asDouble();
        ankleRight_init[2] = bAnkleRight_init->get(2).asDouble();

        for(int i=0; i<new_keypoints.size(); i++)
        {
            if(new_keypoints[i].first == KeyPointTag::ankle_right)
                new_keypoints[i] = make_pair(KeyPointTag::ankle_right,ankleRight_init);
        }
    }
    if(Bottle *bHipCenter_init = b.find("ankle_right_init_pose").asList())
    {
        hipCenter_init[0] = bHipCenter_init->get(0).asDouble();
        hipCenter_init[1] = bHipCenter_init->get(1).asDouble();
        hipCenter_init[2] = bHipCenter_init->get(2).asDouble();

        for(int i=0; i<new_keypoints.size(); i++)
        {
            if(new_keypoints[i].first == KeyPointTag::hip_center)
                new_keypoints[i] = make_pair(KeyPointTag::hip_center,hipCenter_init);
        }
    }

    skeletonInit->update(new_keypoints);

    return true;
}

bool Manager::loadMotionList()
{
    LockGuard lg(mutex);

    ResourceFinder rf;
    rf.setVerbose();
    rf.setDefaultContext(this->rf->getContext().c_str());
    rf.setDefaultConfigFile(this->rf->find("from").asString().c_str());

//    string confFile = this->rf->findFileByName(motion_repertoire_file);
//    rf.setDefaultConfigFile(confFile.c_str()); //(this->rf->find("configuration-file").asString().c_str());
    rf.configure(0, NULL);

    Bottle &bGeneral = rf.findGroup("GENERAL");

    if(!bGeneral.isNull())
    {
        if(Bottle *metric_tag = bGeneral.find("metric_tag").asList())
        {
            if(Bottle *n_metric_tag = bGeneral.find("number_metrics").asList())
            {
                for(int i=0; i<metric_tag->size(); i++)
                {
                    string curr_tag = metric_tag->get(i).asString();
                    int motion_number = n_metric_tag->get(i).asInt();
//                    skeletonsInit.resize(motion_number);

                    for(int j=0; j<motion_number; j++)
                    {
                        Bottle &bMotion = rf.findGroup(curr_tag+"_"+to_string(j));
                        if(!bMotion.isNull())
                        {
                            string motion_type = bMotion.find("motion_type").asString();
                            string tag_joint = bMotion.find("tag_joint").asString();
                            double min = bMotion.find("min").asDouble();
                            double max = bMotion.find("max").asDouble();
                            double duration = bMotion.find("duration").asDouble();
                            int nrep = bMotion.find("nrep").asInt();
                            int nenv = bMotion.find("nenv").asInt();
                            double tempwin = bMotion.find("tempwin").asDouble();
                            double threshold = bMotion.find("threshold").asDouble();
                            Vector camerapos;
                            camerapos.resize(3);
                            if(Bottle *bCamerapos = bMotion.find("camerapos").asList())
                            {
                                camerapos[0] = bCamerapos->get(0).asDouble();
                                camerapos[1] = bCamerapos->get(1).asDouble();
                                camerapos[2] = bCamerapos->get(2).asDouble();
                            }
                            else
                                yError() << "Could not find camera position";

                            Vector focalpoint;
                            focalpoint.resize(3);
                            if(Bottle *bFocalpoint = bMotion.find("focalpoint").asList())
                            {
                                focalpoint[0] = bFocalpoint->get(0).asDouble();
                                focalpoint[1] = bFocalpoint->get(1).asDouble();
                                focalpoint[2] = bFocalpoint->get(2).asDouble();
                            }
                            else
                                yError() << "Could not find focal point";

                            Vector ref_dir;
                            ref_dir.resize(3);
                            if(Bottle *bRefdir = bMotion.find("ref_dir").asList())
                            {
                                ref_dir[0] = bRefdir->get(0).asDouble();
                                ref_dir[1] = bRefdir->get(1).asDouble();
                                ref_dir[2] = bRefdir->get(2).asDouble();
                            }
                            else
                                yError() << "Could not find reference direction";

                            string tag_plane = bMotion.find("tag_plane").asString();
                            double range_plane = bMotion.find("range_plane").asDouble();

                            string s = curr_tag+"_"+to_string(j);
                            getJointInitialConf(bMotion,s);

                            if(curr_tag == Rom_Processor::metric_tag)
                            {
                                metric_repertoire = new Rom();
                            }
                            else if(curr_tag == EndPoint_Processor::metric_tag)
                            {
                                Vector target;
                                target.resize(3);
                                if(Bottle *bTarget = bMotion.find("target").asList())
                                {
                                    target[0] = bTarget->get(0).asDouble();
                                    target[1] = bTarget->get(1).asDouble();
                                    target[2] = bTarget->get(2).asDouble();
                                }
                                else
                                    yError() << "Could not find target";

                                metric_repertoire = new EndPoint();
                                metric_repertoire->setTarget(target);
                            }

                            metric_repertoire->initialize(curr_tag, motion_type, tag_joint, ref_dir, tag_plane,
                                                          range_plane, min, max, duration, nrep, nenv, tempwin,
                                                          threshold, camerapos, focalpoint, keypoints2conf);

                            //add the current metric to the repertoire
                            motion_repertoire.insert(pair<string, Metric*>(curr_tag+"_"+to_string(j), metric_repertoire));
//                            motion_repertoire[curr_tag+"_"+to_string(j)]->print();
                        }
                    }
                }
            }
        }
    }
    else
    {
        yError() << "Error in loading parameters. Stopping module!";
        return false;
    }

    return true;
}

/********************************************************/
void Manager::getJointInitialConf(const Bottle &bMotion, const string &tag)
{
    skeletonInit = new SkeletonWaist();

    //overwrites initial configuration if different
    loadInitialConf(bMotion,skeletonInit);
    skeletonInit->setTag(tag);
    skeletonsInit.push_back(skeletonInit);

    Bottle *elbowLC = bMotion.find("elbow_left_configuration").asList();
    if(elbowLC)
    {
        string eLC = elbowLC->get(0).asString();
        keypoints2conf[KeyPointTag::elbow_left] = make_pair(eLC, elbowLC->get(1).asDouble());
    }
    else
        yError() << "Could not load elbow left configuration";

    Bottle *elbowRC = bMotion.find("elbow_right_configuration").asList();
    if(elbowRC)
    {
        string eRC = elbowRC->get(0).asString();
        keypoints2conf[KeyPointTag::elbow_right] = make_pair(eRC, elbowRC->get(1).asDouble());
    }
    else
        yError() << "Could not load elbow right configuration";

    Bottle *handLC = bMotion.find("hand_left_configuration").asList();
    if(handLC)
    {
        string hnLC = handLC->get(0).asString();
        keypoints2conf[KeyPointTag::hand_left] = make_pair(hnLC, handLC->get(1).asDouble());
    }
    else
        yError() << "Could not load hand left configuration";

    Bottle *handRC = bMotion.find("hand_right_configuration").asList();
    if(handRC)
    {
        string hnRC = handRC->get(0).asString();
        keypoints2conf[KeyPointTag::hand_right] = make_pair(hnRC, handRC->get(1).asDouble());
    }
    else
        yError() << "Could not load hand right configuration";

    Bottle *headC = bMotion.find("head_configuration").asList();
    if(headC)
    {
        string hC = headC->get(0).asString();
        keypoints2conf[KeyPointTag::head] = make_pair(hC, headC->get(1).asDouble());
    }
    else
        yError() << "Could not load head configuration";

    Bottle *shoulderCC = bMotion.find("shoulder_center_configuration").asList();
    if(shoulderCC)
    {
        string sCC = shoulderCC->get(0).asString();
        keypoints2conf[KeyPointTag::shoulder_center] = make_pair(sCC, shoulderCC->get(1).asDouble());
    }
    else
        yError() << "Could not load shoulder center configuration";

    Bottle *shoulderLC = bMotion.find("shoulder_left_configuration").asList();
    if(shoulderLC)
    {
        string sLC = shoulderLC->get(0).asString();
        keypoints2conf[KeyPointTag::shoulder_left] = make_pair(sLC, shoulderLC->get(1).asDouble());
    }
    else
        yError() << "Could not load shoulder left configuration";

    Bottle *shoulderRC = bMotion.find("shoulder_right_configuration").asList();
    if(shoulderRC)
    {
        string sRC = shoulderRC->get(0).asString();
        keypoints2conf[KeyPointTag::shoulder_right] = make_pair(sRC, shoulderRC->get(1).asDouble());
    }
    else
        yError() << "Could not load shoulder right configuration";

    Bottle *hipLC = bMotion.find("hip_left_configuration").asList();
    if(hipLC)
    {
        string hLC = hipLC->get(0).asString();
        keypoints2conf[KeyPointTag::hip_left] = make_pair(hLC, hipLC->get(1).asDouble());
    }
    else
        yError() << "Could not load hip left configuration";

    Bottle *hipRC = bMotion.find("hip_right_configuration").asList();
    if(hipRC)
    {
        string hRC = hipRC->get(0).asString();
        keypoints2conf[KeyPointTag::hip_right] = make_pair(hRC, hipRC->get(1).asDouble());
    }
    else
        yError() << "Could not load hip right configuration";

    Bottle *kneeLC = bMotion.find("knee_left_configuration").asList();
    if(kneeLC)
    {
        string kLC = kneeLC->get(0).asString();
        keypoints2conf[KeyPointTag::knee_left] = make_pair(kLC, kneeLC->get(1).asDouble());
    }
    else
        yError() << "Could not load knee left configuration";

    Bottle *kneeRC = bMotion.find("knee_right_configuration").asList();
    if(kneeRC)
    {
        string kRC = kneeRC->get(0).asString();
        keypoints2conf[KeyPointTag::knee_right] = make_pair(kRC, kneeRC->get(1).asDouble());
    }
    else
        yError() << "Could not load knee right configuration";

    Bottle *ankleLC = bMotion.find("ankle_left_configuration").asList();
    if(ankleLC)
    {
        string aLC = ankleLC->get(0).asString();
        keypoints2conf[KeyPointTag::ankle_left] = make_pair(aLC, ankleLC->get(1).asDouble());
    }
    else
        yError() << "Could not load ankle left configuration";

    Bottle *ankleRC = bMotion.find("ankle_right_configuration").asList();
    if(ankleRC)
    {
        string aRC = ankleRC->get(0).asString();
        keypoints2conf[KeyPointTag::ankle_right] = make_pair(aRC, ankleRC->get(1).asDouble());
    }
    else
        yError() << "Could not load ankle right configuration";
}

/********************************************************/
double Manager::loadMetric(const string &metric_tag)
{
    LockGuard lg(mutex);

    if(motion_repertoire.count(metric_tag))
    {
        metric = motion_repertoire.at(metric_tag);
        yInfo() << "Metric to analyze";
        metric->print();
        for(int j=0; j<skeletonsInit.size(); j++)
        {
            if(skeletonsInit[j]->getTag() == metric_tag)
            {
                skel = skeletonsInit[j];
                skel->print();
                break;
            }
        }

        processor = createProcessor(metric_tag, metric);

        //send commands to skeletonScaler
        Bottle cmd,reply;
        cmd.addVocab(Vocab::encode("load"));
        string f = metric->getMotionType() + ".log";
        string context = rf->getContext();
        cmd.addString(f);
        cmd.addString(context);
        scalerPort.write(cmd,reply);
        if(reply.get(0).asVocab()!=Vocab::encode("ok"))
        {
            yError() << "skeletonScaler could not load" << f << "file";
            return -1.0;
        }

        cmd.clear();
        reply.clear();
        cmd.addVocab(Vocab::encode("rot"));
        Vector cp = metric->getCameraPos();
        Vector fp = metric->getFocalPoint();
        cmd.addList().read(cp);
        cmd.addList().read(fp);
        scalerPort.write(cmd,reply);
        if(reply.get(0).asVocab()!=Vocab::encode("ok"))
        {
            yWarning() << "skeletonScaler could not rotate properly the camera";
        }

        cmd.clear();
        reply.clear();
        cmd.addVocab(Vocab::encode("tagt"));
        string tag_template = metric->getMotionType();
        cmd.addString(tag_template);
        dtwPort.write(cmd,reply);
        if(reply.get(0).asVocab()!=Vocab::encode("ok"))
        {
            yError() << "alignmentManager could not load the skeleton tag";
            return -1.0;
        }

        if(metric!=NULL)
            return metric->getDuration();

        return -1.0;
    }
    else
    {
        yWarning() << "The metric does not exist in the repertoire";
        return -1.0;
    }
}

/********************************************************/
vector<string> Manager::listMetrics()
{
    LockGuard lg(mutex);

    vector<string> reply;
    for (map<string,Metric*>::iterator it=motion_repertoire.begin(); it!=motion_repertoire.end(); it++)
    {
        reply.push_back(it->first);
    }

    return reply;
}

/********************************************************/
bool Manager::selectSkel(const string &skel_tag)
{
    LockGuard lg(mutex);

    this->skel_tag = skel_tag;
    yInfo() << "Analyzing skeleton " << this->skel_tag.c_str();

    //send tag to skeletonScaler
    Bottle cmd, reply;
    cmd.addVocab(Vocab::encode("tag"));
    cmd.addString(this->skel_tag);
    yInfo() << cmd.toString();

    scalerPort.write(cmd, reply);
    dtwPort.write(cmd, reply);

#if !__OPTIMIZE__
    yInfo() << "Debugging";
    {
        double t1 = Time::now();
        while( (Time::now()-t1) < 15.0 )
        {
            //do nothing
            yInfo() << "Waiting to start";
        }
        return startDebug();
    }
#endif

    return true;
}

/********************************************************/
double Manager::getQuality()
{
    LockGuard lg(mutex);

    double dev=processor->getDeviation();
    double dev1=0.0,score1=1.0;
    double dev2=2.5,score2=0.3;
    double score_static=score1+((score2-score1)/(dev2-dev1))*(dev-dev1);

//    double score_dynamic=fabs(result_der)/(metric->getMax()-metric->getMin());
//    double score_dynamic=0.0;
//    if(fabs(result_der)>30.0)
//        score_dynamic=score_exercise;

    double range=findMax()-findMin();
    double idealRange=metric->getMax()-metric->getMin();
    double score_dynamic=range/idealRange;
    
//    double score_dynamic=0.0;
//    if(range>metric->getThresh())
//        score_dynamic=score_exercise;

//    if(score_dynamic>0.6 && result>metric->getMin() && result<metric->getMax())
//        score_dynamic+=0.1;

    double score;
    if(score_static > 0.0)
        score = score_static;
    else
        score = 0.0;
        
    if(score_static > score_dynamic)
    {
        score = score_dynamic;
        if(score_dynamic > 1.0)
            score = 1.0;
    }
    
/*    if(score_dynamic<0.4 &&
            ( score_static>0.6 || (score_static>0.3 && score_static<0.6) || score_static<0.3) )
        score=LOW;

    else if( (score_dynamic>0.4 && score_dynamic<0.6) && score_static>0.6)
        score=MEDIUM;
    else if( (score_dynamic>0.4 && score_dynamic<0.6) && (score_static>0.3 && score_static<0.6) )
        score=MEDIUM;
    else if( (score_dynamic>0.4 && score_dynamic<0.6) && score_static<0.3)
        score=LOW;

    else if(score_dynamic>0.6 && score_static>0.6)
        score=HIGH;
    else if(score_dynamic>0.6 && score_static>0.3 && score_static<0.6)
        score=MEDIUM;
    else if(score_dynamic>0.6 && score_static<0.3)
        score=LOW; */

//    if(score_dynamic<0.6 && score_static>0.6)
//        score=LOW;
//    else if(score_dynamic<0.6 && score_static>0.3 && score_static<0.6)
//        score=LOW;
//    else if(score_dynamic<0.6 && score_static<0.3)
//        score=LOW;

    yInfo() << "score_static" << score_static << " score_dynamic" << score_dynamic 
            << " deviation" << dev << "range min-max" << range
            << "final_score" << score;

    return score;
}

/********************************************************/
double Manager::findMin()
{
    double min=result_time[0];
    for(int i=0;i<result_time.size();i++)
    {
        if(result_time[i]<min)
            min=result_time[i];
    }

    return min;
}

double Manager::findMax()
{
    double max=result_time[0];
    for(int i=0;i<result_time.size();i++)
    {
        if(result_time[i]>max)
            max=result_time[i];
    }

    return max;
}

/********************************************************/
bool Manager::start()
{
    LockGuard lg(mutex);
                
    yInfo() << "Start!";

    result_time.clear();

    Bottle cmd,reply;
    cmd.addVocab(Vocab::encode("run"));
    scalerPort.write(cmd,reply);
    if(reply.get(0).asVocab()!=Vocab::encode("ok"))
    {
		yError() << "Could not run skeletonScaler";
		return false;		
	}
	
	Time::delay(3.0);
	
    bool out=false;
    while(out==false)
    {    
        getSkeleton();

        //we do not start if we haven't selected a skeleton tag
        if(skel_tag.empty() || !updated)
        {
            yWarning() << "Please select a proper skeleton tag";
            return false;
        }

        out=skeletonIn.update_planes();
        Time::yield();
    }    

    processor->setInitialConf(skel,metric->getInitialConf(),skeletonIn);

    //start alignmentManager
    reply.clear();
    cmd.addInt(metric->getNrep());
    cmd.addInt(metric->getNenv());
    cmd.addDouble(metric->getDuration());
    dtwPort.write(cmd,reply);
    if(reply.get(0).asVocab()==Vocab::encode("ok"))
    {
        tstart_session = Time::now()-tstart;
        starting = true;
        return true;
    }
    return false;
}

/********************************************************/
bool Manager::startDebug()
{
    yInfo() << "Start!";

    result_time.clear();

    bool out=false;
    while(out==false)
    {
        getSkeleton();
        out=skeletonIn.update_planes();
        Time::yield();
    }

    processor->setInitialConf(skel,metric->getInitialConf(),skeletonIn);

    //start alignmentManager
    Bottle cmd,reply;
    cmd.addVocab(Vocab::encode("run"));
    scalerPort.write(cmd,reply);
    cmd.addInt(metric->getNrep());
    cmd.addInt(metric->getNenv());
    cmd.addDouble(metric->getDuration());
    dtwPort.write(cmd,reply);
    if(reply.get(0).asVocab()==Vocab::encode("ok"))
    {
        tstart_session = Time::now()-tstart;
        starting = true;
        return true;
    }
    return false;
}

/********************************************************/
bool Manager::stop()
{
    LockGuard lg(mutex);

    //stop skeletonScaler
    Bottle cmd, reply;
    cmd.addVocab(Vocab::encode("stop"));
    scalerPort.write(cmd,reply);
    dtwPort.write(cmd,reply);
    if(reply.get(0).asVocab()==Vocab::encode("ok") && starting)
    {
        cmd.clear();
        reply.clear();
        cmd.addVocab(Vocab::encode("rot"));
        cmd.addList().read(cameraposinit);
        cmd.addList().read(focalpointinit);
        yInfo() << cmd.toString();
        scalerPort.write(cmd, reply);

//        yInfo() << "stopping";
        starting = false;
//        metric = NULL;
        skel_tag = "";
        tend_session = Time::now()-tstart;

        // Use MATIO to write the results in a .mat file
        string filename_report = out_folder + "/user-" + skeletonIn.getTag() + "-" + metric->getMotionType() + "-" + to_string(nsession) + ".mat";
        mat_t *matfp = Mat_CreateVer(filename_report.c_str(),NULL,MAT_FT_MAT5);
        if (matfp == NULL)
            yError() << "Error creating MAT file";

        yInfo() << "Writing to file";
        if(writeKeypointsToFile(matfp))
        {
            time_samples.clear();
            all_keypoints.clear();
            all_planes.clear();
            ideal_samples.clear();
        }
        else
            yError() << "Could not save to file";

        yInfo() << "Keypoints saved to file" << filename_report.c_str();
        Mat_Close(matfp);

        nsession++;

        return true;
    }
    yError() << "Could not stop... maybe not started?";
    return false;
}

/********************************************************/
void Manager::getSkeleton()
{
    //ask for the property id
    Bottle cmd, reply;
    cmd.addVocab(Vocab::encode("ask"));
    Bottle &content = cmd.addList().addList();
    content.addString("skeleton");

//    yInfo() << "Query opc: " << cmd.toString();
    opcPort.write(cmd, reply);
//    yInfo() << "Reply from opc:" << reply.toString();

    if(reply.size() > 1)
    {
        if(reply.get(0).asVocab() == Vocab::encode("ack"))
        {
            if(Bottle *idField = reply.get(1).asList())
            {
                if(Bottle *idValues = idField->get(1).asList())
                {
                    if(!skel_tag.empty())
                    {
                        updated=false;
                        for(int i=0; i<idValues->size(); i++)
                        {
                            int id = idValues->get(i).asInt();

                            //given the id, get the value of the property
                            cmd.clear();
                            cmd.addVocab(Vocab::encode("get"));
                            Bottle &content = cmd.addList().addList();
                            Bottle replyProp;
                            content.addString("id");
                            content.addInt(id);

//                            yInfo() << "Command sent to the port: " << cmd.toString();
                            opcPort.write(cmd, replyProp);
//                            yInfo() << "Reply from opc:" << replyProp.toString();

                            if(replyProp.get(0).asVocab() == Vocab::encode("ack"))
                            {
                                if(Bottle *propField = replyProp.get(1).asList())
                                {
                                    Property prop(propField->toString().c_str());
                                    string tag=prop.find("tag").asString();
                                    if(!tag.empty())
                                    {
                                        if(prop.check("tag") && tag==skel_tag)
                                        {
                                            Skeleton* skeleton = skeleton_factory(prop);
                                            skeletonIn.update(skeleton->toProperty());
                                            if(skeleton->update_planes())
                                                all_keypoints.push_back(skeletonIn.get_unordered());
                                            updated=true;
                                            delete skeleton;
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

/********************************************************/
bool Manager::attach(RpcServer &source)
{
    return this->yarp().attachAsServer(source);
}

/********************************************************/
bool Manager::configure(ResourceFinder &rf)
{
    this->rf = &rf;
    string moduleName = rf.check("name", Value("motionAnalyzer")).asString();
    setName(moduleName.c_str());

    string robot = rf.check("robot", Value("icub")).asString();

    opcPort.open(("/" + getName() + "/opc").c_str());
    scopePort.open(("/" + getName() + "/scope").c_str());
    scalerPort.open(("/" + getName() + "/scaler:cmd").c_str());
    dtwPort.open(("/" + getName() + "/dtw:cmd").c_str());
    rpcPort.open(("/" + getName() + "/cmd").c_str());
    attach(rpcPort);

    metric=NULL;

    init();
    loadInitialConf();
    if(!loadMotionList())
        return false;

    out_folder = rf.getHomeContextPath();

    tstart = Time::now();

    nsession = 0;
    finishedSession = false;
    starting = false;

    skel_tag="";

    for(int i=0; i<skeletonsInit.size(); i++)
        skeletonsInit[i]->print();

    return true;
}

/********************************************************/
bool Manager::interruptModule()
{
    opcPort.interrupt();
    scopePort.interrupt();
    scalerPort.interrupt();
    dtwPort.interrupt();
    rpcPort.interrupt();
    yInfo() << "Interrupted module";

    return true;
}

/********************************************************/
bool Manager::close()
{
    delete metric_repertoire;
    delete metric;
    delete processor;
    delete skeletonInit;

    yInfo() << "Freed memory";

    opcPort.close();
    scopePort.close();
    scalerPort.close();
    dtwPort.close();
    rpcPort.close();

    yInfo() << "Closed ports";

    return true;
}

/********************************************************/
double Manager::getPeriod()
{
    return 0.01;
}

/********************************************************/
bool Manager::updateModule()
{
    LockGuard lg(mutex);

    //if we query the database
    if(opcPort.getOutputCount() > 0 && starting)
    {
        //if no metric has been defined we do not analyze motion
        if(metric != NULL)
        {
            //get skeleton and normalize
            getSkeleton();

            if(updated)
            {
                //update time array
                time_samples.push_back(Time::now()-tstart);

                processor->update(skeletonIn);
                processor->isDeviatingFromIntialPose();

                result = processor->computeMetric();
                all_planes.push_back(processor->getPlaneNormal());
                ideal_samples.push_back(processor->getIdeal());

                result_time.push_back(result);
                if((Time::now()-tstart)-tstart_session>metric->getTempWin())
                    result_time.pop_front();

                //write on output port
                Bottle &scopebottleout = scopePort.prepare();
                scopebottleout.clear();
                scopebottleout.addDouble(result);
                scopebottleout.addDouble(processor->getIdeal());
                scopePort.write();

            }
        }
        else
            yInfo() << "Please specify metric";
    }

    return true;
}

/********************************************************/
bool Manager::writeStructToMat(const string& name, const vector< vector< pair<string,Vector> > >& keypoints_skel, mat_t *matfp)
{
    const char *fields[numKeypoints];
    for(int i=0; i<numKeypoints; i++)
    {
//        if(keypoints_skel[0][i].first.c_str() != KeyPointTag::hip_center)
//        cout << i << " " << keypoints_skel[0][i].first.c_str() << endl;
        fields[i]=keypoints_skel[0][i].first.c_str();
    }

    matvar_t *field;

    size_t dim_struct[2] = {1,1};

    size_t nSamples = keypoints_skel.size()-1;

    matvar_t *matvar = Mat_VarCreateStruct(name.c_str(),2,dim_struct,fields,numKeypoints);
    if(matvar != NULL)
    {
        vector<double> field_vector;
        field_vector.resize(3*(nSamples));

        size_t dims_field[2] = {nSamples,3};

    //    print(keypoints_skel);

        for(int i=0; i<numKeypoints; i++)
        {
            for(int j=0; j<nSamples; j++)
            {
                field_vector[j] = keypoints_skel[j][i].second[0];
                field_vector[j+nSamples] = keypoints_skel[j][i].second[1];
                field_vector[j+2*nSamples] = keypoints_skel[j][i].second[2];
            }

            field = Mat_VarCreate(NULL,MAT_C_DOUBLE,MAT_T_DOUBLE,2,dims_field,field_vector.data(),MAT_F_GLOBAL);
            Mat_VarSetStructFieldByName(matvar, fields[i], 0, field);
        }

        Mat_VarWrite(matfp,matvar,MAT_COMPRESSION_NONE);
//        Mat_VarWriteAppend(matfp,matvar,MAT_COMPRESSION_NONE,1);
        Mat_VarFree(matvar);
    }
    else
        return false;

    return true;

}

/********************************************************/
bool Manager::writeStructToMat(const string& name, const Metric& metric, mat_t *matfp)
{
    int numFields=8;
    const char *fields[numFields] = {"motion_type", "ref_joint", "ref_direction", "ref_plane", "max", "min", "tstart", "tend"};
    matvar_t *field;

    size_t dim_struct[2] = {1,1};
    matvar_t *matvar = Mat_VarCreateStruct(name.c_str(),2,dim_struct,fields,numFields);

    if(matvar != NULL)
    {
        string motion_met = metric.getMotionType();
        char *motion_c = new char[motion_met.length() + 1];
        strcpy(motion_c, motion_met.c_str());
        size_t dims_field_motion[2] = {1,motion_met.size()};
        field = Mat_VarCreate(NULL,MAT_C_CHAR,MAT_T_UTF8,2,dims_field_motion,motion_c,0);
        Mat_VarSetStructFieldByName(matvar, fields[0], 0, field);
        delete [] motion_c;

        string joint_met = metric.getTagJoint();
        char *joint_c = new char[joint_met.length() + 1];
        strcpy(joint_c, joint_met.c_str());
        size_t dims_field_joint[2] = {1,joint_met.size()};

        field = Mat_VarCreate(NULL,MAT_C_CHAR,MAT_T_UTF8,2,dims_field_joint,joint_c,0);
        Mat_VarSetStructFieldByName(matvar, fields[1], 0, field);
        delete [] joint_c;

        size_t dims_field_dir[2] = {1,3};
        Vector ref_met = metric.getRefDir();
        field = Mat_VarCreate(NULL,MAT_C_DOUBLE,MAT_T_DOUBLE,2,dims_field_dir,ref_met.data(),MAT_F_DONT_COPY_DATA);
        Mat_VarSetStructFieldByName(matvar, fields[2], 0, field);

        size_t nPlanes = all_planes.size();
        vector<double> field_vector;
        field_vector.resize(3*nPlanes);
        size_t dims_field_plane[2] = {nPlanes,3};
        for(size_t i=0; i<nPlanes; i++)
        {
            field_vector[i] = all_planes[i][0];
            field_vector[i+nPlanes] = all_planes[i][1];
            field_vector[i+2*nPlanes] = all_planes[i][2];
        }
        field = Mat_VarCreate(NULL,MAT_C_DOUBLE,MAT_T_DOUBLE,2,dims_field_plane,field_vector.data(),MAT_F_GLOBAL);
        Mat_VarSetStructFieldByName(matvar, fields[3], 0, field);

//        field = Mat_VarCreate(NULL,MAT_C_DOUBLE,MAT_T_DOUBLE,2,dims_field_plane,all_planes.data(),MAT_F_GLOBAL);
//        Mat_VarSetStructFieldByName(matvar, fields[3], 0, field);

//        size_t dims_field_plane[2] = {1,3};
//        Vector plane_met = plane_normal;
//        field = Mat_VarCreate(NULL,MAT_C_DOUBLE,MAT_T_DOUBLE,2,dims_field_plane,plane_met.data(),MAT_F_DONT_COPY_DATA);
//        Mat_VarSetStructFieldByName(matvar, fields[3], 0, field);

        size_t dims_field_ideal[2] = {ideal_samples.size(),1};
        field = Mat_VarCreate(NULL,MAT_C_DOUBLE,MAT_T_DOUBLE,2,dims_field_ideal,ideal_samples.data(),MAT_F_DONT_COPY_DATA);
        Mat_VarSetStructFieldByName(matvar, fields[4], 0, field);

//        size_t dims_field_max[2] = {1,1};
//        double max_val = metric.getMax();
//        field = Mat_VarCreate(NULL,MAT_C_DOUBLE,MAT_T_DOUBLE,2,dims_field_max,&max_val,MAT_F_DONT_COPY_DATA);
//        Mat_VarSetStructFieldByName(matvar, fields[4], 0, field);

        size_t dims_field_min[2] = {1,1};
        double min_val = metric.getMin();
        field = Mat_VarCreate(NULL,MAT_C_DOUBLE,MAT_T_DOUBLE,2,dims_field_min,&min_val,MAT_F_DONT_COPY_DATA);
        Mat_VarSetStructFieldByName(matvar, fields[5], 0, field);

        size_t dims_field_tstart[2] = {1,1};
        cout << "started at " << tstart_session;
        field = Mat_VarCreate(NULL,MAT_C_DOUBLE,MAT_T_DOUBLE,2,dims_field_tstart,&tstart_session,MAT_F_DONT_COPY_DATA);
        Mat_VarSetStructFieldByName(matvar, fields[6], 0, field);

        cout << " ended at " << tend_session << endl;
        size_t dims_field_tend[2] = {1,1};
        field = Mat_VarCreate(NULL,MAT_C_DOUBLE,MAT_T_DOUBLE,2,dims_field_tend,&tend_session,MAT_F_DONT_COPY_DATA);
        Mat_VarSetStructFieldByName(matvar, fields[7], 0, field);

        Mat_VarWrite(matfp,matvar,MAT_COMPRESSION_NONE);
//        Mat_VarWriteAppend(matfp, matvar, MAT_COMPRESSION_NONE,1);
        Mat_VarFree(matvar);
    }
    else
        return false;


    return true;

}

/********************************************************/
void Manager::print(const vector< vector< pair<string,Vector> > >& keypoints_skel)
{
    for(int i=0; i<keypoints_skel[0].size(); i++)
    {
        for(int j=0; j<keypoints_skel.size(); j++)
        {
            cout << keypoints_skel[j][i].first << " " << keypoints_skel[j][i].second[0] << " "
                 << keypoints_skel[j][i].second[1] << " " << keypoints_skel[j][i].second[2] << endl;
        }
    }
    cout << endl;
}

/********************************************************/
bool Manager::writeKeypointsToFile(mat_t *matfp)
{
    // Use MATIO to write the results in a .mat file

    //Save time samples
    size_t nSamples = time_samples.size();
    size_t dims[2] = {nSamples,1};
    matvar_t *matvar = Mat_VarCreate("Time_samples", MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, time_samples.data(), 0);
    if(matvar != NULL)
    {
        Mat_VarWrite(matfp, matvar, MAT_COMPRESSION_NONE);
//        Mat_VarWriteAppend(matfp, matvar, MAT_COMPRESSION_NONE,1);
        Mat_VarFree(matvar);
    }
    else
    {
        yError() << "Could not save time samples.. the file will not be saved";
        return false;
    }

//    if(all_keypoints.size() != 0)
//        print(all_keypoints);

    //Save keypoint
    if(!writeStructToMat("Keypoints", all_keypoints, matfp))
    {
        yError() << "Could not save keypoints.. the file will not be saved";
        return false;
    }

//    //Save kind of metric to process
//    for(int i=0; i<metrics.size(); i++)
//    {
//        //         cout << metrics[i]->getName().c_str() << endl;
//        if(!writeStructToMat(metrics[i]->getName().c_str(), *metrics[i]))
//        {
//            yError() << "Could not save metrics.. the file will not be saved";
//            return false;
//        }
//    }

    //Save kind of metric to process
    if(!writeStructToMat(metric->getName().c_str(), *metric, matfp))
    {
        yError() << "Could not save metrics.. the file will not be saved";
        return false;
    }

    return true;
}
