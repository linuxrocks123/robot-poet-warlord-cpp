#pragma once

#include "robot_api.hpp"
#include "WorldAPI.hpp"

#include <cstdint>
#include <functional>
#include <list>
#include <vector>

using robot_api::Robot_Specs;
using robot_api::Robot_Status;

/**
 * Robot Interface:
 * Your code must implement this simple interface in order to be
 * useable by the simulator.
 */
class Robot
{
public:
     /**
      * Entry point for your robot on its creation
      * @param api a pointer to a WorldAPI object you can use to
      *            interact with the simulator, if it's not NULL
      *            (currently unused and always NULL)
      * @param skill_points the number of skill points your robot is
      *                     allowed to have.
      * @param message a 64-byte message from the robot who created you.
      *                If you were created by the simulator, the first two
      *                bytes of the message will contain your ID, which is
      *                unique among the IDs of all your team's robots
      *                created by the world.  Otherwise, the format of the
      *                message is unspecified: it's up to you to define it.
      * @return You are to return a Robot_Specs object containing the
      *         allocation of skill points you have chosen for yourself.
      */
     virtual Robot_Specs createRobot(WorldAPI* api, int skill_points, std::vector<std::uint8_t> message) = 0;

     /**
      * Each turn, this method is called to allow your robot to act.
      * @param api a reference to a WorldAPI object you can use to
      *            interact with the simulator.
      * @param status a Robot_Status object containing information about
      *               your current health and energy level
      * @param received_radio the radio signals you have received this
      *                       round.  Each message is exactly 64 bytes long.
      *                       You may be able to receive additional radio
      *                       signals by calling getMessages() with a
      *                       nonzero power if you are being jammed.
      */
     virtual void act(WorldAPI& api, Robot_Status status, std::vector<std::vector<std::uint8_t> > received_radio) = 0;
};
