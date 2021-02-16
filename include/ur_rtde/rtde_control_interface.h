#pragma once
#ifndef RTDE_CONTROL_INTERFACE_H
#define RTDE_CONTROL_INTERFACE_H

#include <ur_rtde/rtde_export.h>
#include <ur_rtde/rtde.h>
#include <map>

#define MAJOR_VERSION 0
#define MINOR_VERSION 1
#define CB3_MAJOR_VERSION 3
#define UR_CONTROLLER_RDY_FOR_CMD 1
#define UR_CONTROLLER_DONE_WITH_CMD 2
#define UR_EXECUTION_TIMEOUT 300
#define UR_PATH_EXECUTION_TIMEOUT 600
#define UR_GET_READY_TIMEOUT 3
#define RTDE_START_SYNCHRONIZATION_TIMEOUT 5
#define WAIT_FOR_PROGRAM_RUNNING_TIMEOUT 60

#define UR_JOINT_VELOCITY_MAX 3.14      // rad/s
#define UR_JOINT_VELOCITY_MIN 0         // rad/s
#define UR_JOINT_ACCELERATION_MAX 40.0  // rad/s^2
#define UR_JOINT_ACCELERATION_MIN 0     // rad/s^2
#define UR_TOOL_VELOCITY_MAX 3.0        // m/s
#define UR_TOOL_VELOCITY_MIN 0          // m/s
#define UR_TOOL_ACCELERATION_MAX 150.0  // m/s^2
#define UR_TOOL_ACCELERATION_MIN 0      // m/s^2
#define UR_SERVO_LOOKAHEAD_TIME_MAX 0.2
#define UR_SERVO_LOOKAHEAD_TIME_MIN 0.03
#define UR_SERVO_GAIN_MAX 2000
#define UR_SERVO_GAIN_MIN 100
#define UR_BLEND_MAX 2.0
#define UR_BLEND_MIN 0.0

// forward declarations
namespace boost { class thread; }
namespace ur_rtde { class DashboardClient; }
namespace ur_rtde { class ScriptClient; }
namespace ur_rtde { class RobotState; }
namespace ur_rtde { class RTDE; }

namespace ur_rtde
{
class Path;

class RTDEControlInterface
{
 public:
  RTDE_EXPORT explicit RTDEControlInterface(std::string hostname, bool upload_script = true, bool verbose = false,
                                            bool use_upper_range_registers = false);

  RTDE_EXPORT virtual ~RTDEControlInterface();

  enum RobotStatus
  {
    ROBOT_STATUS_POWER_ON = 0,
    ROBOT_STATUS_PROGRAM_RUNNING = 1,
    ROBOT_STATUS_TEACH_BUTTON_PRESSED = 2,
    ROBOT_STATUS_POWER_BUTTON_PRESSED = 3
  };

  enum SafetyStatus
  {
    IS_NORMAL_MODE = 0,
    IS_REDUCED_MODE = 1,
    IS_PROTECTIVE_STOPPED = 2,
    IS_RECOVERY_MODE = 3,
    IS_SAFEGUARD_STOPPED = 4,
    IS_SYSTEM_EMERGENCY_STOPPED = 5,
    IS_ROBOT_EMERGENCY_STOPPED = 6,
    IS_EMERGENCY_STOPPED = 7,
    IS_VIOLATION = 8,
    IS_FAULT = 9,
    IS_STOPPED_DUE_TO_SAFETY = 10
  };

  enum Feature
  {
    FEATURE_BASE,
    FEATURE_TOOL,
    FEATURE_CUSTOM  // not supported yet - reserved for future
  };

  /**
   * @returns Can be used to disconnect from the robot. To reconnect you have to call the reconnect() function.
   */
  RTDE_EXPORT void disconnect();

  /**
   * @returns Can be used to reconnect to the robot after a lost connection.
   */
  RTDE_EXPORT bool reconnect();

  /**
   * @returns Connection status for RTDE, useful for checking for lost connection.
   */
  RTDE_EXPORT bool isConnected();

  /**
   * @brief In the event of an error, this function can be used to resume operation by reuploading the RTDE control
   * script. This will only happen if a script is not already running on the controller.
   */
  RTDE_EXPORT bool reuploadScript();

