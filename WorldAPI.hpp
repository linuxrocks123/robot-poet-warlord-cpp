#include "robot_api.hpp"

using namespace robot_api;

/**
 * WorldAPI Interface:<br>
 * Provides callbacks into Simulator so your Robot can take actions in the
 * virtual world.<br>
 * The underlying class type is an inner class defined inside the simulator
 * which is opaque to your robot.
 */
class WorldAPI
{
public:
     /*/**********************************************
      * Movement Methods
      ***********************************************/

     /**
      * Melee attack: attack an adjacent grid cell
      * @param power Power to use for attack (may not exceed attack skill)
      * @param adjacent_cell Adjacent GridCell to attack
      * @return AttackResult indicating whether attack succeeded and/or
      *         destroyed target of attack.
      */
     virtual AttackResult meleeAttack(int power, GridCell& adjacent_cell)=0;

     /**
      * Ranged attack: attack nonadjacent grid cell within certain range
      * @param power Power to use for attack (may not exceed attack skill)
      * @param nonadjacent_cell non-adjacent GridCell to attack
      * @return AttackResult indicating whether attack succeeded and/or
      *         destroyed target of attack.
      */
     virtual AttackResult rangedAttack(int power, GridCell& nonadjacent_cell)=0;

     /**
      * Capsule attack: attack with a capsule
      * @param power_of_capsule power of the capsule to use in the attack
      * @param cell GridCell (may be adjacent or nonadjacent) to attack
      */
     virtual AttackResult capsuleAttack(int power_of_capsule, GridCell& cell)=0;

     /**
      * Defend: increase defense
      * @param power power to use for defense (may not exceed defense skill)
      */
     virtual void defend(int power)=0;

     /*/**********************************************
      * Movement Methods
      ***********************************************/

     /**
      * move: move robot
      * @param steps how far to move
      * @param way which way to move
      */
     virtual void move(int steps, Direction way)=0;

     /**
      * pick_up_capsule: pick up a capsule
      * @param adjacent_cell GridCell where capsule is that you want to pick
      *                      up (must be adjacent)
      */
     virtual void pick_up_capsule(GridCell& adjacent_cell)=0;

     /**
      * drop_capsule: drop a capsule (for an ally to pick up, presumably)
      * @param adjacent_cell where to drop capsule (must be adjacent)
      * @param power_of_capsule how powerful a capsule to drop
      */
     virtual void drop_capsule(GridCell& adjacent_cell, int power_of_capsule)=0;

     /*/**********************************************
      * Construction Methods
      ***********************************************/

     /**
      * Tells us what we're in the middle of building.
      * @return BuildStatus object indicating what we're building.
      *         Is null if we're not building anything.
      */
     virtual BuildStatus getBuildStatus()=0;

     /**@return GridCell indicating the target of our building efforts.
      *         Is null if we're not building anything.
      */
     virtual GridCell* getBuildTarget()=0;

     /**@return how much we've invested in our current build target.
      */
     virtual int getInvestedBuildPower()=0;

     /**
      * Tells the simulator the robot is beginning to build something in an
      * adjacent cell (or, for capsules, inside itself).<br>
      * In order to mark a capsule or robot as "done", call this method with
      * both parameters null.  This <i>will</i> destroy any in-progress
      * build.<br><br>
      * Note that you must not move from your current cell while in the
      * process of building anything other than a capsule.  If you do, you
      * will lose any in-progress work on a wall or fort, and a robot will
      * be automatically finalized with however many skill points you've
      * invested up to that point.
      * @param status what we're going to start building (or, in the case of
      *               an energy capsule, resume building)
      * @param location where to direct our building efforts.  Must be an
      *                 adjacent, empty location, or null if status=capsule.
      */
     virtual void setBuildTarget(BuildStatus status, GridCell* location)=0;

     /**
      * Tells the simulator the robot is beginning to build something in an
      * adjacent cell (or, for capsules, inside itself).<br>
      * In order to mark a capsule or robot as "done", call this method with
      * both parameters null.  This <i>will</i> destroy any in-progress
      * build.<br><br>
      * Note that you must not move from your current cell while in the
      * process of building anything other than a capsule.  If you do, you
      * will lose any in-progress work on a wall or fort, and a robot will
      * be automatically finalized with however many skill points you've
      * invested up to that point.
      * @param status what we're going to start building (or, in the case of
      *               an energy capsule, resume building)
      * @param location where to direct our building efforts.  Must be an
      *                 adjacent, empty location, or null if status=capsule.
      * @param creation_message message to send to newly created robot
      *                         (if we're finalizing one)
      */
     virtual void setBuildTarget(BuildStatus status, GridCell* location, vector<uint8_t> message)=0;

     /**@param power how much power to apply to building the current target.
      *              Must not be more than remaining power needed to finish
      *              building target.
      */
     virtual void build(int power)=0;

     /**
      * Spend power to repair yourself.  2 power restores 1 health.
      * @param power amount of power to spend on repairs.  Should be even.
      */
     virtual void repair(int power)=0;

     /**
      * Spend power to charge an adjacent ally robot.  1-for-1 efficiency.
      * @param power amount of power to use for charging ally
      * @param ally cell containing ally to charge.  Must be adjacent.
      */
     virtual void charge(int power, GridCell& ally)=0;

     /*/**********************************************
      * Radio Methods
      ***********************************************/

     /**
      * Spend additional power to get radio messages unavailable because of
      * jamming.
      * @param power amount of power to spend to attempt to overcome jamming
      * @return messages received after additional power spent.  Will
      *         include all messages included in simulator's call to act().
      *         May be null if no messages were received.
      */
     /*byte[][] getMessages(int power);*/ //disabled

     /**
      * Sends a message to an ally or allies.
      * @param message 64-byte array containing message to transmit
      * @param power amount of power to use for sending message
      */
     virtual void sendMessage(vector<uint8_t> message, int power)=0;

     /**
      * Gets a copy of the portion of the world visible to the robot.
      * Range is equal to defense skill.  Does not cost any power.
      * @return a 2-dimensional array containing a GridCell for each cell
      *         visible to the robot.
      */
     virtual vector<vector<GridCell> > getVisibleNeighborhood()=0;

     /**
      * Gets a copy of the entire world.  Takes 3 power, plus additional if
      * jamming is taking place (which won't be; jamming is not implemented).
      * @param power to spend attempting to get the world
      * @return a 2-dimensional array containing a GridCell for each cell in
      *         the world.  Will be null if jamming has prevented the world
      *         from being retrieved.
      */
     virtual vector<vector<GridCell> > getWorld(int power)=0;

     /**
      * Scans an enemy (or ally), retrieving information about the robot.
      * The cell scanned must be visible (within defense cells from us).<br>
      * Takes 1 power.
      * @param enemySpecs empty Robot_Specs object to be filled in
      * @param enemyStatus empty Robot_Status object to be filled in
      * @param toScan cell containing robot we want to scan
      */
     virtual void scanEnemy(Robot_Specs& enemySpecs, Robot_Status& enemyStatus, GridCell toScan)=0;

     /**
      * Jams radio.  Affects both allies and enemies.  Also affects self, so
      * it's best to call this method only after we've attempted to get
      * everything we can out of and send everything we want into the radio.
      * @param power power to use for jamming.  Must not be more than 5.
      */
     /*virtual void jamRadio(int power)=0;*/ //disabled
};
