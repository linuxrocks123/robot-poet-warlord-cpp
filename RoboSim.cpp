#include "RoboSim.hpp"
#include "player_config.hpp"

#include <algorithm>

using std::ceil;
using std::find;
using std::list;
using std::rand;
using std::to_string;

using namespace robot_api;

bool RoboSim::SimGridAllyDeterminant::operator()(const GridCell& potential_ally)
{
     if(&potential_ally==&origin)
          return false;

     switch(potential_ally.contents)
     {
     case ALLY:
          return true;
     case SELF:
          if(!potential_ally.has_private_members || !origin.has_private_members)
               return false;
          if(potential_ally.occupant_data!=NULL && origin.occupant_data!=NULL &&
             potential_ally.occupant_data->player==origin.occupant_data->player)
               return true;
     default:
          return false;
     }
}

vector<vector<GridCell> > RoboSim::getSanitizedSubGrid(int x_left, int y_up, int x_right, int y_down, int player) const
{
     const int x_length = x_right - x_left + 1;
     const int y_height = y_down - y_up + 1;
     
     vector<vector<GridCell> > to_return(x_length,vector<GridCell>(y_height));
     for(int i=x_left; i<=x_right; i++)
          for(int j=y_up; j<=y_down; j++)
          {
               GridCell& sanitized = to_return[i-x_left][j-y_up];
               sanitized = worldGrid[i][j];
               if(sanitized.contents==SELF)
                    if(sanitized.occupant_data->player==player)
                         sanitized.contents = ALLY;
                    else
                         sanitized.contents = ENEMY;
               sanitized.has_private_members = false;
               sanitized.occupant_data = NULL;
               sanitized.wallforthealth = 0;
          }
     
     return to_return;
}

Robot_Specs RoboSim::checkSpecsValid(Robot_Specs proposed, int player, int skill_points)
{
     if(proposed.attack + proposed.defense + proposed.power + proposed.charge != skill_points)
          throw RoboSimExecutionException("attempted to create invalid robot!",player);
     else
          return proposed;
}

RoboSim::RoboSim(int initial_robots_per_combatant, int skill_points, int length, int width, int obstacles) :
     worldGrid(length,vector<GridCell>(width)), turnOrder(RBP_NUM_PLAYERS*initial_robots_per_combatant),
     turnOrder_pos(0)
{
     for(int i=0; i<length; i++)
          for(int j=0; j<width; j++)
          {
               worldGrid[i][j].x_coord = i;
               worldGrid[i][j].y_coord = j;
               worldGrid[i][j].contents = EMPTY;
          }

     //Add robots for each combatant
     for(int player=1; player<=RBP_NUM_PLAYERS; player++)
     {
          for(int i=0; i<initial_robots_per_combatant; i++)
          {
               int x_pos,y_pos;
               do
               {
                    x_pos = rand()%length;
                    y_pos = rand()%width;
               } while(worldGrid[x_pos][y_pos].contents!=EMPTY);

               worldGrid[x_pos][y_pos].contents = SELF;
               RobotData& data = turnOrder[(player-1)*initial_robots_per_combatant+i];
               worldGrid[x_pos][y_pos].occupant_data = &data;
               data.assoc_cell = &worldGrid[x_pos][y_pos];
               data.robot = RBP_CALL_CONSTRUCTOR(player);
               data.player = player;
               vector<uint8_t> creation_message(64);
               creation_message[1] = turnOrder_pos % 256;
               creation_message[0] = turnOrder_pos / 256;
               data.specs = checkSpecsValid(data.robot->createRobot(NULL, skill_points, creation_message), player, skill_points);
               data.status.charge = data.status.health = data.specs.charge*10;

               data.status.defense_boost = 0;
               data.whatBuilding = NOTHING;
               data.investedPower = 0;
               data.invested_assoc_cell = NULL;
          }
     }

     //Add obstacles to battlefield
     for(int i=0; i<obstacles; i++)
     {
          int x_pos, y_pos;
          do
          {
               x_pos = rand()%length;
               y_pos = rand()%width;
          } while(worldGrid[x_pos][y_pos].contents!=EMPTY);
          worldGrid[x_pos][y_pos].contents = WALL;
          worldGrid[x_pos][y_pos].wallforthealth = WALL_HEALTH;
     }
}