  /**
   * @brief Send a custom ur script to the controller
   * @param function_name specify a name for the custom script function
   * @param script the custom ur script to be sent to the controller specified as a string,
   * each line must be terminated with a newline. The code will automatically be indented with one tab
   * to fit with the function body.
   */
  RTDE_EXPORT bool sendCustomScriptFunction(const std::string &function_name, const std::string &script);

  /**
   * Send a custom ur script to the controller
   * The function enables sending of short scripts which was defined inline
   * within source code. So you can write code like this:
   * \code
   * const std::string inline_script =
                "def script_test():\n"
                        "\tdef test():\n"
                                "textmsg(\"test1\")\n"
                                "textmsg(\"test2\")\n"
                        "\tend\n"
                        "\twrite_output_integer_register(0, 1)\n"
                        "\ttest()\n"
                        "\ttest()\n"
                        "\twrite_output_integer_register(0, 2)\n"
                "end\n"
                "run program\n";
          bool result = rtde_c.sendCustomScript(inline_script);
   * \endcode
   * @return Returns true if the script has been executed successfully and false
   * on timeout
   */
  RTDE_EXPORT bool sendCustomScript(const std::string &script);

  /**
   * @brief Send a custom ur script file to the controller
   * @param file_path the file path to the custom ur script file
   */
  RTDE_EXPORT bool sendCustomScriptFile(const std::string &file_path);

  /**
   * Assign a custom script file that will be sent to device as the main
   * control script.
   * Setting an empty file_name will disable the custom script loading
   * This eases debugging when modifying the control
   * script because it does not require to recompile the whole library
   */
  RTDE_EXPORT void setCustomScriptFile(const std::string &file_path);

  /**
   * @brief This function will terminate the script on controller.
   */
  RTDE_EXPORT void stopScript();

  /**
   * @brief Stop (linear in tool space) - decelerate tool speed to zero
   * @param a tool acceleration [m/s^2] (rate of deceleration of the tool)
   */
  RTDE_EXPORT void stopL(double a = 10.0);

  /**
   * @brief Stop (linear in joint space) - decelerate joint speeds to zero
   * @param a joint acceleration [rad/s^2] (rate of deceleration of the leading axis).
   */
  RTDE_EXPORT void stopJ(double a = 2.0);

  /**
   * @brief Move to joint position (linear in joint-space)
   * @param q joint positions
   * @param speed joint speed of leading axis [rad/s]
   * @param acceleration joint acceleration of leading axis [rad/s^2]
   * @param async a bool specifying if the move command should be asynchronous. If async is true it is possible to
   * stop a move command using either the stopJ or stopL function. Default is false, this means the function will
   * block until the movement has completed.
   */
  RTDE_EXPORT bool moveJ(const std::vector<double> &q, double speed = 1.05, double acceleration = 1.4,
                         bool async = false);

  /**
   * @brief Move to each joint position specified in a path
   * @param path with joint positions that includes acceleration, speed and blend for each position
   * @param async a bool specifying if the move command should be asynchronous. If async is true it is possible to
   * stop a move command using either the stopJ or stopL function. Default is false, this means the function will
   * block until the movement has completed.
   */
  RTDE_EXPORT bool moveJ(const std::vector<std::vector<double>> &path, bool async = false);

  /**
   * @brief Move to pose (linear in joint-space)
   * @param pose target pose
   * @param speed joint speed of leading axis [rad/s]
   * @param acceleration joint acceleration of leading axis [rad/s^2]
   * @param async a bool specifying if the move command should be asynchronous. If async is true it is possible to
   * stop a move command using either the stopJ or stopL function. Default is false, this means the function will
   * block until the movement has completed.
   */
  RTDE_EXPORT bool moveJ_IK(const std::vector<double> &pose, double speed = 1.05, double acceleration = 1.4,
                            bool async = false);

  /**
   * @brief Move to position (linear in tool-space)
   * @param pose target pose
   * @param speed tool speed [m/s]
   * @param acceleration tool acceleration [m/s^2]
   * @param async a bool specifying if the move command should be asynchronous. If async is true it is possible to
   * stop a move command using either the stopJ or stopL function. Default is false, this means the function will
   * block until the movement has completed.
   */
  RTDE_EXPORT bool moveL(const std::vector<double> &pose, double speed = 0.25, double acceleration = 1.2,
                         bool async = false);

