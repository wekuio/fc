#pragma once
#include <fc/thread/task.hpp>

namespace fc {
  class microseconds;

  /** 
   * Starts a new OS level thread and will process tasks using cooperative multitasking until quit() is called. 
   *
   * The majority of the functionality is implemented via boost::fibers, this API simply facilitates a 
   * fiber-pool which work off tasks to improve effeciency over launching a fiber for each task.
   */
  class thread {
    public:
      thread( const std::string& name = "" );
      thread( thread&& m );
      thread& operator=(thread&& t );
      ~thread();

      /**
       *  Returns the current thread.
          Note: Creates fc::thread object (but not a boost thread) if no current thread assigned yet 
                (this can happend if current() is called from the main thread of application or from
                an existing "unknown" boost thread). In such cases, thread_d doesn't have access boost::thread object.
       */
      static thread& current();

      /**
       *  @brief returns the name given by @ref set_name() for this thread
       */
      const string& name()const;

      /**
       *  @brief associates a name with this thread.
       */
      void        set_name( const string& n );
     
      /**
       *  This method will cancel all pending tasks causing them to throw cmt::error::thread_quit.
       *
       *  If the <i>current</i> thread is not <code>this</code> thread, then the <i>current</i> thread will
       *  wait for <code>this</code> thread to exit.
       *
       *  This is a blocking wait via <code>boost::thread::join</code>
       *  and other tasks in the <i>current</i> thread will not run while 
       *  waiting for <code>this</code> thread to quit.
       *
       *  @todo make quit non-blocking of the calling thread by eliminating the call to <code>boost::thread::join</code>
       */
      void quit();
      void exec();
     
      /**
       *  @return true unless quit() has been called.
       */
      bool is_running()const;
      bool is_current()const;

      /**
       *  This method is thread safe and can be called from any any thread, it should
       *  return a boost::fibers::future<R> depending upon the return type *R* of f()
       */
      template<typename Function>
      auto async( Function&& f ) {
         if( !is_running() ) 
            throw std::runtime_error( "unable to async task on quit thread" ); 
         auto tsk = new detail::task_impl<decltype(f())>( std::forward<Function>(f) );    
         auto fut = tsk->pt.get_future();
         async_task( tsk );
         return fut;
      }


      /**
       *  This method returns a std::shared_ptr to a scheduled_task that can be used to
       *  get the result or to cancel the task.  If the task is canceled, any fibers waiting
       *  on the task will receive a fc::canceled_exception
       */
      template<typename Function>
      auto schedule( Function&& f, time_point when ) {
          auto result = std::make_shared< scheduled_task_impl< decltype(f()) > >( std::forward<Function>(f), std::ref(*this), when );
          schedule( result );
          return result;
      }

      void join();
    private:
      friend class thread_detail;
      friend void detail::thread_detail_cancel( thread&, scheduled_task* );

      thread( class thread_detail* );
      thread_detail*  my;

      void async_task( detail::task* t );
      void schedule( const std::shared_ptr<scheduled_task>& t );
  };

  template<typename Function>
  inline auto async( Function&& f ) {
     return thread::current().async( std::forward<Function>(f) );
  }

  inline void usleep( microseconds microsec ) {
     boost::this_fiber::sleep_for( std::chrono::microseconds(microsec.count()) );
  }

} // end namespace fc


