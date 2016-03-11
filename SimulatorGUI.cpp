#include "SimulatorGUI.hpp"
#include <cstdlib>
#include <ctime>
#include <iostream>

/*"sleep" is defined in POSIX header <unistd.h> but as usual
 * Windows is a special snowflake and does not include the
 * POSIX standard headers everyone else supports.
*/

#ifdef _WIN32

//Windows
#include <windows.h>
void sleep(unsigned int seconds) { Sleep(seconds*1000); }

#else

//All other OSes
#include <unistd.h>

#endif

using std::atoi;
using std::cout;
using std::endl;
using std::srand;
using std::time;

using robot_api::RoboSimExecutionException;

int SimulatorGUI::do_timestep()
{
     const auto& world = current_sim.getWorldGrid();
     for(const auto& row : world)
     {
          for(const auto& cell : row)
               switch(cell.contents)
               {
               case robot_api::BLOCKED: cout << 'X';
                         break;
               case robot_api::SELF: cout << current_sim.getOccupantPlayer(cell);
                    break;
               case robot_api::WALL: cout << '+';
                    break;
               case robot_api::FORT:
                    switch(cell.fort_orientation)
                    {
                    case robot_api::UP: cout << '^';
                         break;
                    case robot_api::DOWN: cout << 'v';
                         break;
                    case robot_api::LEFT: cout << '<';
                         break;
                    case robot_api::RIGHT: cout << '>';
                         break;
                    }
                    break;
               case robot_api::CAPSULE: cout << 'o';
                    break;
               case robot_api::EMPTY: cout << '*';
                    break;
               }
          cout << endl;
     }
     cout << endl;

     int ret;
     try
     {
          ret = current_sim.executeSingleTimeStep();
     }
     catch(RoboSimExecutionException e)
     {
          cout << "An error occurred during execution: " << e.msg << endl;
          ret=-1;
     }

     return ret;
}

int main(int argc, const char** argv)
{
     int x=20, y=20, skill_points=20, bots_per_player=5, obstacles=30, naptime=3;
     if(argc>=6)
     {
          x=atoi(argv[1]);
          y=atoi(argv[2]);
          skill_points=atoi(argv[3]);
          bots_per_player=atoi(argv[4]);
          obstacles=atoi(argv[5]);
     }
     if(argc==7)
          naptime=atoi(argv[6]);
     if(argc==2)
          naptime=atoi(argv[1]);

     //RNG should be seeded before simulation
     //srand(time(NULL));

     SimulatorGUI gui{x,y,skill_points,bots_per_player,obstacles};
     int winner = -1;
     try
     {
          while((winner=gui.do_timestep())==-1)
               sleep(naptime);
     }
     catch(RoboSimExecutionException e)
     {
          cout << "Failed with Robot Exception: " << e.msg << endl;
     }

     cout << "Winner: " << winner << endl;
     return 0;
}