  /**
   * @brief Move to each pose specified in a path
   * @param path with tool poses that includes acceleration, speed and blend for each position
   * @param async a bool specifying if the move command should be asynchronous. If async is true it is possible to
   * stop a move command using either the stopJ or stopL function. Default is false, this means the function will
   * block until the movement has completed.
   */
  RTDE_EXPORT bool moveL(const std::vector<std::vector<double>> &path, bool async = false);

  /**
   * @brief Move to position (linear in tool-space)
   * @param q joint positions
   * @param speed tool speed [m/s]
   * @param acceleration tool acceleration [m/s^2]
   * @param async a bool specifying if the move command should be asynchronous. If async is true it is possible to
   * stop a move command using either the stopJ or stopL function. Default is false, this means the function will
   * block until the movement has completed.
   */
  RTDE_EXPORT bool moveL_FK(const std::vector<double> &q, double speed = 0.25, double acceleration = 1.2,
                            bool async = false);

  /**
   * @brief Move Circular: Move to position (circular in tool-space)
   * @param pose_via path point (note: only position is used)
   * @param pose_to target pose (note: only position is used in Fixed orientation mode).
   * @param speed tool speed [m/s]
   * @param acceleration tool acceleration [m/s^2]
   * @param blend blend radius [m]
   * @param mode 0: Unconstrained mode. Interpolate orientation from current pose to target pose (pose_to)
   * 1: Fixed mode. Keep orientation constant relative to the tangent of the circular arc (starting from current pose)
   */
  RTDE_EXPORT bool moveC(const std::vector<double> &pose_via, const std::vector<double> &pose_to, double speed = 0.25,
                         double acceleration = 1.2, double blend = 0.0, int mode = 0);

  /**
   * @brief Move Process: Blend circular (in tool-space) and move linear (in tool-space) to position. Accelerates to
   * and moves with constant tool speed v.
   * @param pose target pose
   * @param speed tool speed [m/s]
   * @param acceleration tool acceleration [m/s^2]
   * @param blend blend radius [m]
   */
  RTDE_EXPORT bool moveP(const std::vector<double> &pose, double speed = 0.25, double acceleration = 1.2,
                         double blend = 0.0);

  /**
   * @brief Joint speed - Accelerate linearly in joint space and continue with constant joint speed
   * @param qd joint speeds [rad/s]
   * @param acceleration joint acceleration [rad/s^2] (of leading axis)
   * @param time time [s] before the function returns (optional)
   */
  RTDE_EXPORT bool speedJ(const std::vector<double> &qd, double acceleration = 0.5, double time = 0.0);

  /**
   * @brief Tool speed - Accelerate linearly in Cartesian space and continue with constant tool speed. The time t is
   * optional;
   * @param xd tool speed [m/s] (spatial vector)
   * @param acceleration tool position acceleration [m/s^2]
   * @param time time [s] before the function returns (optional)
   */
  RTDE_EXPORT bool speedL(const std::vector<double> &xd, double acceleration = 0.25, double time = 0.0);

  /**
   * @brief Servo to position (linear in joint-space)
   * @param q joint positions [rad]
   * @param speed NOT used in current version
   * @param acceleration NOT used in current version
   * @param time time where the command is controlling the robot. The function is blocking for time t [S]
   * @param lookahead_time time [S], range [0.03,0.2] smoothens the trajectory with this lookahead time
   * @param gain proportional gain for following target position, range [100,2000]
   */
  RTDE_EXPORT bool servoJ(const std::vector<double> &q, double speed, double acceleration, double time,
                          double lookahead_time, double gain);

  /**
   * @brief Servo to position (linear in tool-space)
   * @param pose target pose
   * @param speed NOT used in current version
   * @param acceleration NOT used in current version
   * @param time time where the command is controlling the robot. The function is blocking for time t [S]
   * @param lookahead_time time [S], range [0.03,0.2] smoothens the trajectory with this lookahead time
   * @param gain proportional gain for following target position, range [100,2000]
   */
  RTDE_EXPORT bool servoL(const std::vector<double> &pose, double speed, double acceleration, double time,
                          double lookahead_time, double gain);

  /**
   * Move to each waypoint specified in the given path
   * @param path The path with waypoints
   * @param @param async a bool specifying if the move command should be asynchronous.
   * If async is true it is possible to stop a move command using either the
   * stopJ or stopL function. Default is false, this means the function will
   * block until the movement has completed.
   */
  RTDE_EXPORT bool movePath(const Path &path, bool async = false);

