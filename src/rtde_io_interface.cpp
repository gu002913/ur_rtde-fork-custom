#include <ur_rtde/rtde_io_interface.h>
#include <iostream>
#include <bitset>
#include <chrono>

namespace ur_rtde
{
RTDEIOInterface::RTDEIOInterface(std::string hostname, int port) : hostname_(std::move(hostname)), port_(port)
{
  rtde_ = std::make_shared<RTDE>(hostname_);
  rtde_->connect();
  rtde_->negotiateProtocolVersion();
  auto controller_version = rtde_->getControllerVersion();
  uint32_t major_version = std::get<MAJOR_VERSION>(controller_version);

  double frequency = 125;
  // If e-Series Robot set frequency to 500Hz
  if (major_version > CB3_MAJOR_VERSION)
    frequency = 500;

  // Setup output
  std::vector<std::string> state_names = {"robot_status_bits", "output_int_register_0"};
  rtde_->sendOutputSetup(state_names, frequency);

  // Setup input recipes

  // Recipe 1
  std::vector<std::string> no_cmd_input = {"input_int_register_20"};
  rtde_->sendInputSetup(no_cmd_input);

  // Recipe 2
  std::vector<std::string> set_std_digital_out_input = {"input_int_register_20", "standard_digital_output_mask",
                                                        "standard_digital_output"};
  rtde_->sendInputSetup(set_std_digital_out_input);

  // Recipe 3
  std::vector<std::string> set_tool_digital_out_input = {"input_int_register_20", "tool_digital_output_mask",
                                                         "tool_digital_output"};
  rtde_->sendInputSetup(set_tool_digital_out_input);

  // Recipe 4
  std::vector<std::string> set_speed_slider = {"input_int_register_20", "speed_slider_mask", "speed_slider_fraction"};
  rtde_->sendInputSetup(set_speed_slider);

  // Recipe 5
  std::vector<std::string> set_std_analog_output = {"input_int_register_2", "standard_analog_output_mask",
                                                    "standard_analog_output_type", "standard_analog_output_0",
                                                    "standard_analog_output_1"};
  rtde_->sendInputSetup(set_std_analog_output);

  // Init Robot state
  robot_state_ = std::make_shared<RobotState>();

  // Start RTDE data synchronization
  rtde_->sendStart();
}

RTDEIOInterface::~RTDEIOInterface()
{
  if (rtde_ != nullptr)
  {
    if (rtde_->isConnected())
      rtde_->disconnect();
  }
}

bool RTDEIOInterface::reconnect()
{
  rtde_->connect();
  rtde_->negotiateProtocolVersion();
  auto controller_version = rtde_->getControllerVersion();
  uint32_t major_version = std::get<MAJOR_VERSION>(controller_version);

  double frequency = 125;
  // If e-Series Robot set frequency to 500Hz
  if (major_version > CB3_MAJOR_VERSION)
    frequency = 500;

  // Setup output
  std::vector<std::string> state_names = {"robot_status_bits","output_int_register_0"}; //
  rtde_->sendOutputSetup(state_names, frequency);

  // Setup input recipes

  // Recipe 1
  std::vector<std::string> no_cmd_input = {"input_int_register_20"};
  rtde_->sendInputSetup(no_cmd_input);

  // Recipe 2
  std::vector<std::string> set_std_digital_out_input = {"input_int_register_20", "standard_digital_output_mask",
                                                        "standard_digital_output"};
  rtde_->sendInputSetup(set_std_digital_out_input);

  // Recipe 3
  std::vector<std::string> set_tool_digital_out_input = {"input_int_register_20", "tool_digital_output_mask",
                                                         "tool_digital_output"};
  rtde_->sendInputSetup(set_tool_digital_out_input);

  // Recipe 4
  std::vector<std::string> set_speed_slider = {"input_int_register_20", "speed_slider_mask", "speed_slider_fraction"};
  rtde_->sendInputSetup(set_speed_slider);

  // Recipe 5
  std::vector<std::string> set_std_analog_output = {"input_int_register_20", "standard_analog_output_mask",
                                                    "standard_analog_output_type", "standard_analog_output_0",
                                                    "standard_analog_output_1"};
  rtde_->sendInputSetup(set_std_analog_output);

  // Init Robot state
  robot_state_ = std::make_shared<RobotState>();

  // Start RTDE data synchronization
  rtde_->sendStart();

  // Wait for connection to be fully established before returning
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  return true;
}

void RTDEIOInterface::verifyValueIsWithin(const double &value, const double &min, const double &max)
{
  if (std::isnan(min) || std::isnan(max))
  {
    throw std::invalid_argument("Make sure both min and max are not NaN's");
  }
  else if (std::isnan(value))
  {
    throw std::invalid_argument("The value is considered NaN");
  }
  else if (!(std::isgreaterequal(value, min) && std::islessequal(value, max)))
  {
    std::ostringstream oss;
    oss << "The value is not within [" << min << ";" << max << "]";
    throw std::range_error(oss.str());
  }
}

bool RTDEIOInterface::setStandardDigitalOut(std::uint8_t output_id, bool signal_level)
{
  RTDE::RobotCommand robot_cmd;
  robot_cmd.type_ = RTDE::RobotCommand::Type::SET_STD_DIGITAL_OUT;
  robot_cmd.recipe_id_ = 2;

  if (signal_level)
  {
    robot_cmd.std_digital_out_mask_ = static_cast<uint8_t>(std::pow(2.0, output_id));
    robot_cmd.std_digital_out_ = static_cast<uint8_t>(std::pow(2.0, output_id));
  }
  else
  {
    robot_cmd.std_digital_out_mask_ = static_cast<uint8_t>(std::pow(2.0, output_id));
    robot_cmd.std_digital_out_ = 0;
  }

  return sendCommand(robot_cmd);
}

bool RTDEIOInterface::setToolDigitalOut(std::uint8_t output_id, bool signal_level)
{
  RTDE::RobotCommand robot_cmd;
  robot_cmd.type_ = RTDE::RobotCommand::Type::SET_TOOL_DIGITAL_OUT;
  robot_cmd.recipe_id_ = 3;

  if (signal_level)
  {
    robot_cmd.std_tool_out_mask_ = static_cast<uint8_t>(std::pow(2.0, output_id));
    robot_cmd.std_tool_out_ = static_cast<uint8_t>(std::pow(2.0, output_id));
  }
  else
  {
    robot_cmd.std_tool_out_mask_ = static_cast<uint8_t>(std::pow(2.0, output_id));
    robot_cmd.std_tool_out_ = 0;
  }

  return sendCommand(robot_cmd);
}

bool RTDEIOInterface::setSpeedSlider(double speed)
{
  RTDE::RobotCommand robot_cmd;
  robot_cmd.type_ = RTDE::RobotCommand::Type::SET_SPEED_SLIDER;
  robot_cmd.recipe_id_ = 4;
  robot_cmd.speed_slider_mask_ = 1;  // use speed_slider_fraction to set speed slider value
  robot_cmd.speed_slider_fraction_ = speed;
  return sendCommand(robot_cmd);
}

bool RTDEIOInterface::setAnalogOutputVoltage(std::uint8_t output_id, double voltage_ratio)
{
  RTDE::RobotCommand robot_cmd;
  robot_cmd.type_ = RTDE::RobotCommand::Type::SET_STD_ANALOG_OUT;
  robot_cmd.recipe_id_ = 5;
  robot_cmd.std_analog_output_mask_ = static_cast<uint8_t>(std::pow(2.0, output_id));
  robot_cmd.std_analog_output_type_ = 1;  // set output type to voltage
  if (output_id == 0)
    robot_cmd.std_analog_output_0_ = voltage_ratio;
  else if (output_id == 1)
    robot_cmd.std_analog_output_1_ = voltage_ratio;
  return sendCommand(robot_cmd);
}

bool RTDEIOInterface::setAnalogOutputCurrent(std::uint8_t output_id, double current_ratio)
{
  RTDE::RobotCommand robot_cmd;
  robot_cmd.type_ = RTDE::RobotCommand::Type::SET_STD_ANALOG_OUT;
  robot_cmd.recipe_id_ = 5;
  robot_cmd.std_analog_output_mask_ = static_cast<uint8_t>(std::pow(2.0, output_id));
  robot_cmd.std_analog_output_type_ = 0;  // set output type to current
  if (output_id == 0)
    robot_cmd.std_analog_output_0_ = current_ratio;
  else if (output_id == 1)
    robot_cmd.std_analog_output_1_ = current_ratio;
  return sendCommand(robot_cmd);
}

bool RTDEIOInterface::isProgramRunning()
{
  if (robot_state_ != nullptr)
  {
    // Receive RobotState
    rtde_->receiveData(robot_state_);
    // Read Bits 0-3: Is power on(1) | Is program running(2) | Is teach button pressed(4) | Is power button pressed(8)
    std::bitset<sizeof(uint32_t)> status_bits(robot_state_->getRobot_status());
    return status_bits.test(RobotStatus::ROBOT_STATUS_PROGRAM_RUNNING);
  }
  else
  {
    throw std::logic_error("Please initialize the RobotState, before using it!");
  }
}

bool RTDEIOInterface::sendCommand(const RTDE::RobotCommand &cmd)
{
  try
  {
    // Send command to the controller
    rtde_->send(cmd);
    return true;
  }
  catch (std::exception& e)
  {
    std::cout << "RTDEIOInterface: Lost connection to robot..." << std::endl;
    std::cerr << e.what() << std::endl;
    if (rtde_ != nullptr)
    {
      if (rtde_->isConnected())
        rtde_->disconnect();
    }
  }

  if(!rtde_->isConnected())
  {
    std::cout << "RTDEIOInterface: Robot is disconnected, reconnecting..." << std::endl;
    reconnect();
    sendCommand(cmd);
  }
}

}  // namespace ur_rtde