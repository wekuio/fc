#include <fc/crypto/scrypt.hpp>
#include <scrypt/scrypt.h>

namespace fc{ namespace scrypt {

proof::proof() { memset( &padding, 0x00, sizeof( padding ) ); }
proof::~proof() {}

sha256 proof::hash( const sha256& input )
{
   data = input;
   sha256 output;

   scrypt_1024_1_1_256( (char*)&input, (char*)&output );

   return output;
}

} } // fc::scrypt