  /**
   * @brief Stop servos
   */
  RTDE_EXPORT bool servoStop();

  /**
   * @brief Stop speeding
   */
  RTDE_EXPORT bool speedStop();

  /**
   * @brief Servo to position (circular in tool-space). Accelerates to and moves with constant tool speed v.
   * @param pose target pose
   * @param speed tool speed [m/s]
   * @param acceleration tool acceleration [m/s^2]
   * @param blend blend radius (of target pose) [m]
   */
  RTDE_EXPORT bool servoC(const std::vector<double> &pose, double speed = 0.25, double acceleration = 1.2,
                          double blend = 0.0);

  /**
   * @brief Set robot to be controlled in force mode
   * @param task_frame A pose vector that defines the force frame relative to the base frame.
   * @param selection_vector A 6d vector of 0s and 1s. 1 means that the robot will be compliant in the corresponding
   * axis of the task frame
   * @param wrench The forces/torques the robot will apply to its environment. The robot adjusts its position
   * along/about compliant axis in order to achieve the specified force/torque. Values have no effect for
   * non-compliant axes
   * @param type An integer [1;3] specifying how the robot interprets the force frame.
   * 1: The force frame is transformed in a way such that its y-axis is aligned with a vector pointing from the
   * robot tcp towards the origin of the force frame. 2: The force frame is not transformed. 3: The force frame is
   * transformed in a way such that its x-axis is the projection of the robot tcp velocity vector onto the x-y plane
   * of the force frame.
   * @param limits (Float) 6d vector. For compliant axes, these values are the maximum allowed tcp speed along/about
   * the axis. For non-compliant axes, these values are the maximum allowed deviation along/about an axis between the
   * actual tcp position and the one set by the program.
   */
  RTDE_EXPORT bool forceMode(const std::vector<double> &task_frame, const std::vector<int> &selection_vector,
                             const std::vector<double> &wrench, int type, const std::vector<double> &limits);

  /**
   * @brief Resets the robot mode from force mode to normal operation.
   */
  RTDE_EXPORT bool forceModeStop();


  /**
   * @brief Starts jogging with the given speed vector with respect to the given
   * feature.
   * When jogging has started, it is possible to provide new speed vectors by
   * calling the jogStart() function over and over again. This makes it
   * possible to use a joystick or a 3D Space Navigator to provide new speed
   * vectors if the user moves the joystick or the Space Navigator cap.
   * Switching the feature (base or tool) is only possible, if the jogging has
   * been stopped before the jogStart() function is called. That means, with
   * the first call of jogStart() the speed vector and feature parameter is
   * evaluated. With all following calls of the function only the speed vector
   * will be evaluated.
   * @param speed Speed vector for translation and rotation. Translation values
   * are given in mm / s and rotation values in rad / s.
   * @param feature Configures to move to move with respect to base frame
   * (FEATURE_BASE) or with respect to tcp frame (FEATURE_TOOL)
   */
  RTDE_EXPORT bool jogStart(const std::vector<double> &speeds, int feature = FEATURE_BASE);

  /**
   * Stops jogging that has been started start_jog
   */
  RTDE_EXPORT bool jogStop();


  /**
   * @brief Zeroes the TCP force/torque measurement from the builtin force/torque sensor by subtracting the current
   * measurement from the subsequent.
   */
  RTDE_EXPORT bool zeroFtSensor();

  /**
   * @brief Set payload
   * @param mass Mass in kilograms
   * @param cog Center of Gravity, a vector [CoGx, CoGy, CoGz] specifying the displacement (in meters) from the
   * toolmount. If not specified the current CoG will be used.
   */
  RTDE_EXPORT bool setPayload(double mass, const std::vector<double> &cog = {});

  /**
   * @brief Set robot in freedrive mode. In this mode the robot can be moved around by hand in the same way as
   * by pressing the "freedrive" button. The robot will not be able to follow a trajectory (eg. a movej) in this mode.
   */
  RTDE_EXPORT bool teachMode();

  /**
   * @brief Set robot back in normal position control mode after freedrive mode.
   */
  RTDE_EXPORT bool endTeachMode();

