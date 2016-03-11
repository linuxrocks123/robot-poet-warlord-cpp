#pragma once

#include "RoboSim.hpp"

/**
 * SimulatorGUI: Main GUI class.<br><br>
 * ASCII art for now.
 */
class SimulatorGUI
{
private:
     //Parameters for RoboSim
     int initial_robots_per_combatant;
     int skill_points;
     int length;
     int width;
     int obstacles;

     //Program State
     RoboSim current_sim;
     
public:
     SimulatorGUI(int gridX, int gridY, int skillz, int bots_per_player, int obstacles_) : length(gridX),width(gridY),skill_points(skillz),initial_robots_per_combatant(bots_per_player),obstacles(obstacles_),current_sim(initial_robots_per_combatant,skill_points,length,width,obstacles) { }
	
     int do_timestep();
};
