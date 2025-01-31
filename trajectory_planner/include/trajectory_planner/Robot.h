#pragma once

#include <ros/ros.h>
#include <ros/callback_queue.h>
#include <ros/console.h>
#include <ros/package.h>
#include <std_srvs/Empty.h>
#include "trajectory_planner/JntAngs.h"
#include "trajectory_planner/Trajectory.h"
#include "trajectory_planner/GeneralTraj.h"
#include "nav_msgs/Odometry.h"
#include <tf/transform_broadcaster.h> 

#include "DCM.h"
#include "Link.h"
#include "PID.h"
#include "Controller.h"
#include "Ankle.h"
#include "MinJerk.h"
#include "GeneralMotion.h"
#include "QuatEKF.h"
#include "LieEKF.h"
#include "PreviewTraj.h"
#include "ZMPPlanner.h"
#include "AnkleTraj.h"
#include "FootStepPlanner.h"

#include "fstream"
#include <random>
#include <chrono>
#include <thread>

using namespace std;

class Robot{
    friend class Surena;
    public:
        Robot(ros::NodeHandle *nh, Controller robot_ctrl);
        ~Robot();

        void spinOnline(int iter, double config[], double jnt_vel[], Vector3d torque_r, Vector3d torque_l, double f_r, double f_l, Vector3d gyro, Vector3d accelerometer, double* joint_angles);
        void spinOffline(int iter, double* config);

        void extractTrajParams(const trajectory_planner::Trajectory::Request& req);
        void planFootSteps();
        void planCoMTraj();
        void getInitCondition(Vector3d& x0, Vector3d& y0);
        void planAnkleTraj();
        
        bool jntAngsCallback(trajectory_planner::JntAngs::Request  &req,
                            trajectory_planner::JntAngs::Response &res);
        bool trajCallback(trajectory_planner::Trajectory::Request  &req,
                            trajectory_planner::Trajectory::Response &res);
        bool generalTrajCallback(trajectory_planner::GeneralTraj::Request  &req,
                                trajectory_planner::GeneralTraj::Response &res);
        bool resetTrajCallback(std_srvs::Empty::Request  &req,
                              std_srvs::Empty::Response &res);

        int findTrajIndex(vector<int> arr, int n, int K);

        void distributeFT(Vector3d zmp_y, Vector3d r_foot_y,Vector3d l_foot_y, Vector3d &r_wrench, Vector3d &l_wrench);

        void baseOdomPublisher(Vector3d base_pos, Vector3d base_vel, Quaterniond base_quat);
    private:

        double thigh_;
        double shank_;
        double torso_;
        double mass_;

        int num_steps_ = 2;
        double CoM_height_ = 0.68;
        double step_len_ = 0.15;
        double step_width_ = 0;
        double t_init_ds_ = 1;
        double t_ds_ = 0.1;
        double t_step_ = 1;
        double t_final_ds_ = 1;
        double swing_height_ = 0.025;
        double theta_ = 0;
        double dt_ = 0.005;
        bool use_file_ = true;

        double joints_[12];

        PID* DCMController_;
        PID* CoMController_;
        Controller onlineWalk_;
        QuatEKF* quatEKF_;
        LieEKF* lieEKF_;
        FootStepPlanner* stepPlanner_;
        ZMPPlanner ZMPPlanner_;

        template <typename T>
        T* appendTrajectory(T* old_traj, T* new_traj, int old_size, int new_size){
            /*
                This function appends a new trajectory to an old one.
                for example:
                when you want to call general_traj service twice.
            */
            int total_size = old_size + new_size;
            T* temp_traj = new T[total_size];
            copy(old_traj, old_traj + old_size, temp_traj);
            delete[] old_traj;
            old_traj = temp_traj;
            temp_traj = new_traj;
            copy(temp_traj, temp_traj + new_size, old_traj + old_size);
            return old_traj;
        }

        void doIK(MatrixXd pelvisP, Matrix3d pelvisR, MatrixXd leftAnkleP, Matrix3d leftAnkleR, MatrixXd rightAnkleP, Matrix3d rightAnkleR);
        double* geometricIK(MatrixXd p1, MatrixXd r1, MatrixXd p7, MatrixXd r7, bool isLeft);
        Matrix3d Rroll(double phi);
        Matrix3d RPitch(double theta);

        vector<Vector3d> CoMPos_;
        vector<Matrix3d> CoMRot_;
        Vector3d* zmpd_;
        Vector3d* CoMDot_;
        Vector3d* xiDesired_;
        Vector3d* xiDot_;
        vector<Vector3d> rAnklePos_;
        vector<Vector3d> lAnklePos_;
        vector<Matrix3d> rAnkleRot_;
        vector<Matrix3d> lAnkleRot_;
        vector<int> robotState_;

        Vector3d rSole_;    // current position of right sole
        Vector3d lSole_;    // current position of left sole
        Vector3d* FKCoM_;      // current CoM of robot
        Vector3d* FKCoMDot_;
        Vector3d* realXi_;
        Vector3d* realZMP_;      // current ZMP of robot
        Vector3d* rSoles_;
        Vector3d* lSoles_;
        Vector3d* cntOut_;
        bool leftSwings_;
        bool rightSwings_;

        _Link* links_[13];

        Vector3d CoMEstimatorFK(double config[]);
        void updateState(double config[], Vector3d torque_r, Vector3d torque_l, double f_r, double f_l, Vector3d gyro, Vector3d accelerometer);
        Matrix3d rDot_(Matrix3d R);
        void updateSolePosition();
        Vector3d getZMPLocal(Vector3d torque, double fz);
        Vector3d ZMPGlobal(Vector3d zmp_r, Vector3d zmp_l, double f_r, double f_l);

        ros::ServiceServer jntAngsServer_;
        ros::ServiceServer trajGenServer_;
        ros::ServiceServer generalTrajServer_;
        ros::ServiceServer resetTrajServer_;

        ros::NodeHandle trajNode_;
        ros::CallbackQueue trajCbQueue_;

        ros::Publisher baseOdomPub_;
        tf::TransformBroadcaster baseOdomBroadcaster_;

        bool isTrajAvailable_;
        bool useController_;

        int index_;
        int size_;
        int dataSize_;
        vector<int> trajSizes_;
        vector<bool> trajContFlags_;
};
