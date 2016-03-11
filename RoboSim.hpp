#pragma once

#include "robot_api.hpp"
#include "Robot.hpp"

using namespace robot_api;

/**
 * RoboSim: Main simulator logic class.
 */

#include <cmath>
#include <cstdlib>
#include <vector>
#include <list>

using std::abs;
using std::min;
using std::vector;

class RoboSim
{
public:
     
     /**FSPPredicate derivative making use of SimGridCell-specific information*/
     class SimGridAllyDeterminant
     {
     private:
          const GridCell& origin;
          
     public:
          SimGridAllyDeterminant(const GridCell& origin_) : origin(origin_) { }
          
          bool operator()(const GridCell& potential_ally);
     };

private:
     //RoboSim environmental constants
     static const int WALL_HEALTH = 10;
     static const int WALL_DEFENSE = 10;

     //RoboSim execution data (world grid, turn order, GUI reference, etc.)
     vector<vector<GridCell> > worldGrid;
     vector<RobotData> turnOrder;
     int turnOrder_pos;

public:
     /**This is so SimulatorGUI can get a copy of world*/
     vector<vector<GridCell> >& getWorldGrid() { return worldGrid; }

     /**SimulatorGUI needs to see who owns the robots in the cells
      * This is a hack to allow this by downcasting the passed GridCell
      * to SimGridCell and extracting the data.*/
     int getOccupantPlayer(const GridCell& cell) const
          {
               return cell.occupant_data->player;
          }

private:
     /**Helper method to retrieve a sanitized subgrid of the world grid
      * @param x_left left x coordinate (inclusive)
      * @param y_up smaller y coordinate (inclusive)
      * @param x_right right x coordinate (inclusive)
      * @param y_down larger y coordinate (inclusive)
      * @param player player number
      * @return subgrid of the world grid (NOT copied or sanitized)
      */
     vector<vector<GridCell> > getSanitizedSubGrid(int x_left, int y_up, int x_right, int y_down, int player) const;

     static Robot_Specs checkSpecsValid(Robot_Specs proposed, int player, int skill_points);

public:
     /**
      * Constructor for RoboSim:
      * @param initial_robots_per_combatant how many robots each team starts
      *                                     out with
      * @param skill_points skill points per combatant
      * @param length length of arena
      * @param width width of arena
      * @param obstacles number of obstacles on battlefield
      */
     RoboSim(int initial_robots_per_combatant, int skill_points, int length, int width, int obstacles);

     /**
      * The implementing class for the WorldAPI reference.
      * We can't just use ourselves for this because students
      * could downcast us to our real type and manipulate the
      * simulator in untoward ways (not that any of you would
      * do that, and I would catch it when I reviewed your
      * code anyway, but still).
      */
private:
     class RoboAPIImplementor : public WorldAPI
     {
     private:
          RoboSim& rsim;
          RobotData& actingRobot;

     public:
          /**
           * Constructor
           * @param actingRobot_ data for robot attempting to act in the simulator
           */
          RoboAPIImplementor(RoboSim& rsim_, RobotData& actingRobot_) : rsim(rsim_), actingRobot(actingRobot_) { };

          /**
           * Utility method to check whether student's robot is adjacent to cell it is attacking.
           * Note: diagonal cells are not adjacent.
           * @param adjacent_cell cell to compare with student's robot's position
           */
     private:
          bool isAdjacent(GridCell adjacent_cell)
               {
                    return (abs(actingRobot.assoc_cell->x_coord-adjacent_cell.x_coord)==1 &&
                              actingRobot.assoc_cell->y_coord == adjacent_cell.y_coord ||
                            abs(actingRobot.assoc_cell->y_coord-adjacent_cell.y_coord)==1 &&
                              actingRobot.assoc_cell->x_coord == adjacent_cell.x_coord);
               }

          /**
           * Did the attacker hit the defender?
           * @param attack attack skill of attacker (including bonuses/penalties)
           * @param defense defense skill of defender (including bonuses)
           * @return whether the attacker hit
           */
          bool calculateHit(int attack, int defense)
               {
                    int luckOfAttacker = rand()%10;
                    return luckOfAttacker+attack-defense>=5;
               }

          /**
           * Process attack, assigning damage and deleting destroyed objects if necessary.
           * @param attack attack skill of attacker (including bonuses/penalties)
           * @param cell_to_attack cell attacker is attacking containing enemy or obstacle
           * @param damage damage if attack hits
           */
          AttackResult processAttack(int attack, GridCell& cell_to_attack, int power);

     public:
          AttackResult meleeAttack(int power, GridCell& adjacent_cell);
          AttackResult rangedAttack(int power, GridCell& nonadjacent_cell);
          AttackResult capsuleAttack(int power_of_capsule, GridCell& cell);
          