AttackResult RoboSim::RoboAPIImplementor::processAttack(int attack, GridCell& cell_to_attack, int power)
{
     //Holds result of attack
     AttackResult to_return = MISSED;

     //Calculate defense skill of opponent
     int defense = 0;
     switch(cell_to_attack.contents)
     {
     case SELF:
          defense = cell_to_attack.occupant_data->specs.defense + cell_to_attack.occupant_data->status.defense_boost;
          break;

     case FORT:
     case WALL:
          defense = 10;
          break;
     }


     //If we hit, damage the opponent
     if(calculateHit(attack,defense))
     {
          //We hit
          to_return = HIT;

          if(cell_to_attack.occupant_data!=NULL)
          {
               //we're a robot
               for(int i=0; true; i++)
                    if(&rsim.turnOrder[i]==cell_to_attack.occupant_data)
                    {
                         if((cell_to_attack.occupant_data->status.health-=power)<=0)
                         {
                              //We destroyed the opponent!
                              to_return = DESTROYED_TARGET;

                              //Handle in-progress build, reusing setBuildTarget() to handle interruption of build due to death
                              RoboAPIImplementor(rsim,*(cell_to_attack.occupant_data)).setBuildTarget(NOTHING,cell_to_attack.occupant_data->invested_assoc_cell);

                              //Handle cell
                              cell_to_attack.occupant_data = NULL;
                              cell_to_attack.contents = cell_to_attack.wallforthealth>0 ? FORT : EMPTY;

                              //Handle turnOrder position
                              rsim.turnOrder.erase(rsim.turnOrder.begin()+i);
                              if(i<rsim.turnOrder_pos)
                                   rsim.turnOrder_pos--;
                         }
                         break;
                    }
          }
          else
               if((cell_to_attack.wallforthealth-=power)<=0)
               {
                    //We destroyed the target!
                    to_return = DESTROYED_TARGET;

                    cell_to_attack.wallforthealth = 0;
                    cell_to_attack.contents = EMPTY;
               }
     }

     return to_return;
}

AttackResult RoboSim::RoboAPIImplementor::meleeAttack(int power, GridCell& adjacent_cell)
{
     //Lots of error checking here (as everywhere...)

     //Check that we're using a valid amount of power
     if(power > actingRobot.status.power || power > actingRobot.specs.attack || power < 1)
          throw RoboSimExecutionException("attempted melee attack with illegal power level",actingRobot.player,*actingRobot.assoc_cell);

     //Are cells adjacent?
     if(!isAdjacent(adjacent_cell))
          throw RoboSimExecutionException("attempted to melee attack nonadjacent cell",actingRobot.player,*actingRobot.assoc_cell);

     //Does cell exist in grid?
     //(could put this in isAdjacent() method but want to give students more useful error messages)
     if(adjacent_cell.x_coord > rsim.worldGrid.size() || adjacent_cell.y_coord > rsim.worldGrid[0].size() ||
        adjacent_cell.x_coord < 0 || adjacent_cell.y_coord < 0)
          throw RoboSimExecutionException("passed invalid cell coordinates to meleeAttack()",actingRobot.player,*actingRobot.assoc_cell,adjacent_cell);

     //Safe to use this now, checked for oob condition from student
     GridCell& cell_to_attack = rsim.worldGrid[adjacent_cell.x_coord][adjacent_cell.y_coord];

     //Is there an enemy, fort, or wall at the cell's location?
     switch(cell_to_attack.contents)
     {
     case EMPTY:
          throw RoboSimExecutionException("attempted to attack empty cell",actingRobot.player,*actingRobot.assoc_cell,cell_to_attack);
     case BLOCKED:
          throw RoboSimExecutionException("attempted to attack blocked tile",actingRobot.player,*actingRobot.assoc_cell,cell_to_attack);
     case SELF:
          if(cell_to_attack.occupant_data->player==actingRobot.player)
               throw RoboSimExecutionException("attempted to attack ally",actingRobot.player,*actingRobot.assoc_cell,cell_to_attack);
          break;
     case CAPSULE:
          throw RoboSimExecutionException("attempted to attack energy capsule",actingRobot.player,*actingRobot.assoc_cell,cell_to_attack);
     case ALLY:
          throw RoboSimExecutionException("ERROR in RoboSim.RoboAPIImplementor.meleeAttack().  This is probably not the student's fault.  Contact Patrick Simmons about this message.  (Not the Doobie Brother...)");
     }
                    
     //Okay, if we haven't thrown an exception, the cell is valid to attack.  Perform the attack.
     //Update this robot's charge status and power status.
     actingRobot.status.charge-=power;
     actingRobot.status.power-=power;

     //Begin calculation of our attack power
     int raw_attack = actingRobot.specs.attack;

     //If we're outside a fort attacking someone in the fort, range penalty applies
     if(cell_to_attack.wallforthealth > 0 && cell_to_attack.occupant_data!=NULL)
          raw_attack/=2;

     //Attack adds power of attack to raw skill
     int attack = raw_attack + power;

     //Process attack
     return processAttack(attack,cell_to_attack,power);
}

