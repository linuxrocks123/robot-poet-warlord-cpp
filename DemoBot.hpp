#include "Robot.hpp"

#include <cmath>
#include <iostream>

using namespace robot_api;
using namespace std;

/**DemoBot:
 * Demo Robot implementation for OffenseBot
 */
class DemoBot : public Robot
{
private:
     Robot_Specs my_specs;

public:
     Robot_Specs createRobot(WorldAPI* api, int skill_points, vector<uint8_t> message)
          {
               Robot_Specs to_return;
               to_return.attack = to_return.defense = to_return.power = to_return.charge = skill_points/4;
               //This handles the pathological case where skill_points<4
               if(skill_points<4)
               {
                    to_return.charge = 1;
                    skill_points--;
                    if(skill_points > 0)
                    {
                         to_return.power = 1;
                         skill_points--;
                    }
                    if(skill_points > 0)
                    {
                         to_return.defense = 1;
                         skill_points--;
                    }
               }
               else
               {
                    skill_points-=(skill_points/4)*4;
                    to_return.attack += skill_points;
               }

               //Keep track of our specs; simulator won't do it for us!
               my_specs = to_return;
               return to_return;
          }

private:
     static bool isAdjacent(const GridCell& c1, const GridCell& c2)
          {
               return (abs(c1.x_coord-c2.x_coord)==1 &&
                       c1.y_coord == c2.y_coord ||
                       abs(c1.y_coord-c2.y_coord)==1 &&
                       c1.x_coord == c2.x_coord);
          }
     
     static int searchAndDestroy(GridCell& self, vector<vector<GridCell>> neighbors, WorldAPI& api,int remaining_power)
          {
               //cout << "Self: (" << self.x_coord << "," << self.y_coord << ")" << endl;
               for(int i=0; i<neighbors.size(); i++)
                    for(int j=0; j<neighbors[0].size(); j++)
                         if(neighbors[i][j].contents==ENEMY)
                         {
                              auto path = RobotUtility::findShortestPath(self,neighbors[i][j],neighbors);
                              if(path.size())
                              {
                                   auto end_pos = path.end();
                                   end_pos--;
                                   for(auto k=path.begin(); k!=end_pos && remaining_power > 0; k++)
                                   {
                                        //cout << "Move: (" << (*k)->x_coord << "," << (*k)->y_coord << ")";
                                        Direction way;
                                        if((*k)->x_coord < self.x_coord)
                                             way = LEFT;
                                        else if((*k)->x_coord > self.x_coord)
                                             way = RIGHT;
                                        else if((*k)->y_coord < self.y_coord)
                                             way = UP;
                                        else
                                             way = DOWN;
                                        api.move(1,way);
                                        remaining_power--;
                                        self = **k;
                                   }
                              }
                              if(remaining_power > 0 && isAdjacent(self,neighbors[i][j]))
                              {
                                   api.meleeAttack(remaining_power,neighbors[i][j]);
                                   remaining_power = 0;
                              }
                         }

               return remaining_power;
          }

     void act(WorldAPI& api, Robot_Status status, vector<vector<uint8_t>> received_radio)
          {
               int remaining_power = status.power;
               int remaining_charge = status.charge;

               try
               {
                    auto neighbors = api.getVisibleNeighborhood();

                    //What's our position?
                    GridCell self;
                    for(int i=0; i<neighbors.size(); i++)
                         for(int j=0; j<neighbors[0].size(); j++)
                              if(neighbors[i][j].contents==SELF)
                              {
                                   self=neighbors[i][j];
                                   break;
                              }

                    //Visible and reachable enemy?  Attack!
                    remaining_power = searchAndDestroy(self,neighbors,api,remaining_power);
                    
                    //Do we still have power?  Then we haven't attacked anything.
                    //Perhaps we couldn't find an enemy...
                    if(remaining_power > 3)
                    {
                         auto world = api.getWorld(3);
                         remaining_power-=3;
                         searchAndDestroy(self,world,api,remaining_power);
                    }
               }
               catch(RoboSimExecutionException e)
               {
                    cerr << e.msg << endl;
               }
          }
};
