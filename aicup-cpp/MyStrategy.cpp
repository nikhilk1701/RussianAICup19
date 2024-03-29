#include "MyStrategy.hpp"
#include <math.h>

MyStrategy::MyStrategy() {}

double distanceSqr(Vec2Double a, Vec2Double b) {
	return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
}

bool checkforobstacles(const Vec2Double &a, const Vec2Double &b, const Game &game);

UnitAction MyStrategy::getAction(const Unit &unit, const Game &game,
								 Debug &debug) {
	bool jump = true;
	bool reload = false;
	bool shoot = true;
	bool swapWeapon = false;
	bool plantMine = false;
	double velocity; 
	int health_limit = 70;

	// Locate the nearest weapon.
	const LootBox *nearestWeapon = nullptr;
	for (const LootBox &lootBox : game.lootBoxes) {
		if (std::dynamic_pointer_cast<Item::Weapon>(lootBox.item)) {
			if (nearestWeapon == nullptr ||distanceSqr(unit.position, lootBox.position) <
				distanceSqr(unit.position, nearestWeapon->position)) {
				nearestWeapon = &lootBox;
			}
		}
	}

	// Locate the nearest enemy.
	const Unit *nearestEnemy = nullptr;
	for (const Unit &other : game.units) {
		if (other.playerId != unit.playerId) {
			if (nearestEnemy == nullptr || distanceSqr(unit.position, other.position) <
			 	distanceSqr(unit.position, nearestEnemy->position)) {
				nearestEnemy = &other;
			}	
		}
	}

	// Locate nearest HealthPack.
	const LootBox *nearestHealthPack = nullptr;
	for (const LootBox &lootBox : game.lootBoxes) {
		if (std::dynamic_pointer_cast<Item::HealthPack>(lootBox.item)) {
			if (nearestHealthPack == nullptr ||distanceSqr(unit.position, lootBox.position) <
				distanceSqr(unit.position, nearestHealthPack->position)) {
				nearestHealthPack = &lootBox;
			}
		}
	}

	// Locate nearest Mine.
	// TODO:: Avoid Mines!
	const LootBox *nearestMine = nullptr;
	for (const LootBox &lootBox : game.lootBoxes) {
		if (std::dynamic_pointer_cast<Item::Mine>(lootBox.item)) {
			if (nearestMine == nullptr ||distanceSqr(unit.position, lootBox.position) <
				distanceSqr(unit.position, nearestMine->position)) {
				nearestMine = &lootBox;
			}
		}
	}

	// Fix target position.
	Vec2Double targetPos = nearestWeapon->position;
	// No weapon, then go to nearest the weapon.
	if (unit.weapon == nullptr && nearestWeapon != nullptr) {
		targetPos = nearestWeapon->position;
	} 
	// Have a weapon, but not an AR, then set swapWeapon to true.
	else if(unit.weapon != nullptr && unit.weapon->typ != WeaponType::ASSAULT_RIFLE){
		// TODO:: Swap only if you'll get a better weapon!
		swapWeapon = true;
	}
	// Why rush towards the enemy!
	// Else go to the nearest enemy.
	else if (nearestEnemy != nullptr) {
		targetPos = nearestEnemy->position;
	}

	// If health is low, go towards a HealthPack. 
	if(unit.health < health_limit){
		if(nearestHealthPack != nullptr){
			targetPos = nearestHealthPack->position;
			swapWeapon = false;
		}
	}

	debug.draw(CustomData::Log(std::string("Target pos: ") + targetPos.toString()));
	
	Vec2Double aim = Vec2Double(0, 0);
	if (nearestEnemy != nullptr) {
		aim = Vec2Double(nearestEnemy->position.x - unit.position.x, nearestEnemy->position.y - unit.position.y);
	}
	
	jump = targetPos.y > unit.position.y;
	if (targetPos.x > unit.position.x &&
		game.level.tiles[size_t(unit.position.x + 1)][size_t(unit.position.y)] ==
			Tile::WALL) {
		jump = true;
		reload = true;
	}
	if (targetPos.x < unit.position.x &&
		game.level.tiles[size_t(unit.position.x - 1)][size_t(unit.position.y)] ==
			Tile::WALL) {
		jump = true;
		reload = true;
	}
	
	// If enemy is adjacent then fallback.
	if (abs(nearestEnemy->position.x-unit.position.x) == 1 && 
			nearestEnemy->position.y == unit.position.y){
		reload = false;
		plantMine = true;
		// Decide where to go after planting the mine.
	}

	// Check for obstacles in your aim.
	shoot = checkforobstacles(unit.position, nearestEnemy->position, game);

	// Reload if you are not shooting or magazine is empty.
	if(unit.weapon != nullptr){
		if(!shoot){
			reload = true;
		}
		else if(unit.weapon->magazine == 0){
			// TODO:: Take Cover.
			// Try going to the nearest HealthKit if available.
			if(nearestHealthPack != nullptr){
				targetPos = nearestHealthPack->position;
			}
			// Else try going somewhere else.
			else if(nearestWeapon != nullptr){
				targetPos = nearestWeapon->position;
			}
		}
	}
	// Fix velocity.
	velocity = (targetPos.x > unit.position.x)*50 - (targetPos.x < unit.position.x)*50;

	UnitAction action;
	action.velocity = velocity;
	action.jump = jump;
	action.jumpDown = !action.jump;
	action.aim = aim;
	action.shoot = shoot;
	action.reload = reload;
	action.swapWeapon = swapWeapon;
	action.plantMine = plantMine;
	return action;
}

bool checkforobstacles(const Vec2Double &a, const Vec2Double &b, const Game &game){
	//vector<vector<Tile> > v = game.level.tiles;
	
	if(a.x == b.x){
		return true;
	}

	double slope = (b.y-a.y)/(b.x-a.x);

	double curr_x = a.x, curr_y = a.y;
	double change_x = 1/sqrt(1+slope*slope), change_y = abs(slope)/sqrt(1+slope*slope);
	double distance = sqrt((a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y));

	while(distance>=1){
		if(slope >= 0 && a.x > b.x){
			curr_x -= change_x;
			curr_y -= change_y;
		}
		else if(slope >= 0 && a.x < b.x){
			curr_x += change_x;
			curr_y += change_y;
		}
		else if(slope < 0 && a.x > b.x){
			curr_x -= change_x;
			curr_y += change_y;
		}
		else if(slope < 0 && a.x < b.x){
			curr_x += change_x;
			curr_y -= change_y;
		}
		if(game.level.tiles[size_t(curr_x)][size_t(curr_y)] == Tile::WALL 
		   || game.level.tiles[size_t(curr_x)][size_t(curr_y)] == Tile::PLATFORM)
			return false;
		distance -= 1;
	}
	return true;
}