          void defend(int power)
               {
                    //Error checking
                    if(power < 0 || power > actingRobot.specs.defense || power > actingRobot.specs.power || power > actingRobot.status.charge)
                         throw RoboSimExecutionException("attempted to defend with negative power",actingRobot.player, *actingRobot.assoc_cell);

                    //This one's easy
                    actingRobot.status.charge-=power;
                    actingRobot.status.defense_boost+=power;
               }

          void move(int steps, Direction way);
          void pick_up_capsule(GridCell& adjacent_cell);
          void drop_capsule(GridCell& adjacent_cell, int power_of_capsule);

          BuildStatus getBuildStatus()
               {
                    return actingRobot.whatBuilding;
               }

          GridCell* getBuildTarget()
               {
                    return actingRobot.invested_assoc_cell;
               }

          int getInvestedBuildPower()
               {
                    return actingRobot.investedPower;
               }

     private:

          void finalizeBuilding(vector<uint8_t> creation_message);

     public:

          void setBuildTarget(BuildStatus status, GridCell* location)
               {
                    setBuildTarget(status,location,vector<uint8_t>());
               }

          void setBuildTarget(BuildStatus status, GridCell* location, vector<uint8_t> message);

          void build(int power)
               {
                    if(power > actingRobot.status.power || power < 0)
                         throw RoboSimExecutionException("attempted to apply invalid power to build task",actingRobot.player,*actingRobot.assoc_cell);
                    actingRobot.status.charge-=power;
                    actingRobot.status.power-=power;
                    actingRobot.investedPower+=power;
               }

          void repair(int power)
               {
                    if(power > actingRobot.status.power || power < 0)
                         throw RoboSimExecutionException("attempted to apply invalid power to repair task",actingRobot.player,*actingRobot.assoc_cell);
                    actingRobot.status.charge-=power;
                    actingRobot.status.power-=power;
                    actingRobot.status.health+=power/2;

                    //Can't have more health than charge skill*10
                    if(actingRobot.status.health > actingRobot.specs.charge*10)
                         actingRobot.status.health = actingRobot.specs.charge*10;
               }

          void charge(int power, GridCell& ally)
               {
                    //Check that we're using a valid amount of power
                    if(power > actingRobot.status.power || power < 1)
                         throw RoboSimExecutionException("attempted charge with illegal power level",actingRobot.player,*actingRobot.assoc_cell);

                    //Are cells adjacent?
                    if(!isAdjacent(ally))
                         throw RoboSimExecutionException("attempted to charge nonadjacent cell",actingRobot.player,*actingRobot.assoc_cell);

                    //Does cell exist in grid?
                    //(could put this in isAdjacent() method but want to give students more useful error messages)
                    if(ally.x_coord > rsim.worldGrid.size() || ally.y_coord > rsim.worldGrid[0].size() || ally.x_coord < 0 || ally.y_coord < 0)
                         throw RoboSimExecutionException("passed invalid cell coordinates to charge()",actingRobot.player,*actingRobot.assoc_cell,ally);

                    //Safe to use this now, checked for oob condition from student
                    GridCell& allied_cell = rsim.worldGrid[ally.x_coord][ally.y_coord];

                    //Is there an ally in that cell?
                    if(allied_cell.contents!=SELF || allied_cell.occupant_data->player!=actingRobot.player)
                         throw RoboSimExecutionException("attempted to charge non-ally, or cell with no robot in it",actingRobot.player,*actingRobot.assoc_cell,allied_cell);

                    //Perform the charge
                    actingRobot.status.power-=power;
                    actingRobot.status.charge-=power;
                    allied_cell.occupant_data->status.charge+=power;
               }

          void sendMessage(vector<uint8_t> message, int power)
               {
                    if(power < 1 || power > 2)
                         throw RoboSimExecutionException("attempted to send message with invalid power", actingRobot.player,*actingRobot.assoc_cell);

                    if(message.size()!=64)
                         throw RoboSimExecutionException("attempted to send message byte array of incorrect length", actingRobot.player,*actingRobot.assoc_cell);

                    GridCell* target = NULL;
                    if(power==1)
                    {
                         target = RobotUtility::findNearestAlly(*actingRobot.assoc_cell,rsim.worldGrid);
                         if(target!=NULL)
                         {
                              /*There's a way to "cheat" here and set up a power-free comm channel
                               *between two allied robots.  If you can find it ... let me know, and
                               *you'll get extra credit :).  Additional credit for a bugfix.*/
                              target->occupant_data->buffered_radio.push_back(message);
                         }
                         return;
                    }
                    else //power==2
                         for(RobotData& x : rsim.turnOrder)
                              if(&x!=&actingRobot && x.player==actingRobot.player)
                                   x.buffered_radio.push_back(message);
               }

