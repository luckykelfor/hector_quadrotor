#include <boost/bind.hpp>
#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <Eigen/Eigen>
#include <tf/transform_listener.h>

using namespace std;
using namespace Eigen;

Matrix3d Rnb;
Vector3d tnb;

ros::Time star;

int target_height;
int velocity;
int length;


//generate a rectangle path
void generate_path(double t, Vector3d& p, Vector3d& v)
{
  int n = floor(velocity*t/length);
  double l = velocity*t - n*length;
  if(n % 4 == 0)
  {
    p(0) = l;
    p(1) = 0;
    v(0) = velocity;
    v(1) = 0;
  }else if(n % 4 == 1)
  {
    p(0) = length;
    p(1) = l;
    v(0) = 0;
    v(1) = velocity;
  }else if(n % 4 == 2)
  {
    p(0) = length - l;
    p(1) = length;
    v(0) = -velocity;
    v(1) = 0;
  }else if(n % 4 == 3)
  {
    p(0) = 0;
    p(1) = length - l;
    v(0) = 0;
    v(1) = -velocity;
  }
  p(0) -= length/2;
  p(1) -= length/2;
  p(2) = target_height;
}

Vector3d generate_vel_cmd(Vector3d p, Vector3d p_target, Vector3d v_target)
{
  double k = 0.1;
  Vector3d v_cmd = k*(p_target - p) + v_target;
  return v_cmd;
}

int main(int argc, char** argv)
{
  
  ros::init(argc, argv, "quadrotor_draw_circle");
  ros::NodeHandle nh;
  nh.param("height", target_height, 3);
  nh.param("length", length, 4);
  nh.param("velocity", velocity, 4);
  cout << "length: " << length << "\n" 
       << "height: " << target_height << "\n" 
       << "velocity: " << velocity << endl;

  //cmd_vel_mux/input/teleop
  ros::Publisher twist_pub = nh.advertise<geometry_msgs::Twist>("cmd_vel", 1);

  
  ros::Rate loop_rate(50);
  ros::Time start = ros::Time::now();
  tf::TransformListener listener;

  while(ros::ok())
  {
    
    Vector3d pt, vt;
    
    tf::StampedTransform transform;
    try{
      //targe_frame <- source frame
      listener.waitForTransform("/world", "/base_link", start, ros::Duration(1.0));
        listener.lookupTransform("/world", "/base_link",
                     start, transform);
      }
      catch (tf::TransformException &ex) 
      {
        ROS_ERROR("%s",ex.what());
        ros::Duration(1.0).sleep();
        continue;
      }
      Vector3d p;
      Quaterniond q;
      p(0) = transform.getOrigin().x();
      p(1) = transform.getOrigin().y();
      p(2) = transform.getOrigin().z();
      q.w() = transform.getRotation().w();
      q.x() = transform.getRotation().x();
      q.y() = transform.getRotation().y();
      q.z() = transform.getRotation().z();
      Matrix3d Rnb = q.toRotationMatrix();

      double t = (ros::Time::now() - start).toSec();
      generate_path(t, pt, vt);
      Vector3d vel_cmd_n = generate_vel_cmd(p, pt, vt);
      //vel in world to body
      Vector3d vel_cmd = Rnb.transpose()*vel_cmd_n;//world to body
      cout << "vel_cmd: " << vel_cmd.transpose() << endl;
      geometry_msgs::Twist twist;
      twist.linear.x = vel_cmd(0);
      twist.linear.y = vel_cmd(1);
      twist.linear.z = vel_cmd(2);
      twist_pub.publish(twist);
      loop_rate.sleep();
  }

  ros::spin();
  return 0;
}