  /**
   * @brief Sets the damping parameter in force mode.
   * @param damping Between 0 and 1, default value is 0.005
   *
   * A value of 1 is full damping, so the robot will decellerate quickly if no force is present.
   * A value of 0 is no damping, here the robot will maintain the speed.
   *
   * The value is stored until this function is called again. Call this function
   * before force mode is entered (otherwise default value will be used).
   */
  RTDE_EXPORT bool forceModeSetDamping(double damping);

  /**
   * @brief Scales the gain in force mode.
   * @param scaling scaling parameter between 0 and 2, default is 1.
   *
   * A value larger than 1 can make force mode unstable, e.g. in case of collisions or pushing against hard surfaces.
   *
   * The value is stored until this function is called again. Call this function before force mode is entered
   * (otherwise default value will be used)
   */
  RTDE_EXPORT bool forceModeSetGainScaling(double scaling);

  /**
   * @brief Detects when a contact between the tool and an object happens.
   * @param direction The first three elements are interpreted as a 3D vector (in the robot base coordinate system)
   * giving the direction in which contacts should be detected. If all elements of the list are zero, contacts from all
   * directions are considered.
   * @returns The returned value is the number of time steps back to just before the contact have started. A value
   * larger than 0 means that a contact is detected. A value of 0 means no contact.
   */
  RTDE_EXPORT int toolContact(const std::vector<double> &direction);

  /**
   * @brief Returns the duration of the robot time step in seconds.
   *
   * In every time step, the robot controller will receive measured joint positions and velocities from the robot, and
   * send desired joint positions and velocities back to the robot. This happens with a predetermined frequency, in
   * regular intervals. This interval length is the robot time step.
   *
   * @returns Duration of the robot step in seconds
   */
  RTDE_EXPORT double getStepTime();

  /**
   * @brief Detects when a contact between the tool and an object happens.
   * @param direction The first three elements are interpreted as a 3D vector (in the robot base coordinate system)
   * giving the direction in which contacts should be detected. If all elements of the list are zero, contacts from all
   * directions are considered.
   * @returns The returned value is the number of time steps back to just before the contact have started. A value
   * larger than 0 means that a contact is detected. A value of 0 means no contact.
   */
  RTDE_EXPORT std::vector<double> getActualJointPositionsHistory(int steps = 0);

  /**
   * @brief Returns the target waypoint of the active move
   *
   * This is different from the target tcp pose which returns the target pose for each time step.
   * The get_target_waypoint() returns the same target pose for movel, movej, movep or movec during the motion. It
   * returns the target tcp pose, if none of the mentioned move functions are running.
   *
   * This method is useful for calculating relative movements where the previous move command uses blends.
   *
   * @returns The desired waypoint TCP vector [X, Y, Z, Rx, Ry, Rz]
   */
  RTDE_EXPORT std::vector<double> getTargetWaypoint();

  /**
   * @brief Sets the active tcp offset, i.e. the transformation from the output flange coordinate system to the
   * TCP as a pose.
   * @param tcp_offset A pose describing the transformation of the tcp offset.
   */
  RTDE_EXPORT bool setTcp(const std::vector<double> &tcp_offset);

  /**
   * @brief Calculate the inverse kinematic transformation (tool space -> jointspace). If qnear is defined, the
   * solution closest to qnear is returned.Otherwise, the solution closest to the current joint positions is returned.
   * If no tcp is provided the currently active tcp of the controller will be used.
   * @param x tool pose
   * @param qnear list of joint positions (Optional)
   * @param maxPositionError the maximum allowed positionerror (Optional)
   * @param maxOrientationError the maximum allowed orientationerror (Optional)
   * @returns joint positions
   */
  RTDE_EXPORT std::vector<double> getInverseKinematics(const std::vector<double> &x,
                                                       const std::vector<double> &qnear = {},
                                                       double max_position_error = 1e-10,
                                                       double max_orientation_error = 1e-10);

  /**
   * @brief Pose transformation to move with respect to a tool or w.r.t. a custom feature/frame
   * The first argument, p_from, is used to transform the second argument,
   * p_from_to, and the result is then returned. This means that the result is the resulting pose, when starting at
   * the coordinate system of p_from, and then in that coordinate system moving p_from_to.
   * This function can be seen in two different views. Either the function transforms, that is translates and rotates,
   * p_from_to by the parameters of p_from. Or the function is used to get the resulting pose, when first
   * making a move of p_from and then from there, a move of p_from_to.
   * If the poses were regarded as transformation matrices, it would look like:
   * @verbatim
   * T_world->to = T_world->from * T_from->to
   * T_x->to = T_x->from * T_from->to
   * @endverbatim
   * @param p_from starting pose (spatial vector)
   * @param p_from_to pose change relative to starting pose (spatial vector)
   * @returns resulting pose (spatial vector)
   */
  RTDE_EXPORT std::vector<double> poseTrans(const std::vector<double> &p_from, const std::vector<double> &p_from_to);

