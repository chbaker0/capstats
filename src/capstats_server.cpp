#ifdef WIN32
#include <Windows.h>
#endif

#include <memory>
#include <cstdlib>
#include <restbed>

#include <sql.h>
#include <sqltypes.h>
#include <sqlext.h>

#include <string>
#include <mutex>

#include "include_otl.h"
#include "JsonBox.h"

#include "player_dao.h"

#include "capstats_exceptions.h"

using namespace std;
using namespace restbed;
using namespace JsonBox;

otl_connect db;
mutex dbMutex;

PlayerDAO playerDAO;

long addPlayerJson(Value playerJson);
Value getPlayerJson(long id);

void post_user_handler( const shared_ptr< Session > session )
{
    const auto request = session->get_request( );

    int content_length = stoi(request->get_header( "Content-Length", "0"));

    session->fetch( content_length, [ ]( const shared_ptr< Session > session, const Bytes & body )
    {
        try
        {
            fprintf( stdout, "%.*s\n", ( int ) body.size( ), body.data( ) );

            Value player;
            player.loadFromString(string(body.begin(), body.end()));

            playerDAO.addPlayer(Player(player["telegramId"].getInteger(), player["name"].getString()));

            session->close(OK, "", { { "Content-Length", "0" },{ "Content-Type", "application/json" } });
        }
        catch (const exception& e)
        {
            cerr << e.what() << endl;
            session->close(INTERNAL_SERVER_ERROR, "", {{"Content-Length", "0"}, {"Content-Type", "text/html"}});
        }
    });
}

void get_user_handler(const shared_ptr<Session> session)
{
    try
    {
        const auto request = session->get_request();
        const unsigned int telegramId = stoi(request->get_query_parameter("telegramId", "0"));

        Player player = playerDAO.getPlayer(telegramId);

        Object playerJson;
        playerJson["name"] = Value(player.getName());
        playerJson["telegramId"] = Value((int)player.getTelegramId());

        Object out;
        out["player"] = playerJson;

        stringstream stream;
        Value(out).writeToStream(stream);
        string response_body = stream.str();

        session->close(OK, response_body, { { "Content-Length", to_string(response_body.length()) },{ "Content-Type", "application/json" } });
    }
    catch (const PlayerNotFoundException& e)
    {
        string body(e.what());
        session->close(NOT_FOUND, body, { {"Content-Length", to_string(body.size())}, {"Content-type", "text/html"}});
    }
    catch (const otl_exception& e)
    {
        cerr << e.msg << endl;
        cerr << e.stm_text << endl;
        cerr << e.var_info << endl;
    }
    catch (const exception& e)
    {
        string body;
        body = "Unexpected exception: \"";
        body += e.what();
        body += "\"";

        cerr << body << endl;

        session->close(INTERNAL_SERVER_ERROR, body, { {"Content-Length", to_string(body.size())}, {"Content-type", "text/html"} });
    }
}

void get_user_html(const shared_ptr<Session> session)
{
	const auto request = session->get_request();
	const unsigned int telegramId = stoi(request->get_query_parameter("telegramId"));

	Player player = playerDAO.getPlayer(telegramId);

	string out_html = "<h1>" + player.getName() + "</h1><p><b>Telegram ID:</b>"  + to_string(player.getTelegramId()) + "</p>";

	out_html += "<form action='player' method='post'><input type='text' name='name'><input type ='text' name='telegramId'><input type='submit'></form>";
	session->close(OK, out_html, { { "Content-Length", to_string(out_html.length()) },{ "Content-Type", "text/html" } });
}

int main( const int, const char** )
{
	// Setup db.
	otl_connect::otl_initialize();
	try {

		db << "DRIVER=SQLite3 ODBC Driver;Database=test.db;";
		playerDAO.init();
	}

	catch (otl_exception& p) { // intercept OTL exceptions
		cerr << p.msg << endl; // print out error message
		cerr << p.stm_text << endl; // print out SQL that caused the error
		cerr << p.var_info << endl; // print out the variable that caused the error
	}

	auto user = make_shared<Resource>();
	user->set_path("/player");
	user->set_method_handler("GET", get_user_handler);
	user->set_method_handler("POST", post_user_handler);
	//user->set_method_handler("GET", { {"Accept","text/html"} }, get_user_html);


    auto settings = make_shared< Settings >( );
    settings->set_port( 23232 );
    settings->set_default_header( "Connection", "close" );

    Service service;
	service.publish(user);
    service.start( settings );

    return EXIT_SUCCESS;
}

// TODO: getting errors when i try to pass via constant reference.
long addPlayerJson(Value playerJson) {
	return playerDAO.addPlayer(Player(playerJson["telegramId"].getInteger(), playerJson["name"].getString()));
}

Value getPlayerJson(long id) {
	Object playerJson;
	try {
		Player player = playerDAO.getPlayer(id);

		playerJson["name"] = Value(player.getName());
		playerJson["telegramId"] = Value((int)player.getTelegramId());
	}
	catch (PlayerNotFoundException) {

	}

	Object out;
	out["player"] = playerJson;

	return out;
}
