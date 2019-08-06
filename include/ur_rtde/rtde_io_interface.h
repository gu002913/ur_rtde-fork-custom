#ifndef RTDE_IO_INTERFACE_H
#define RTDE_IO_INTERFACE_H

#include <rtde_export.h>
#include <ur_rtde/rtde.h>
#include <thread>
#include <sstream>

#define MAJOR_VERSION 0
#define CB3_MAJOR_VERSION 3

namespace ur_rtde
{
class RTDEIOInterface
{
 public:
  RTDE_EXPORT explicit RTDEIOInterface(std::string hostname, int port = 30004);

  RTDE_EXPORT virtual ~RTDEIOInterface();

  enum RobotStatus
  {
    ROBOT_STATUS_POWER_ON = 0,
    ROBOT_STATUS_PROGRAM_RUNNING = 1,
    ROBOT_STATUS_TEACH_BUTTON_PRESSED = 2,
    ROBOT_STATUS_POWER_BUTTON_PRESSED = 3
  };

  /**
    * @returns Can be used to reconnect to the robot after a lost connection.
    */
  RTDE_EXPORT bool reconnect();

  /**
    * @brief Set standard digital output signal level
    * @param output_id The number (id) of the output, integer: [0:7]
    * @param signal_level The signal level. (boolean)
    */
  RTDE_EXPORT bool setStandardDigitalOut(std::uint8_t output_id, bool signal_level);

  /**
    * @brief Set tool digital output signal level
    * @param output_id The number (id) of the output, integer: [0:1]
    * @param signal_level The signal level. (boolean)
    */
  RTDE_EXPORT bool setToolDigitalOut(std::uint8_t output_id, bool signal_level);

  /**
    * @brief Set the speed slider on the controller
    * @param speed set the speed slider on the controller as a fraction value between 0 and 1 (1 is 100%)
    */
  RTDE_EXPORT bool setSpeedSlider(double speed);

  /**
    * @brief Set Analog output voltage
    * @param output_id The number (id) of the output, integer: [0:1]
    * @param voltage_ratio voltage set as a (ratio) of the voltage span [0..1], 1 means full voltage.
    */
  RTDE_EXPORT bool setAnalogOutputVoltage(std::uint8_t output_id, double voltage_ratio);

  /**
    * @brief Set Analog output current
    * @param output_id The number (id) of the output, integer: [0:1]
    * @param current_ratio current set as a (ratio) of the current span [0..1], 1 means full current.
    */
  RTDE_EXPORT bool setAnalogOutputCurrent(std::uint8_t output_id, double current_ratio);

  /**
    * @brief Returns true if a program is running on the controller, otherwise it returns false
    */
  RTDE_EXPORT bool isProgramRunning();

 private:
  bool sendCommand(const RTDE::RobotCommand &cmd);

  void verifyValueIsWithin(const double &value, const double &min, const double &max);

 private:
  std::string hostname_;
  int port_;
  std::shared_ptr<RTDE> rtde_;
  std::shared_ptr<RobotState> robot_state_;
};

}  // namespace ur_rtde

#endif  // RTDE_IO_INTERFACE_H
