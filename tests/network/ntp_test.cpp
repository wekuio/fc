//#include <boost/test/unit_test.hpp>

#include <fc/network/ntp.hpp>
#include <fc/log/logger.hpp>
#include <fc/thread/thread.hpp>

//BOOST_AUTO_TEST_SUITE(fc_network)

//BOOST_AUTO_TEST_CASE( ntp_test )
//{
int main( int argc, char** argv ) {
   ilog("start ntp test");
   fc::usleep( fc::seconds(1) );
   fc::ntp ntp_service;
   ntp_service.set_request_interval(5);
   ilog( "sleeping for 4 seconds" );
   fc::usleep(fc::seconds(4) );
   ilog( "... done sleeping for 4 seconds" );
   auto time = ntp_service.get_time();
  // BOOST_CHECK( time );
   auto ntp_time = *time;
   auto delta = ntp_time - fc::time_point::now();
   auto minutes = delta.count() / 1000000 / 60;
   auto hours = delta.count() / 1000000 / 60 / 60;
   auto seconds = delta.count() / 1000000;
   auto msec= delta.count() / 1000;
   idump((hours)(minutes)(seconds)(msec)); 

   ilog( "sleeping for 20 seconds" );
   fc::usleep(fc::seconds(20) );
   ilog( "... done sleeping for 20 seconds" );
  
  // BOOST_CHECK( msec < 100 );
   ilog("done ntp test");
}

//BOOST_AUTO_TEST_SUITE_END()