AttackResult RoboSim::RoboAPIImplementor::rangedAttack(int power, GridCell& nonadjacent_cell)
{
     //Lots of error checking here (as everywhere...)

     //Check that we're using a valid amount of power
     if(power > actingRobot.status.power || power > actingRobot.specs.attack || power < 1)
          throw RoboSimExecutionException("attempted ranged attack with illegal power level",actingRobot.player,*actingRobot.assoc_cell);

     //Does cell exist in grid?
     //(could put this in isAdjacent() method but want to give students more useful error messages)
     if(nonadjacent_cell.x_coord > rsim.worldGrid.size() || nonadjacent_cell.y_coord > rsim.worldGrid[0].size() ||
        nonadjacent_cell.x_coord < 0 || nonadjacent_cell.y_coord < 0)
          throw RoboSimExecutionException("passed invalid cell coordinates to rangedAttack()",actingRobot.player,*actingRobot.assoc_cell,nonadjacent_cell);

     //Are cells nonadjacent?
     if(isAdjacent(nonadjacent_cell))
          throw RoboSimExecutionException("attempted to range attack adjacent cell",actingRobot.player,*actingRobot.assoc_cell);

     //Safe to use this now, checked for oob condition from student
     GridCell& cell_to_attack = rsim.worldGrid[nonadjacent_cell.x_coord][nonadjacent_cell.y_coord];

     //Do we have a "clear shot"?
     list<GridCell*> shortest_path = robot_api::RobotUtility::findShortestPath(*actingRobot.assoc_cell,cell_to_attack,rsim.worldGrid);
     if(!shortest_path.size()) //we don't have a clear shot
          throw RoboSimExecutionException("attempted to range attack cell with no clear path",actingRobot.player,*actingRobot.assoc_cell,cell_to_attack);
     else if(shortest_path.size()>actingRobot.specs.defense) //out of range
          throw RoboSimExecutionException("attempted to range attack cell more than (defense) tiles away",actingRobot.player,*actingRobot.assoc_cell,cell_to_attack);

     //Is there an enemy, fort, or wall at the cell's location?
     switch(cell_to_attack.contents)
     {
     case EMPTY:
          throw RoboSimExecutionException("attempted to attack empty cell",actingRobot.player,*actingRobot.assoc_cell,cell_to_attack);
     case BLOCKED:
          throw RoboSimExecutionException("attempted to attack blocked tile",actingRobot.player,*actingRobot.assoc_cell,cell_to_attack);
     case SELF:
          if(cell_to_attack.occupant_data->player==actingRobot.player)
               throw RoboSimExecutionException("attempted to attack ally",actingRobot.player,*actingRobot.assoc_cell,cell_to_attack);
          break;
     case CAPSULE:
          throw RoboSimExecutionException("attempted to attack energy capsule",actingRobot.player,*actingRobot.assoc_cell,cell_to_attack);
     case ALLY:
          throw RoboSimExecutionException("ERROR in RoboSim.RoboAPIImplementor.rangedAttack().  This is probably not the student's fault.  Contact Patrick Simmons about this message.  (Not the Doobie Brother...)");
     }

     //Okay, if we haven't thrown an exception, the cell is valid to attack.  Perform the attack.
     //Update this robot's charge status.
     actingRobot.status.charge-=power;
     actingRobot.status.power-=power;

     //Begin calculation of our attack power
     int raw_attack = actingRobot.specs.attack/2;

     //Attack adds power of attack to raw skill
     int attack = raw_attack + power;

     //Process attack
     return processAttack(attack,cell_to_attack,power);
}

