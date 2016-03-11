#pragma once

#include <functional>
#include <cstdint>
#include <list>
#include <string>
#include <vector>

class Robot;
class RoboSim;

namespace robot_api
{
     using std::uint8_t;
     using std::string;
     using std::list;
     using std::vector;
     
     /** Represents skill point allocation of Robot.*/
     struct Robot_Specs
     {
          /**Attack skill.*/
          int attack;

          /**Defense skill.*/
          int defense;

          /**Power skill.*/
          int power;

          /**Charge skill.*/
          int charge;
     };

     /**
      * Represents current status of Robot, passed to Robot.act() method
      * to let robot know its current health, charge, and capsules.
      */
     struct Robot_Status
     {
          /**remaining power for turn*/
          int power;

          /**current charge.*/
          int charge;

          /**current health.*/
          int health;

          /**current defense boost*/
          int defense_boost;

          /**
           * Current number and power of capsules.<br>
           * Each capsule is represented by an element of the list.<br>
           * The value of the element of the array is equal to the power
           * of the capsule.<br>
           * In-progress capsules are not represented;
           * use getInvestedBuildPower() for that.
           */
          list<int> capsules;
     };

     /**Represents object located in particular GridCell.*/
     enum GridObject { EMPTY, BLOCKED, SELF, ALLY, ENEMY, WALL, FORT, CAPSULE };

     /**Represents orientation in simulator's world.*/
     enum Direction { UP, DOWN, LEFT, RIGHT };

     class RobotUtility;
     class RobotData;
     
     /**Represents cell in grid of simulator's world.*/
     struct GridCell
     {
          /**X-coordinate of cell.*/
          int x_coord;

          /**Y-coordinate of cell.*/
          int y_coord;

          /**contents of cell.*/
          GridObject contents;

          /**orientation of fort, if there is a fort in the cell.*/
          Direction fort_orientation;

          /**power of capsule, if there is a capsule in the cell.*/
          int capsule_power;

     private:
          friend class ::RoboSim;
          friend class RobotUtility;

          bool has_private_members = false;
          RobotData* occupant_data;
          int wallforthealth;
     };

     /**Represents what a robot is currently building*/
     enum BuildStatus { NOTHING, /*WALL, FORT, CAPSULE,*/ ROBOT };
     
     class RobotData
     {
          friend class ::RoboSim;
          
          GridCell* assoc_cell;
          Robot_Specs specs;
          Robot_Status status;
          Robot* robot;
          int player;

          //Build information
          BuildStatus whatBuilding;
          int investedPower;
          GridCell* invested_assoc_cell;

          //Buffered radio messages
          vector<vector<uint8_t>> buffered_radio;
     };

     /**Attack type: melee, ranged, or capsule*/
     enum AttackType { MELEE, RANGED, /*CAPSULE*/ };

     /**struct representing information about a particular attack or
      * attempted attack you suffered the previous round.*/
     struct AttackNotice
     {
          /**Cell from which the attack originated.*/
          GridCell origin;

          /**Form of the attack.*/
          AttackType form;
          
          /**Amount of damage suffered from attack (0 means attack failed)*/
          int damage;
     
     };

     /**Represents result of attack: missed, hit, destroyed target*/
     enum AttackResult { MISSED, HIT, DESTROYED_TARGET };

     /**RobotUtility class<br>
      * Provides utilities for students to use in Robot implementations.<br>
      */
     class RobotUtility
     {
     public:
          /**Find nearest neighbor:<br>
           * Finds the nearest ally to cell, approximately as the crow flies.
           * @param origin cell to find
           * @param grid grid to analyze
           * @return nearest ally, or null if we have no allies in grid
           */
          static GridCell* findNearestAlly(GridCell& origin, std::vector<std::vector<GridCell> >& grid);

          /**Shortest path calculator:<br>
           * Finds the shortest path from one grid cell to another.<br><br>
           * This uses Dijkstra's algorithm to attempt to find the shortest
           * path from one grid cell to another.  Cells are adjacent if they
           * are up, down, left, or right of each other.  Cells are
           * <i>not</i> adjacent if they are diagonal to one another.<br>
           * @param origin starting grid cell
           * @param target ending grid cell
           * @param grid grid to analyze
           * @return a path guaranteed to be the shortest path from the
           *         origin to the target.  Will return null if no path
           *         could be found in the given grid.
           */
          static std::list<GridCell*> findShortestPath(GridCell& origin, const GridCell& target, std::vector<std::vector<GridCell> >& grid);
          
     private:
          static std::list<GridCell*> findShortestPathInternal(GridCell& origin, const std::function<bool(const GridCell&)>& isTarget, const std::function<bool(const GridCell&)>& isPassable, std::vector<std::vector<GridCell> >& grid);
     };

     struct RoboSimExecutionException
     {
          string msg;

          RoboSimExecutionException(const string& msg_) : msg(msg_) {}

          RoboSimExecutionException(const string& msg_, int player)
               {
                    msg = string("Player ")+std::to_string(player)+" "+msg_;
               }

          RoboSimExecutionException(const string& msg_, int player, const GridCell& cell)
               {
                    msg = string("Player ")+std::to_string(player)+" "+msg_+" with robot at coordinates ["+std::to_string(cell.x_coord)+"]["+std::to_string(cell.y_coord)+"]";
               }

          RoboSimExecutionException(const string& msg_, int player, const GridCell& cell, const GridCell& cell2)
               {
                    msg = string("Player ")+std::to_string(player)+" "+msg_+" with robot at coordinates ["+std::to_string(cell.x_coord)+"]["+std::to_string(cell.y_coord)+"], coordinates of invalid cell are ["+std::to_string(cell2.x_coord)+"]["+std::to_string(cell2.y_coord)+"]";
               }

          RoboSimExecutionException(const string& msg_, int player, int x1, int y1, int x2, int y2)
               {
                    msg = string("Player ")+std::to_string(player)+" "+msg_+" with robot at coordinates ["+std::to_string(x1)+"]["+std::to_string(y1)+"], coordinates of invalid cell are ["+std::to_string(x2)+"]["+std::to_string(y2)+"]";
               }
     };
}
