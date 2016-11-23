#include <fc/asio.hpp>
#include <fc/thread/thread.hpp>
#include <boost/thread.hpp>
#include <fc/log/logger.hpp>
#include <fc/exception/exception.hpp>
#include <boost/fiber/all.hpp>
#include "thread/asio/yield.hpp"
#include "thread/asio/round_robin.hpp"

namespace fc {
  namespace asio {
    namespace detail {

      void read_write_handler::operator()(const boost::system::error_code& ec, size_t bytes_transferred)
      {
        // assert(false); // to detect anywhere we're not passing in a shared buffer
        if( !ec )
          _completion_promise->set_value(bytes_transferred);
        else if( ec == boost::asio::error::eof  )
          _completion_promise->set_exception( std::make_exception_ptr( fc::eof_exception( FC_LOG_MESSAGE( error, "${message} ", ("message", boost::system::system_error(ec).what())) ) ) );
        else
          _completion_promise->set_exception( std::make_exception_ptr( fc::exception( FC_LOG_MESSAGE( error, "${message} ", ("message", boost::system::system_error(ec).what())) ) ) );
      }

      void read_write_handler_with_buffer::operator()(const boost::system::error_code& ec, size_t bytes_transferred)
      {
        if( !ec )
          _completion_promise->set_value(bytes_transferred);
        else if( ec == boost::asio::error::eof  )
          _completion_promise->set_exception( std::make_exception_ptr( fc::eof_exception( FC_LOG_MESSAGE( error, "${message} ", ("message", boost::system::system_error(ec).what())) ) ) );
        else
          _completion_promise->set_exception( std::make_exception_ptr( fc::exception( FC_LOG_MESSAGE( error, "${message} ", ("message", boost::system::system_error(ec).what())) ) ) );
      }

      /*
        void read_write_handler_ec( promise<size_t>* p, boost::system::error_code* oec, const boost::system::error_code& ec, size_t bytes_transferred ) {
            p->set_value(bytes_transferred);
            *oec = ec;
        }
        */
        void error_handler( void_promise_ptr& p,
                              const boost::system::error_code& ec ) {
            if( !ec )
              p->set_value();
            else
            {
                if( ec == boost::asio::error::eof  )
                {
                  p->set_exception( std::make_exception_ptr( fc::eof_exception(
                          FC_LOG_MESSAGE( error, "${message} ", ("message", boost::system::system_error(ec).what())) ) ) );
                }
                else
                {
                  //elog( "${message} ", ("message", boost::system::system_error(ec).what()));
                  p->set_exception( std::make_exception_ptr( fc::exception(
                          FC_LOG_MESSAGE( error, "${message} ", ("message", boost::system::system_error(ec).what())) ) ) );
                }
            }
        }

        void error_handler_ec( error_code_promise_ptr& p,
                              const boost::system::error_code& ec ) {
            p->set_value(ec);
        }

        template<typename EndpointType, typename IteratorType>
        void resolve_handler(
                             promise<std::vector<EndpointType> >& p,
                             const boost::system::error_code& ec,
                             IteratorType itr) {
            if( !ec ) {
                std::vector<EndpointType> eps;
                while( itr != IteratorType() ) {
                    eps.push_back(*itr);
                    ++itr;
                }
                p.set_value( eps );
            } else {
                //elog( "%s", boost::system::system_error(ec).what() );
                //p->set_exception( fc::copy_exception( boost::system::system_error(ec) ) );
                p.set_exception(
                    std::make_exception_ptr( fc::exception(
                        FC_LOG_MESSAGE( error, "process exited with: ${message} ",
                                        ("message", boost::system::system_error(ec).what())) ) ) );
            }
        }
    }

    struct default_io_service_scope
    {
       boost::asio::io_service*          io;
       std::vector<boost::thread*>       asio_threads;

       //boost::asio::io_service::work*    the_work;

       default_io_service_scope()
       {
            io           = new boost::asio::io_service();
            for( int i = 0; i < 1; ++i ) {
               asio_threads.push_back( new boost::thread( [=]()
               {
                 boost::fibers::use_scheduling_algorithm< boost::fibers::asio::round_robin > ( *io );
                 fc::thread::current().set_name("asio" + fc::to_string(i));
                 boost::fibers::async( boost::fibers::launch::post, [](){ fc::thread::current().exec(); } );
                 while (!io->stopped())
                 {
                   try
                   {
                     io->run();
                   }
                   catch (const fc::exception& e)
                   {
                     elog("Caught unhandled exception in asio service loop: ${e}", ("e", e));
                   }
                   catch (const std::exception& e)
                   {
                     elog("Caught unhandled exception in asio service loop: ${e}", ("e", e.what()));
                   }
                   catch (...)
                   {
                     elog("Caught unhandled exception in asio service loop");
                   }
                 }
               }) );
            }
       }

       void cleanup()
       {
          //delete the_work;
          io->stop();
          for( auto asio_thread : asio_threads ) {
             asio_thread->join();
          }
          delete io;
          for( auto asio_thread : asio_threads ) {
             delete asio_thread;
          }
       }

       ~default_io_service_scope()
       {}
    };

    /// If cleanup is true, do not use the return value; it is a null reference
    boost::asio::io_service& default_io_service(bool cleanup) {
        static default_io_service_scope fc_asio_service[1];
        if (cleanup) {
           for( int i = 0; i < 1; ++i )
              fc_asio_service[i].cleanup();
        }
        return *fc_asio_service[0].io;
    }

    namespace tcp {
      std::vector<boost::asio::ip::tcp::endpoint> resolve( const std::string& hostname, const std::string& port)
      {
        try
        {
          resolver res( fc::asio::default_io_service() );
          promise<std::vector<boost::asio::ip::tcp::endpoint> > p;
          auto fut = p.get_future();
          res.async_resolve( boost::asio::ip::tcp::resolver::query(hostname,port),
                            boost::bind( detail::resolve_handler<boost::asio::ip::tcp::endpoint,resolver_iterator>, std::ref(p), _1, _2 ) );
          return fut.get();
        }
        FC_RETHROW_EXCEPTIONS(warn, "")
      }
    }
    namespace udp {
      std::vector<udp::endpoint> resolve( resolver& r, const std::string& hostname, const std::string& port)
      {
        try
        {
          resolver res( fc::asio::default_io_service() );
          promise<std::vector<endpoint> > p;
          auto fut = p.get_future();
          res.async_resolve( resolver::query(hostname,port),
                              boost::bind( detail::resolve_handler<endpoint,resolver_iterator>, std::ref(p), _1, _2 ) );
          return fut.get();
        }
        FC_RETHROW_EXCEPTIONS(warn, "")
      }
    }

} } // namespace fc::asio
