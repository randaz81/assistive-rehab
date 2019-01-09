/*
 * Copyright (C) 2006-2019 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

// Autogenerated by Thrift Compiler (0.12.0-yarped)
//
// This is an automatically generated file.
// It could get re-generated if the ALLOW_IDL_GENERATION flag is on.

#ifndef YARP_THRIFT_GENERATOR_SERVICE_MOTIONANALYZER_IDL_H
#define YARP_THRIFT_GENERATOR_SERVICE_MOTIONANALYZER_IDL_H

#include <yarp/os/Wire.h>
#include <yarp/os/idl/WireTypes.h>

class motionAnalyzer_IDL :
        public yarp::os::Wire
{
public:
    // Constructor
    motionAnalyzer_IDL();

    /**
     * Load exercise to analyze.
     * @param exercise_tag name of the exercise to analyze
     * @return true/false on failure.
     */
    virtual bool loadExercise(const std::string& exercise_tag);

    /**
     * Get the name of the exercise begin performed.
     * @return string containing the name of the exercise begin performed / empty string on failure.
     */
    virtual std::string getExercise();

    /**
     * List available exercises.
     * @return the list of the available exercises as defined in the motion-repertoire.
     */
    virtual std::vector<std::string> listExercises();

    /**
     * Start processing.
     * @param use_robot_template true if robot template is used.
     * @return true/false on success/failure.
     */
    virtual bool start(const bool use_robot_template);

    /**
     * Stop feedback.
     * @return true/false on success/failure.
     */
    virtual bool stopFeedback();

    /**
     * Stop processing.
     * @return true/false on success/failure.
     */
    virtual bool stop();

    /**
     * Select skeleton by its tag.
     * @param skel_tag tag of the skeleton to process
     * @return true/false on success/failure.
     */
    virtual bool selectSkel(const std::string& skel_tag);

    /**
     * List joints on which feedback is computed.
     * @return the list of joints on which feedback is computed.
     */
    virtual std::vector<std::string> listJoints();

    /**
     * Select property to visualize.
     * @param prop property to visualize
     * @return true/false on success/failure.
     */
    virtual bool selectMetricProp(const std::string& prop_tag);

    /**
     * List the available properties computable for the current metric.
     * @return the list of the available properties computable for the current metric.
     */
    virtual std::vector<std::string> listMetricProps();

    /**
     * Select metric to analyze.
     * @param metric_tag metric to analyze
     * @return true/false on success/failure.
     */
    virtual bool selectMetric(const std::string& metric_tag);

    /**
     * List the available metrics for the current exercise.
     * @return the list of the available metrics for the current exercise.
     */
    virtual std::vector<std::string> listMetrics();

    /**
     * Get the metric to visualise.
     * @return metric to visualise.
     */
    virtual std::string getCurrMetricProp();

    /**
     * Select the part to move.
     * @return true/false on success/failure.
     */
    virtual bool setPart(const std::string& part);

    /**
     * Select the template for the analysis.
     * @return true/false on success/failure.
     */
    virtual bool setTemplateTag(const std::string& template_tag);

    /**
     * Mirror the skeleton template.
     * @param robot_skeleton_mirror if true, robot template has to be mirrored.
     * @return true/false on success/failure.
     */
    virtual bool mirrorTemplate(const bool robot_skeleton_mirror);

    /**
     * Detect if the person is standing.
     * @param standing_thresh threshold on the speed of the shoulder center height [m/s].
     * @return true/false if the person is/is not standing.
     */
    virtual bool isStanding(const double standing_thresh);

    /**
     * Detect if the person is sitting.
     * @param standing_thresh threshold on the speed of the shoulder center height [m/s].
     * @return true/false if the person is/is not standing.
     */
    virtual bool isSitting(const double standing_thresh);

    /**
     * Detect if the finish line has been crossed.
     * @param finishline_thresh distance between foot and finish line [m].
     * @return true/false if the finish line has been/not been crossed.
     */
    virtual bool hasCrossedFinishLine(const double finishline_thresh);

    /**
     * Set the pose of the finish line.
     * @param pose of the finish line.
     * @return true/false on success/failure.
     */
    virtual bool setLinePose(const std::vector<double>& line_pose);

    /**
     * Freeze module.
     * @return true/false on success/failure.
     */
    virtual bool freeze();

    // help method
    virtual std::vector<std::string> help(const std::string& functionName = "--all");

    // read from ConnectionReader
    bool read(yarp::os::ConnectionReader& connection) override;
};

#endif // YARP_THRIFT_GENERATOR_SERVICE_MOTIONANALYZER_IDL_H