AttackResult RoboSim::RoboAPIImplementor::capsuleAttack(int power_of_capsule, GridCell& cell)
{
     //Error checking, *sigh*...

     //Does cell exist in grid?
     if(cell.x_coord > rsim.worldGrid.size() || cell.y_coord > rsim.worldGrid[0].size() || cell.x_coord < 0 || cell.y_coord < 0)
          throw RoboSimExecutionException("passed invalid cell coordinates to capsuleAttack()",actingRobot.player,*actingRobot.assoc_cell,cell);

     //Cell to attack
     GridCell& cell_to_attack = rsim.worldGrid[cell.x_coord][cell.y_coord];

     //Do we have a capsule of this power rating?
     auto capsule_it = find(actingRobot.status.capsules.begin(),actingRobot.status.capsules.end(),power_of_capsule);

     if(capsule_it==actingRobot.status.capsules.end())
          throw RoboSimExecutionException(string("passed invalid power to capsuleAttack(): doesn't have capsule of power ")+to_string(power_of_capsule),actingRobot.player,*actingRobot.assoc_cell);

     //Can we use this capsule? (attack + defense >= power)
     if(actingRobot.specs.attack + actingRobot.specs.defense < power_of_capsule)
          throw RoboSimExecutionException("attempted to use capsule of greater power than attack+defense",actingRobot.player,*actingRobot.assoc_cell);

     //Can we hit the target?  Range is power of capsule + defense.
     list<GridCell*> shortest_path = robot_api::RobotUtility::findShortestPath(*actingRobot.assoc_cell,cell_to_attack,rsim.worldGrid);

     if(!shortest_path.size())
          throw RoboSimExecutionException("no clear shot to target",actingRobot.player,*actingRobot.assoc_cell,cell_to_attack);

     if(shortest_path.size() > power_of_capsule + actingRobot.specs.defense)
          throw RoboSimExecutionException("target not in range",actingRobot.player,*actingRobot.assoc_cell,cell_to_attack);

     //Is there an enemy, fort, or wall at the cell's location?
     switch(cell_to_attack.contents)
     {
     case EMPTY:
          throw RoboSimExecutionException("attempted to attack empty cell",actingRobot.player,*actingRobot.assoc_cell,cell_to_attack);
     case BLOCKED:
          throw RoboSimExecutionException("attempted to attack blocked tile",actingRobot.player,*actingRobot.assoc_cell,cell_to_attack);
     case SELF:
          if(cell_to_attack.occupant_data->player==actingRobot.player)
               throw RoboSimExecutionException("attempted to attack ally",actingRobot.player,*actingRobot.assoc_cell,cell_to_attack);
          break;
     case CAPSULE:
          throw RoboSimExecutionException("attempted to attack energy capsule",actingRobot.player,*actingRobot.assoc_cell,cell_to_attack);
     case ALLY:
          throw RoboSimExecutionException("ERROR in RoboSim.RoboAPIImplementor.capsuleAttack().  This is probably not the student's fault.  Contact Patrick Simmons about this message.  (Not the Doobie Brother...)");
     }

     /*Okay, if we're still here, we can use the capsule.
       Need to delete capsule from robot status structure.
     */
     actingRobot.status.capsules.erase(capsule_it);

     //Process attack
     return processAttack(actingRobot.specs.attack + power_of_capsule,cell_to_attack,(int)(ceil(0.1 * power_of_capsule * actingRobot.specs.attack)));
}

