#pragma once
#include <fc/crypto/sha256.hpp>

#define SCRYPT_PADDING_SIZE 48 /* scrypt implementation has 80 byte input */

namespace fc{ namespace scrypt {

struct proof
{
   proof();
   ~proof();

   sha256 hash( const sha256& input );

   sha256 data;
   char padding[SCRYPT_PADDING_SIZE];
};

} } // fc::scrypt

FC_REFLECT( fc::scrypt::proof, (data)(padding) )
