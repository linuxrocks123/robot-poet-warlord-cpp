#pragma once

#include "Robot.hpp"
#include <cstdlib>
#include <iostream>

//Welcome to Hackville.

#define RBP_NUM_PLAYERS 2

//#include "YourRobotName.hpp"
#include "DemoBot.hpp"
#include "DefenderBot.hpp"
//...

typedef DemoBot Player1;
typedef DefenderBot Player2;
//typedef DefenderBot Player2;
//typedef DemoBot Player3;
//typedef ManualBot Player4;

#define RBP_CALL_CONSTRUCTOR(x) rbp_construct_robot(x)

inline Robot* rbp_construct_robot(int x)
{
     switch(x)
     {
#if 1 <= RBP_NUM_PLAYERS
     case 1: return new Player1();
          break;
#endif
#if 2 <= RBP_NUM_PLAYERS
     case 2: return new Player2();
          break;
#endif
#if 3 <= RBP_NUM_PLAYERS
     case 3: return new Player3();
          break;
#endif
#if 4 <= RBP_NUM_PLAYERS
     case 4: return new Player4();
          break;
#endif
#if 5 <= RBP_NUM_PLAYERS
     case 5: return new Player5();
          break;
#endif
#if 6 <= RBP_NUM_PLAYERS
     case 6: return new Player6();
          break;
#endif
#if 7 <= RBP_NUM_PLAYERS
     case 7: return new Player7();
          break;
#endif
#if 8 <= RBP_NUM_PLAYERS
     case 8: return new Player8();
          break;
#endif
#if 9 <= RBP_NUM_PLAYERS
     case 9: return new Player9();
          break;
#endif
#if 10 <= RBP_NUM_PLAYERS
     case 10: return new Player10();
          break;
#endif
#if 11 <= RBP_NUM_PLAYERS
     case 11: return new Player11();
          break;
#endif
#if 12 <= RBP_NUM_PLAYERS
     case 12: return new Player12();
          break;
#endif
#if 13 <= RBP_NUM_PLAYERS
     case 13: return new Player13();
          break;
#endif
#if 14 <= RBP_NUM_PLAYERS
     case 14: return new Player14();
          break;
#endif
#if 15 <= RBP_NUM_PLAYERS
     case 15: return new Player15();
          break;
#endif
#if 16 <= RBP_NUM_PLAYERS
     case 16: return new Player16();
          break;
#endif
#if 17 <= RBP_NUM_PLAYERS
     case 17: return new Player17();
          break;
#endif
#if 18 <= RBP_NUM_PLAYERS
     case 18: return new Player18();
          break;
#endif
#if 19 <= RBP_NUM_PLAYERS
     case 19: return new Player19();
          break;
#endif
#if 20 <= RBP_NUM_PLAYERS
     case 20: return new Player20();
          break;
#endif
     default: std::cerr << "rbp_construct_robot was called with an invalid value.\n";
          std::cerr << "This is a fatal condition.\n";
          std::abort();
          break;
     }

     return NULL;
}