void RoboSim::RoboAPIImplementor::move(int steps, Direction way)
{
     if(steps<1)
          return;

     int x_coord = actingRobot.assoc_cell->x_coord;
     const int actor_x = x_coord;
     int y_coord = actingRobot.assoc_cell->y_coord;
     const int actor_y = y_coord;
     switch(way)
     {
     case UP:
          y_coord-=steps;
          break;
     case DOWN:
          y_coord+=steps;
          break;
     case LEFT:
          x_coord-=steps;
          break;
     case RIGHT:
          x_coord+=steps;
          break;
     }

     //Is our destination in the map?
     if(x_coord < 0 || x_coord > rsim.worldGrid.size() || y_coord < 0 || y_coord > rsim.worldGrid[0].size())
          throw RoboSimExecutionException("attempted to move out of bounds",actingRobot.player,*actingRobot.assoc_cell);

     //Is our destination empty?
     if(rsim.worldGrid[x_coord][y_coord].contents!=EMPTY && rsim.worldGrid[x_coord][y_coord].contents!=FORT)
          throw RoboSimExecutionException("attempted to move onto illegal cell",actingRobot.player,*actingRobot.assoc_cell,rsim.worldGrid[x_coord][y_coord]);

     //Are we approaching the fort from the right angle?
     if(rsim.worldGrid[x_coord][y_coord].contents==FORT && rsim.worldGrid[x_coord][y_coord].fort_orientation!=way)
          throw RoboSimExecutionException("attempted to move onto a fort from an illegal direction",actingRobot.player,*actingRobot.assoc_cell,rsim.worldGrid[x_coord][y_coord]);

     //Okay, now we have to make sure each step is empty
     const bool x_left = x_coord<actor_x;
     const bool y_left = y_coord<actor_y;
     if(x_coord!=actor_x)
     {
          for(int i=(x_left ? actor_x-1 : actor_x+1); i!=x_coord; i=(x_left ? i-1 : i+1))
               if(rsim.worldGrid[i][y_coord].contents!=EMPTY)
                    throw RoboSimExecutionException("attempted to cross illegal cell",actingRobot.player,*actingRobot.assoc_cell,rsim.worldGrid[i][y_coord]);
     }
     else
     {
          for(int i=(y_left ? actor_y-1 : actor_y+1); i!=y_coord; i=(y_left ? i-1 : i+1))
               if(rsim.worldGrid[x_coord][i].contents!=EMPTY)
                    throw RoboSimExecutionException("attempted to cross illegal cell",actingRobot.player,*actingRobot.assoc_cell,rsim.worldGrid[x_coord][i]);
     }

     //Okay, now: do we have enough power/charge?
     if(steps > actingRobot.status.power)
          throw RoboSimExecutionException("attempted to move too far (not enough power)",actingRobot.player,*actingRobot.assoc_cell,rsim.worldGrid[x_coord][y_coord]);

     //Account for power cost
     actingRobot.status.power-=steps;
     actingRobot.status.charge-=steps;

     //Change position of robot.
     actingRobot.assoc_cell->contents = EMPTY;
     actingRobot.assoc_cell->occupant_data = NULL;
     actingRobot.assoc_cell = &rsim.worldGrid[x_coord][y_coord];
     actingRobot.assoc_cell->contents = SELF;
     actingRobot.assoc_cell->occupant_data = &actingRobot;
}