          //It's a wonderful day in the neighborhood...
          vector<vector<GridCell> > getVisibleNeighborhood()
               {
                    //YAY!  No parameters means NO ERROR CHECKING!  YAY!
                    const int range = actingRobot.specs.defense;
                    const int xloc = actingRobot.assoc_cell->x_coord;
                    const int yloc = actingRobot.assoc_cell->y_coord;
                    const int x_left = (xloc - range < 0) ? 0 : (xloc - range);
                    const int x_right = (xloc + range > rsim.worldGrid.size()-1) ? (rsim.worldGrid.size()-1) : (xloc + range);
                    const int y_up = (yloc - range < 0) ? 0 : (yloc - range);
                    const int y_down = (yloc + range > rsim.worldGrid[0].size() - 1) ? (rsim.worldGrid[0].size()-1) : (yloc + range);
                    vector<vector<GridCell> > to_return = rsim.getSanitizedSubGrid(x_left,y_up,x_right,y_down,actingRobot.player);

                    //Set associated cell to SELF instead of ALLY
                    to_return[xloc - x_left][yloc - y_up].contents=SELF;
                    return to_return;
               }

          vector<vector<GridCell> > getWorld(int power)
               {
                    if(power!=3)
                         throw RoboSimExecutionException("tried to get world with invalid power (not equal to 3)",actingRobot.player,*actingRobot.assoc_cell);

                    vector<vector<GridCell> > to_return = rsim.getSanitizedSubGrid(0,0,rsim.worldGrid.size()-1,rsim.worldGrid[0].size()-1,actingRobot.player);

                    //Set self to self instead of ally
                    to_return[actingRobot.assoc_cell->x_coord][actingRobot.assoc_cell->y_coord].contents=SELF;
                    return to_return;
               }

          void scanEnemy(Robot_Specs& enemySpecs, Robot_Status& enemyStatus, GridCell toScan)
               {
                    if(toScan.x_coord < 0 || toScan.x_coord >= rsim.worldGrid.size() || toScan.y_coord < 0 || toScan.y_coord >= rsim.worldGrid[0].size() || actingRobot.status.power==0)
                         throw RoboSimExecutionException("Invalid parameters passed to scanEnemy()",actingRobot.player,*actingRobot.assoc_cell);

                    GridCell& cell = rsim.worldGrid[toScan.x_coord][toScan.y_coord];

                    //Are we within range?
                    if(abs(actingRobot.assoc_cell->x_coord - cell.x_coord) > actingRobot.specs.defense || abs(actingRobot.assoc_cell->y_coord - cell.y_coord) > actingRobot.specs.defense)
                         throw RoboSimExecutionException("attempted to scan farther than range", actingRobot.player, *actingRobot.assoc_cell);

                    //Is there a robot in this cell?
                    if(cell.contents != SELF)
                         throw RoboSimExecutionException("attempted to scan invalid cell (no robot in cell)", actingRobot.player, *actingRobot.assoc_cell, cell);

                    //Register cost
                    actingRobot.status.power--;
                    actingRobot.status.charge--;

                    //Okay, we're good.  Fill in the data.
                    enemySpecs.attack = cell.occupant_data->specs.attack;
                    enemySpecs.defense = cell.occupant_data->specs.defense;
                    enemySpecs.power = cell.occupant_data->specs.power;
                    enemySpecs.charge = cell.occupant_data->specs.charge;
                    enemyStatus.power = cell.occupant_data->status.power;
                    enemyStatus.charge = cell.occupant_data->status.charge;
                    enemyStatus.health = cell.occupant_data->status.health;
                    enemyStatus.defense_boost = cell.occupant_data->status.defense_boost;
                    enemyStatus.capsules = cell.occupant_data->status.capsules;
               }
     };

public:
     /**
      * Executes one timestep of the simulation.
      * @return the winner, if any, or -1
      */
     int executeSingleTimeStep()
          {
               for(turnOrder_pos=0; turnOrder_pos<turnOrder.size(); turnOrder_pos++)
               {
                    //References to robot's data
                    RobotData& data = turnOrder[turnOrder_pos];
                    RoboAPIImplementor student_api(*this,data);

                    //Charge robot an amount of charge equal to charge skill
                    data.status.charge = min(data.status.charge + data.specs.charge, data.specs.charge*10);

                    /*We can spend up to status.power power this turn, but
                     *no more than our current charge level*/
                    data.status.power = min(data.specs.power, data.status.charge);

                    //Defense boost reset to zero at beginning of turn
                    data.status.defense_boost = 0;

                    //Clone status for student
                    Robot_Status clonedStatus = data.status;

                    //Run student code
                    data.robot->act(student_api,clonedStatus,data.buffered_radio);
                    data.buffered_radio.clear();
               }
               
               int player = turnOrder[0].player;
               for(int i=1; i<turnOrder.size(); i++)
                    if(turnOrder[i].player!=player)
                         return -1;
               return player;
          }
};
