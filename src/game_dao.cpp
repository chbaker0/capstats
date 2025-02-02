#include <ctime>
#include <mutex>
#include "game_dao.h"
#include "game.h"
#include "include_otl.h"
#include "capstats_exceptions.h"

using namespace std;

extern otl_connect db;
extern mutex dbMutex;

void GameDAO::init() 
{
	db << "create table if not exists games (timestamp int)";
}

Game GameDAO::getGame(long gameId) const 
{
	otl_stream o((50), "select timestamp from games where rowid=:rowid<long>", db);
	o << gameId;
	long timestamp;
	if (!o.eof())
		o >> timestamp;
	else throw GameNotFoundException();

	return Game(timestamp);
}

long GameDAO::addGame(const Game &game) const
{
	lock_guard<mutex> guard(dbMutex);

	otl_stream insertStream(
		1,
		"insert into games (timestamp) values (:timestamp<long>)",
		db);
	insertStream << static_cast<long>(game.getDate());
	insertStream.flush();
	otl_stream lastRowidStream(
		1,
		"select last_insert_rowid()",
		db);
	lastRowidStream.flush();
	string rowid;
	lastRowidStream >> rowid;

	return stol(rowid);
}