void RoboSim::RoboAPIImplementor::pick_up_capsule(GridCell& adjacent_cell)
{
     //Error checking, *sigh*...

     //Does cell exist in grid?
     if(adjacent_cell.x_coord > rsim.worldGrid.size() || adjacent_cell.y_coord > rsim.worldGrid[0].size() || adjacent_cell.x_coord < 0 || adjacent_cell.y_coord < 0)
          throw RoboSimExecutionException("passed invalid cell coordinates to pick_up_capsule()",actingRobot.player,*actingRobot.assoc_cell,adjacent_cell);

     //Cell in question
     GridCell& gridCell = rsim.worldGrid[adjacent_cell.x_coord][adjacent_cell.y_coord];

     //Cell must be adjacent
     if(!isAdjacent(adjacent_cell))
          throw RoboSimExecutionException("attempted to pick up capsule in nonadjacent cell",actingRobot.player,*actingRobot.assoc_cell,gridCell);

     //We need at least one power.
     if(actingRobot.status.power==0)
          throw RoboSimExecutionException("attempted to pick up capsule with no power",actingRobot.player,*actingRobot.assoc_cell,gridCell);

     //Is there actually a capsule there?
     if(gridCell.contents!=CAPSULE)
          throw RoboSimExecutionException("attempted to pick up capsule from cell with no capsule",actingRobot.player,*actingRobot.assoc_cell,gridCell);

     //Do we have "room" for this capsule?
     if(actingRobot.status.capsules.size()+1>actingRobot.specs.attack+actingRobot.specs.defense)
          throw RoboSimExecutionException("attempted to pick up too many capsules",actingRobot.player,*actingRobot.assoc_cell,gridCell);

     //If still here, yes.

     //Decrement our power
     actingRobot.status.power--;

     //Put capsule in our inventory, delete it from world
     actingRobot.status.capsules.push_back(gridCell.capsule_power);
     gridCell.contents = EMPTY;
     gridCell.capsule_power = 0;
}

void RoboSim::RoboAPIImplementor::drop_capsule(GridCell& adjacent_cell, int power_of_capsule)
{
     //Error checking, *sigh*...
     //Does cell exist in grid?
     if(adjacent_cell.x_coord > rsim.worldGrid.size() || adjacent_cell.y_coord > rsim.worldGrid[0].size() || adjacent_cell.x_coord < 0 || adjacent_cell.y_coord < 0)
          throw RoboSimExecutionException("passed invalid cell coordinates to pick_up_capsule()",actingRobot.player,*actingRobot.assoc_cell,adjacent_cell);

     //Cell in question
     GridCell& gridCell = rsim.worldGrid[adjacent_cell.x_coord][adjacent_cell.y_coord];

     //Cell must be adjacent
     if(!isAdjacent(adjacent_cell))
          throw RoboSimExecutionException("attempted to pick up capsule in nonadjacent cell",actingRobot.player,*actingRobot.assoc_cell,gridCell);

     //Is the cell empty?
     if(gridCell.contents!=EMPTY)
          throw RoboSimExecutionException("attempted to place capsule in nonempty cell",actingRobot.player,*actingRobot.assoc_cell,gridCell);

     //Do we have such a capsule?
     auto it = find(actingRobot.status.capsules.begin(),actingRobot.status.capsules.end(),power_of_capsule);
     if(it==actingRobot.status.capsules.end())
          throw RoboSimExecutionException(string("attempted to drop capsule with power ")+to_string(power_of_capsule)+", having no such capsule",actingRobot.player,*actingRobot.assoc_cell,gridCell);

     //Okay.  We're good.  Drop the capsule
     gridCell.contents = CAPSULE;
     gridCell.capsule_power = power_of_capsule;

     //Delete it from our inventory
     actingRobot.status.capsules.erase(it);
}

