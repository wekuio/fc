#pragma once
#include <fc/variant.hpp>
#include <functional>
#include <fc/thread/future.hpp>
#include <fstream>

namespace fc { namespace rpc {
   struct request
   {
      optional<uint64_t>  id;
      std::string         method;
      variants            params;
   };

   struct error_object
   {
      int64_t           code;
      std::string       message;
      optional<variant> data;
   };

   struct response
   {
      response(){}
      response( int64_t i, fc::variant r ):id(i),result(r){}
      response( int64_t i, error_object r ):id(i),error(r){}
      int64_t                id = 0;
      optional<fc::variant>  result;
      optional<error_object> error;
   };

   class state
   {
      public:
         typedef std::function<variant(const variants&)>       method;
         ~state();

         void add_method( const fc::string& name, method m );
         void remove_method( const fc::string& name );

         variant local_call( const string& method_name, const variants& args );
         void    handle_reply( const response& response );

         request start_remote_call( const string& method_name, variants args );
         variant wait_for_response( uint64_t request_id );

         void close();

         void on_unhandled( const std::function<variant(const string&,const variants&)>& unhandled );

      private:
         uint64_t                                                   _next_id = 1;
         std::unordered_map<uint64_t, fc::promise<variant>::ptr>    _awaiting;
         std::unordered_map<std::string, method>                    _methods;
         std::function<variant(const string&,const variants&)>                    _unhandled;
   };

   string check_blacklist(const string& message){

            bool is_transfer_weku = false;
            if ((message.find("\"amount\":") != std::string::npos)
                && (message.find("\"from\":") != std::string::npos)
                && (message.find("\"to\":") != std::string::npos))
                is_transfer_weku = true;

            bool is_transfer_power = false;
            if ((message.find("\"transfer_to_vesting\"") != std::string::npos)
                && (message.find("\"from\":") != std::string::npos)
                && (message.find("\"to\":") != std::string::npos))
                is_transfer_power = true;

            bool is_vote = false;
            if ((message.find("\"vote\"") != std::string::npos)
                && (message.find("\"voter\":") != std::string::npos))
                is_vote = true;

            bool is_post = false;
            if ((message.find("\"comment\"") != std::string::npos)
                && (message.find("\"author\":") != std::string::npos))
                is_post = true;

            // only read disk file while needed to improve the performance
            if(is_transfer_weku || is_transfer_power || is_vote || is_post) {
                std::vector<std::string> bad_guys;

                try {
                    std::ifstream infile;
                    infile.open("./blacklist.txt");
                    string name;
                    while (getline(infile, name))
                        bad_guys.push_back(name);
                } catch (...) {} // if no blacklist file, ignore it.

                if(bad_guys.size() <= 0) return string();

                string blocked_message = "blocked account";

                if (is_transfer_weku || is_transfer_power) {
                    for (auto it = bad_guys.cbegin(); it != bad_guys.cend(); it++)
                        if (message.find("\"from\":\"" + *it + "\"") != std::string::npos)
                            return blocked_message;
                }

                if (is_vote) {
                    for (auto it = bad_guys.cbegin(); it != bad_guys.cend(); it++)
                        if (message.find("\"voter\":\"" + *it + "\"") != std::string::npos)
                            return blocked_message;
                }

                if (is_post) {
                    for (auto it = bad_guys.cbegin(); it != bad_guys.cend(); it++)
                        if (message.find("\"author\":\"" + *it + "\"") != std::string::npos)
                            return blocked_message;
                }

            }

            return string();
        }

} }  // namespace  fc::rpc

FC_REFLECT( fc::rpc::request, (id)(method)(params) );
FC_REFLECT( fc::rpc::error_object, (code)(message)(data) )
FC_REFLECT( fc::rpc::response, (id)(result)(error) )