  /**
   * @brief Triggers a protective stop on the robot. Can be used for testing and debugging.
   */
  RTDE_EXPORT bool triggerProtectiveStop();

  /**
   * @brief Returns true if a program is running on the controller, otherwise it returns false
   */
  RTDE_EXPORT bool isProgramRunning();

  /**
   * @brief Enable a watchdog for the communication with a specified minimum frequency for which an input update is
   * expected to arrive. The watchdog is useful for safety critical realtime applications eg. servoing. The default
   * action taken is to shutdown the control, if the watchdog is not kicked with the minimum frequency.
   *
   * Preferably you would call this function right after the RTDEControlInterface has been constructed.
   *
   * @param min_frequency The minimum frequency an input update is expected to arrive defaults to 10Hz.
   */
  RTDE_EXPORT bool setWatchdog(double min_frequency = 10.0);

  /**
   * @brief Kicks the watchdog safeguarding the communication. Normally you would kick the watchdog in your control
   * loop. Be sure to kick it as often as specified by the minimum frequency of the watchdog.
   */
  RTDE_EXPORT bool kickWatchdog();

  /**
   * @brief Checks if the given pose is reachable and within the current safety limits of the robot. It checks
   * safety planes limits, TCP orientation deviation limits and range of the robot. If a solution is found when
   * applying the inverse kinematics to the given target TCP pose, this pose is considered reachable.
   *
   * @param pose target pose
   * @returns a bool indicating if the pose is within the safety limits.
   */
  RTDE_EXPORT bool isPoseWithinSafetyLimits(const std::vector<double> &pose);

  /**
   * @brief Checks if the given joint position is reachable and within the current safety limits of the robot. This
   * check considers joint limits (if the target pose is specified as joint positions), safety planes limits, TCP
   * orientation deviation limits and range of the robot. If a solution is found when applying the inverse kinematics
   * to the given target TCP pose, this pose is considered reachable
   *
   * @param q joint positions
   * @returns a bool indicating if the joint positions are within the safety limits.
   */
  RTDE_EXPORT bool isJointsWithinSafetyLimits(const std::vector<double> &q);

  /**
   * @brief Returns the torques of all joints
   *
   * The torque on the joints, corrected by the torque needed to move the
   * robot itself (gravity, friction, etc.), returned as a vector of length 6.
   *
   * @returns The joint torque vector in Nm: [Base, Shoulder, Elbow, Wrist1, Wrist2, Wrist3]
   */
  RTDE_EXPORT std::vector<double> getJointTorques();

  /**
   * @brief Gets the active tcp offset, i.e. the transformation from the output flange coordinate system to the TCP
   * as a pose.
   *
   * @returns the TCP offset as a pose
   */
  RTDE_EXPORT std::vector<double> getTCPOffset();


  /**
   * @brief Calculate the forward kinematic transformation (joint space -> tool
   * space) using the calibrated robot kinematics. If no joint position vector
   * is provided the current joint angles of the robot arm will be used. If no
   * tcp is provided the currently active tcp of the controller will be used.
   *
   * NOTICE! If you specify the tcp_offset you must also specify the q.
   *
   * @param q joint position vector (Optional)
   * @param tcp_offset tcp offset pose (Optional)
   * @returns the forward kinematic transformation as a pose
   */
  RTDE_EXPORT std::vector<double> getForwardKinematics(const std::vector<double> &q = {},
                                                       const std::vector<double> &tcp_offset = {});

  /**
   * @brief Checks if robot is fully at rest.
   *
   * True when the robot is fully at rest, and ready to accept higher external
   * forces and torques, such as from industrial screwdrivers.
   *
   * Note: This function will always return false in modes other than the
   * standard position mode, e.g. false in force and teach mode.
   *
   * @returns True when the robot is fully at rest. Returns False otherwise.
   */
  RTDE_EXPORT bool isSteady();



 private:
  bool setupRecipes(const double &frequency);