void RoboSim::RoboAPIImplementor::finalizeBuilding(vector<uint8_t> creation_message)
{
     //Nothing to finalize if not building anything
     if(actingRobot.whatBuilding==NOTHING)
          return;

     //What do we have to finalize?
     int capsule_power = actingRobot.investedPower/10;
     switch(actingRobot.whatBuilding)
     {
     case WALL:
          if(actingRobot.investedPower >= 50)
          {
               actingRobot.invested_assoc_cell->contents = WALL;
               actingRobot.invested_assoc_cell->wallforthealth = WALL_HEALTH;
          }
          else
               actingRobot.invested_assoc_cell->contents = EMPTY;
          break;

     case FORT:
          if(actingRobot.investedPower >= 75)
          {
               actingRobot.invested_assoc_cell->contents = FORT;
               actingRobot.invested_assoc_cell->wallforthealth = WALL_HEALTH;
          }
          else
               actingRobot.invested_assoc_cell->contents = EMPTY;
          break;

     case CAPSULE:
          if(capsule_power!=0)
          {
               if(actingRobot.status.capsules.size()+1>actingRobot.specs.attack+actingRobot.specs.defense)
                    throw RoboSimExecutionException("attempted to finish building capsule when already at max capsule capacity",actingRobot.player,*actingRobot.assoc_cell);
               actingRobot.status.capsules.push_back(capsule_power);
          }
          break;

     case ROBOT:
          int skill_points = actingRobot.investedPower/20;
          if(skill_points!=0)
          {
               //Check creation message correct size
               if(creation_message.size()!=0 && creation_message.size()!=64)
                    throw RoboSimExecutionException("passed incorrect sized creation message to setBuildTarget()",actingRobot.player,*actingRobot.assoc_cell,*actingRobot.invested_assoc_cell);

               //Set default creation message if we don't have one
               if(!creation_message.size())
               {
                    creation_message.resize(64);
                    creation_message[1] = rsim.turnOrder.size() % 256;
                    creation_message[0] = rsim.turnOrder.size() / 256;
               }

               //Create the robot
               actingRobot.invested_assoc_cell->contents = SELF;
               Robot* robot;
               try
               {
                    robot = RBP_CALL_CONSTRUCTOR(actingRobot.player);
               }
               catch(...)
               {
                    throw RoboSimExecutionException("something went wrong calling student's constructor", actingRobot.player,*actingRobot.assoc_cell, *actingRobot.invested_assoc_cell);
               }
               rsim.turnOrder.emplace_back();
               RobotData& data = *(rsim.turnOrder.rbegin());
               data.robot = robot;
               data.assoc_cell = actingRobot.invested_assoc_cell;
               actingRobot.invested_assoc_cell->occupant_data = &data;
               data.player = actingRobot.player;
               data.specs = checkSpecsValid(data.robot->createRobot(NULL, skill_points, creation_message), actingRobot.player, skill_points);
               data.status.charge = data.status.health = data.specs.power*10;
          }
          else
               actingRobot.invested_assoc_cell->contents = EMPTY;
          break;                              
     }
}

void RoboSim::RoboAPIImplementor::setBuildTarget(BuildStatus status, GridCell* location, vector<uint8_t> message)
{
     //If we're in the middle of building something, finalize it.
     if(actingRobot.whatBuilding!=NOTHING)
          finalizeBuilding(message);

     //Error checking, *sigh*...

     //Does cell exist in grid?
     if(location!=NULL && (location->x_coord > rsim.worldGrid.size() || location->y_coord > rsim.worldGrid[0].size() || location->x_coord < 0 || location->y_coord < 0))
          throw RoboSimExecutionException("passed invalid cell coordinates to setBuildTarget()",actingRobot.player,*actingRobot.assoc_cell,*location);

     //Cell in question
     GridCell* gridCell = (location!=NULL ? &rsim.worldGrid[location->x_coord][location->y_coord] : NULL);

     //Update status
     actingRobot.whatBuilding = status;
     actingRobot.investedPower = 0;                    
     actingRobot.invested_assoc_cell = gridCell;

     //CAN pass us null, so special-case it
     if(location==NULL)
     {
          //We must be building capsule, then.
          if(actingRobot.whatBuilding!=NOTHING && actingRobot.whatBuilding!=CAPSULE)
               throw RoboSimExecutionException("passed null to setBuildTarget() location with non-null and non-capsule build target",actingRobot.player,*actingRobot.assoc_cell);
          return;
     }

     //If location NOT null, must not be building capsule
     if(status == NOTHING || status == CAPSULE)
          throw RoboSimExecutionException("attempted to target capsule or null building on non-null adjacent cell",actingRobot.player,*actingRobot.assoc_cell,*location);

     //Cell must be adjacent
     if(!isAdjacent(*location))
          throw RoboSimExecutionException("attempted to set build target to nonadjacent cell",actingRobot.player,*actingRobot.assoc_cell,*gridCell);

     //Is the cell empty?
     if(gridCell->contents!=EMPTY)
          throw RoboSimExecutionException("attempted to set build target to nonempty cell",actingRobot.player,*actingRobot.assoc_cell,*gridCell);

     //Okay, block off cell since we're building there now.
     gridCell->contents = BLOCKED;
}
