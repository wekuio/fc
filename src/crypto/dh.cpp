#include <fc/crypto/dh.hpp>
#include <openssl/dh.h>

namespace fc {
    SSL_TYPE(ssl_dh, DH, DH_free)

    static bool validate( const ssl_dh& dh, bool& valid ) {
        int check;
        DH_check(dh,&check);
        return valid = !(check /*& DH_CHECK_P_NOT_SAFE_PRIME*/);
    }

   bool diffie_hellman::generate_params( int s, uint8_t g )
   {
        ssl_dh dh = DH_new();
        DH_generate_parameters_ex( dh, s, g, NULL);
        const BIGNUM *dhp;
        DH_get0_pqg(dh, &dhp, NULL, NULL);
        p.resize( BN_num_bytes( dhp ) );
        if( p.size() )
            BN_bn2bin( dhp, (unsigned char*)&p.front()  );
        this->g = g;
        return fc::validate( dh, valid );
   }

   bool diffie_hellman::validate()
   {
        if( !p.size() )
            return valid = false;
        ssl_dh dh = DH_new();
        DH_set0_pqg(dh, BN_bin2bn( (unsigned char*)&p.front(), p.size(), NULL ), NULL, BN_bin2bn( (unsigned char*)&g, 1, NULL ));
        return fc::validate( dh, valid );
   }

   bool diffie_hellman::generate_pub_key()
   {
        if( !p.size() )
            return valid = false;
        ssl_dh dh = DH_new();
        DH_set0_pqg(dh, BN_bin2bn( (unsigned char*)&p.front(), p.size(), NULL ), NULL, BN_bin2bn( (unsigned char*)&g, 1, NULL ));

        if( !fc::validate( dh, valid ) )
        {
            return false;
        }
        DH_generate_key(dh);

        const BIGNUM *dhpub;
        const BIGNUM *dhpriv;
        DH_get0_key(dh, &dhpub, &dhpriv);
        pub_key.resize( BN_num_bytes( dhpub ) );
        priv_key.resize( BN_num_bytes( dhpriv ) );
        if( pub_key.size() )
            BN_bn2bin( dhpub, (unsigned char*)&pub_key.front()  );
        if( priv_key.size() )
            BN_bn2bin( dhpriv, (unsigned char*)&priv_key.front()  );

        return true;
   }

   bool diffie_hellman::compute_shared_key( const char* buf, uint32_t s ) {
        ssl_dh dh = DH_new();
        DH_set0_pqg(dh, BN_bin2bn( (unsigned char*)&p.front(), p.size(), NULL ), NULL, BN_bin2bn( (unsigned char*)&g, 1, NULL ));
        DH_set0_key(dh, BN_bin2bn( (unsigned char*)&pub_key.front(), pub_key.size(), NULL ), BN_bin2bn( (unsigned char*)&priv_key.front(), priv_key.size(), NULL ));
        int check;
        DH_check(dh,&check);
        if( !fc::validate( dh, valid ) )
        {
            return false;
        }

        ssl_bignum pk;
        BN_bin2bn( (unsigned char*)buf, s, pk );
        shared_key.resize( DH_size(dh) );
        DH_compute_key( (unsigned char*)&shared_key.front(), pk, dh );

        return true;
   }
   bool diffie_hellman::compute_shared_key( const std::vector<char>& pubk ) {
      return compute_shared_key( &pubk.front(), pubk.size() );
   }
}
