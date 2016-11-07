#pragma once
#include <boost/fiber/all.hpp>
#include <fc/time.hpp>
#include <fc/exception/exception.hpp>

namespace fc {
  class thread_detail;
  class scheduled_task;
  class thread;

  namespace detail {
     void thread_detail_cancel( thread&, scheduled_task* );

     struct task {
         virtual ~task(){ }
         virtual void exec() noexcept = 0;
         task*  next = nullptr;
     };

     template<typename R>
     struct task_impl : public task {
         template<typename Function>
         task_impl( Function&& f ):pt( std::forward<Function>(f) ){}

         virtual void exec() noexcept override { 
            try { /// pt could throw future_error with future_errc::no_state
              pt(); 
            } catch ( ... ){}
         }
         boost::fibers::packaged_task<R()> pt;
     };
  } // namespace detail

  class scheduled_task {
     public:
        scheduled_task( thread& th, time_point t ):executed(false),scheduled_thread(th),scheduled_time(t){}

        virtual ~scheduled_task(){ }

        /**
         *  True if cancel succuss, false if the task already executed
         */
        virtual bool cancel() = 0;


        time_point get_scheduled_time()const { return scheduled_time; }
     protected:
        std::atomic<bool> executed;
        thread&           scheduled_thread;
     private:
        friend class thread_detail;
        friend class thread;

        virtual void exec() noexcept = 0;
        time_point        scheduled_time;
  };

  template<typename R>
  struct scheduled_task_impl : public scheduled_task {
    public:
      template<typename Function>
      scheduled_task_impl( Function&& f, thread& sch_th, time_point sch_time )
      :scheduled_task(sch_th, sch_time),action( std::forward<Function>(f) ){}

      virtual bool cancel() override {
          bool did_execute = executed.exchange(true, std::memory_order_seq_cst);
          if( !did_execute ) {
             prom.set_exception( std::make_exception_ptr( fc::canceled_exception() ) );
             detail::thread_detail_cancel( scheduled_thread, this );
          }
          return !did_execute;
      }

      auto get_future() { return prom.get_future(); }
    private:
      virtual void exec() noexcept override { 
         auto did_execute = executed.exchange(true, std::memory_order_seq_cst);
         if( !did_execute ) {
            try {
               prom.set_value( action() );   
            } catch ( ... ) {
               prom.set_exception( std::current_exception() );
            }
         }
      }
      boost::fibers::promise<R> prom;
      std::function<R()>        action;
  };

  template<>
  class scheduled_task_impl<void> : public scheduled_task {
     public:
       template<typename Function>
       scheduled_task_impl( Function&& f, thread& th, time_point sch_time )
       :scheduled_task(th,sch_time),action( std::forward<Function>(f) ){}

       virtual bool cancel() override {
           bool did_execute = executed.exchange(true, std::memory_order_seq_cst);
           if( !did_execute ) {
              prom.set_exception( std::make_exception_ptr( fc::canceled_exception() ) );
              detail::thread_detail_cancel( scheduled_thread, this );
           }
           return !did_execute;
       }

       auto get_future() { return prom.get_future(); }
     private:
       virtual void exec() noexcept override { 
          auto did_execute = executed.exchange(true, std::memory_order_seq_cst);
          if( !did_execute ) {
             try {
                action();
                prom.set_value();   
             } catch ( ... ) {
                prom.set_exception( std::current_exception() );
             }
          }
       }
       boost::fibers::promise<void> prom;
       std::function<void()>        action;
  };
  
} // namespace fc
