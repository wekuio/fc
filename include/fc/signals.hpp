#pragma once 

#if defined(_MSC_VER) && _MSC_VER >= 1400 
#pragma warning(push) 
#pragma warning(disable:4996) 
#endif 

#include <boost/signals2/signal.hpp>

#if defined(_MSC_VER) && _MSC_VER >= 1400 
#pragma warning(pop) 
#endif 

#include <fc/thread/thread.hpp>

namespace fc {
   template<typename T>
   using signal = boost::signals2::signal<T>;

   using scoped_connection = boost::signals2::scoped_connection;

   template<typename T>
   inline T wait( boost::signals2::signal<void(T)>& sig, const microseconds& timeout_us=microseconds::maximum() ) {
     boost::fibers::promise<T> p;
     boost::signals2::scoped_connection c( sig.connect( [&]( T t ) { p.set_value(t); } )); 
     auto f = p.get_future();
     f.wait_for( std::chrono::microseconds(timeout_us.count()) ); 
     return f.get_future().get();
   }

   inline void wait( boost::signals2::signal<void()>& sig, const microseconds& timeout_us=microseconds::maximum() ) {
     boost::fibers::promise<void> p;
     boost::signals2::scoped_connection c( sig.connect( [&]() { p.set_value(); } )); 
     auto f = p.get_future();
     f.wait_for( std::chrono::microseconds(timeout_us.count()) ); 
     f.get();
   }
} 
