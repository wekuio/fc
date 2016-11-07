#include <boost/test/unit_test.hpp>

#include <fc/network/http/websocket.hpp>

#include <iostream>

BOOST_AUTO_TEST_SUITE(fc_network)

BOOST_AUTO_TEST_CASE(websocket_test)
{ try {
    fc::http::websocket_client client;
    fc::http::websocket_connection_ptr s_conn, c_conn;
    {
        fc::http::websocket_server server;
        server.on_connection([&]( const fc::http::websocket_connection_ptr& c ){
                ilog( "on connection..." );
                s_conn = c;
                c->on_message_handler([&](const std::string& s){
                    ilog( "   send message ${s}", ("s",s) );
                    c->send_message("echo: " + s);
                });
        //        ilog( "sending hello from server" );
        //        c->send_message("hello from server");
            });

        ilog( "listening on port 8095" );
        server.listen( 8095 );
        server.start_accept();
       // wlog( "waiting for 5 seconds" );
       // fc::usleep( fc::seconds(5) );

        std::string echo;
        elog( "client.connect..." );

        c_conn = client.connect( "ws://localhost:8095" );

        c_conn->on_message_handler([&](const std::string& s){
                wlog( "recv" );
                    std::cerr << "receive: " << s <<"\n";
                    echo = s;
                });

        c_conn->send_message( "hello world" );
        fc::usleep( fc::seconds(1) );
        wdump((echo));
        BOOST_CHECK_EQUAL("echo: hello world", echo);
        c_conn->send_message( "again" );
        fc::usleep( fc::seconds(1) );
        BOOST_CHECK_EQUAL("echo: again", echo);

        wlog( "..............................................................\n" );
        s_conn->close(0, "test");
        fc::usleep( fc::seconds(1) );
        try {
            c_conn->send_message( "again" );
            BOOST_FAIL("expected assertion failure");
        } catch (const fc::assert_exception& e) {
            std::cerr << e.to_string() << "\n";
        }

        wlog( "..............................................................\n" );
        c_conn = client.connect( "ws://localhost:8095" );
        c_conn->on_message_handler([&](const std::string& s){
                    echo = s;
                });
        c_conn->send_message( "hello world" );
        fc::usleep( fc::seconds(1) );
        BOOST_CHECK_EQUAL("echo: hello world", echo);
        wlog( "..............................................................\n" );
    }

    try {
        c_conn->send_message( "again" );
        BOOST_FAIL("expected assertion failure");
    } catch (const fc::exception& e) {
        std::cerr << e.to_string() << "\n";
    }
        wlog( "..............................................................\n" );

    try {
        c_conn = client.connect( "ws://localhost:8095" );
        BOOST_FAIL("expected assertion failure");
    } catch (const fc::exception& e) {
        std::cerr << e.to_string() << "\n";
    }
        wlog( "..............................................................\n" );
  } catch ( const fc::exception& e ) {
     edump((e.to_detail_string()));
     throw;
  }
}

BOOST_AUTO_TEST_SUITE_END()
