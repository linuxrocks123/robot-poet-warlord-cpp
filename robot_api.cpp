#include "robot_api.hpp"
#include "Robot.hpp"
#include "RoboSim.hpp"

#include <functional>
#include <list>
#include <map>
#include <unordered_map>

using std::function;
using std::list;
using std::make_pair;
using std::multimap;
using std::unordered_map;
using std::vector;

namespace robot_api
{
     GridCell* RobotUtility::findNearestAlly(GridCell& origin, vector<vector<GridCell> >& grid)
     {
          auto path = findShortestPathInternal(origin, RoboSim::SimGridAllyDeterminant{origin}, [](const GridCell& ignored) { return true; }, grid);

          if(path.size()==0)
               return NULL;
          else
               return *(--path.end());
     }

     list<GridCell*> RobotUtility::findShortestPath(GridCell& origin, const GridCell& target, vector<vector<GridCell> >& grid)
     {
          return findShortestPathInternal(origin,[&target](const GridCell& maybeTarget)
                                          {
                                               //normal == would compare by value;
                                               //this simulates Java's "same object" test
                                               //I /think/ compare-by-value would work too here, but why risk it?
                                               //This is probably faster, anyway.
                                               return &maybeTarget==&target;
                                          },
                                          [](const GridCell& maybePassable)
                                          {
                                               return maybePassable.contents==EMPTY;
                                          }, grid);
     }

/*TODO: Next up is findShortestPathInternal and the rest of the Robot class.
 * After that, RoboSim class.
 * In RoboSim class, you'll need some sort of preprocessor macro trickery
 * to select the number of players and their classes at compile time.
 */
     list<GridCell*> RobotUtility::findShortestPathInternal(GridCell& origin, const function<bool(const GridCell&)>& isTarget, const function<bool(const GridCell&)>& isPassable, vector<vector<GridCell> >& grid)
     {
          //Offsets to handle incomplete world map
          int x_offset = grid[0][0].x_coord;
          int y_offset = grid[0][0].y_coord;
                    
          //Structures to efficiently handle accounting
          multimap<int,GridCell*> unvisited_nodes;
          unordered_map<GridCell*,list<GridCell*> > current_costs;

          //Origin's shortest path is nothing
          list<GridCell*> origin_path = {};

          //Initialize graph search with origin
          current_costs[&origin] = origin_path;
          unvisited_nodes.insert(make_pair(0,&origin));

          while(unvisited_nodes.size()!=0)
          {
               int our_cost;
               GridCell* our_cell;
               {
                    auto current_entry = unvisited_nodes.begin();
                    our_cost = current_entry->first;
                    our_cell = current_entry->second;
                    //Anonymous block because current_entry is dangling iterator after this
                    unvisited_nodes.erase(current_entry);
               }
          
               list<GridCell*>& current_path = current_costs[our_cell];

               //If we are a destination, algorithm has finished: return our current path
               if(isTarget(*our_cell))
                    return current_path;

               /*We are adjacent to up to 4 nodes, with
                 coordinates relative to ours as follows:
                 (-1,0),(1,0),(0,-1),(0,1)*/
               list<GridCell*> adjacent_nodes;
               int gridX_value = our_cell->x_coord - x_offset;
               int gridY_value = our_cell->y_coord - y_offset;
               if(gridX_value!=0)
                    adjacent_nodes.push_back(&grid[gridX_value-1][gridY_value]);
               if(gridX_value!=grid.size()-1)
                    adjacent_nodes.push_back(&grid[gridX_value+1][gridY_value]);
               if(gridY_value!=0)
                    adjacent_nodes.push_back(&grid[gridX_value][gridY_value-1]);
               if(gridY_value!=grid[0].size()-1)
                    adjacent_nodes.push_back(&grid[gridX_value][gridY_value+1]);

               /*Iterate over adjacent nodes, add to or update
                 entry in unvisited_nodes and current_costs if
                 necessary*/
               for(GridCell* x : adjacent_nodes)
               {
                    //If we're not passable, we can't be used as an adjacent cell, unless we're a target
                    if(!isPassable(*x) && !isTarget(*x))
                         continue;

                    //Generate proposed path
                    list<GridCell*> x_proposed_path = current_path;
                    x_proposed_path.push_back(x);

                    //current least cost path
                    if(current_costs.count(x))
                    {
                         list<GridCell*>& clcp = current_costs[x];
                         if(clcp.size()-1>our_cost+1)
                         {
                              auto old_range = unvisited_nodes.equal_range(clcp.size()-1);
                              for(auto i = old_range.first; i!=old_range.second; ++i)
                                   if(i->second==x)
                                   {
                                        unvisited_nodes.erase(i);
                                        break;
                                   }
                         }
                         else
                              continue;
                    }

                    unvisited_nodes.insert(make_pair(our_cost+1,x));
                    current_costs[x]=x_proposed_path;
               }
          }

          /*Loop is over, and we didn't find a path to the target :(
            Return empty path.*/
          return list<GridCell*>();
     }
}