  void initOutputRegFuncMap();

  bool sendCommand(const RTDE::RobotCommand &cmd);

  void sendClearCommand();

  int getControlScriptState();

  bool isProtectiveStopped();

  bool isEmergencyStopped();

  int getToolContactValue();

  double getStepTimeValue();

  std::vector<double> getTargetWaypointValue();

  std::vector<double> getActualJointPositionsHistoryValue();

  std::vector<double> getInverseKinematicsValue();

  std::vector<double> poseTransValue();

  void verifyValueIsWithin(const double &value, const double &min, const double &max);

  std::string buildPathScriptCode(const std::vector<std::vector<double>> &path, const std::string &cmd);

  void receiveCallback();

  std::string outDoubleReg(int reg) const
  {
    return "output_double_register_" + std::to_string(register_offset_+reg);
  };

  std::string outIntReg(int reg) const
  {
    return "output_int_register_" + std::to_string(register_offset_+reg);
  };

  std::string inDoubleReg(int reg) const
  {
    return "input_double_register_" + std::to_string(register_offset_+reg);
  };

  std::string inIntReg(int reg) const
  {
    return "input_int_register_" + std::to_string(register_offset_+reg);
  };

  double getOutputDoubleReg(int reg)
  {
    std::string func_name = "getOutput_double_register_"+std::to_string(register_offset_+reg);
    return output_reg_func_map_[func_name]();
  };

  int getOutputIntReg(int reg)
  {
    std::string func_name = "getOutput_int_register_"+std::to_string(register_offset_+reg);
    return output_reg_func_map_[func_name]();
  };

 private:
  std::string hostname_;
  int port_;
  bool upload_script_;
  bool verbose_;
  bool use_upper_range_registers_;
  bool custom_script_running_;
  double frequency_;
  double delta_time_;
  int register_offset_;
  std::shared_ptr<RTDE> rtde_;
  std::atomic<bool> stop_thread_{false};
  std::shared_ptr<boost::thread> th_;
  std::shared_ptr<DashboardClient> db_client_;
  std::shared_ptr<ScriptClient> script_client_;
  std::shared_ptr<RobotState> robot_state_;
  std::map<std::string, std::function<double()>> output_reg_func_map_;
};

/**
 * Single path entry in a move path
 */
struct PathEntry
{
  enum eMoveType
  {
    MoveJ,
    MoveL,
    MoveP,
    MoveC
  };

  enum ePositionType
  {
    PositionTcpPose,
    PositionJoints
  };

  /**
   * Creates a waypoint for path
   */
  RTDE_EXPORT PathEntry(eMoveType move_type, ePositionType position_type, const std::vector<double> parameters)
      : move_type_(move_type), pos_type_(position_type), param_(parameters)
  {
  }

  /**
   * Returns valid script code for the given path entry
   */
  RTDE_EXPORT std::string toScriptCode() const;

  eMoveType move_type_;
  ePositionType pos_type_;
  std::vector<double> param_;
};

/**
 * A path with a number of move commands.
 * It is possible to mix different move command types in a single path
 */
class Path
{
 public:
  /**
   * Adds a single path entry to the path
   */
  RTDE_EXPORT void addEntry(const PathEntry &entry);

  /**
   * Deletes all waypoints from the path
   */
  RTDE_EXPORT void clear();

  /**
   * Returns the number of entries in the path
   */
  RTDE_EXPORT std::size_t size() const;

  /**
   * Read access to the waypoints of the path
   */
  RTDE_EXPORT const std::vector<PathEntry> &waypoints() const;

  /**
   * Adds a path with only moveL commands to this path.
   * This is here for backward compatibility to ease switch to this new
   * path implementation for the old moveL and moveJ path functions.
   */
  RTDE_EXPORT void appendMovelPath(const std::vector<std::vector<double>> &path);

  /**
   * Adds a path with moveJ commands to this path.
   * This is here for backward compatibility to ease switch to this new
   * path implementation for the old moveL and moveJ path functions.
   */
  RTDE_EXPORT void appendMovejPath(const std::vector<std::vector<double>> &path);

  /**
   * Returns valid script code for the given path
   */
  RTDE_EXPORT std::string toScriptCode() const;

 private:
  std::vector<PathEntry> waypoints_;
};

}  // namespace ur_rtde

#endif  // RTDE_CONTROL_INTERFACE_H
