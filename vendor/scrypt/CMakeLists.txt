file(GLOB HEADERS "include/scrypt/*.hpp" )

set( CMAKE_C_FLAGS "-std=c99" )

add_library( scrypt
             src/scrypt.cpp
           )

target_link_libraries( scrypt ${OPENSSL_LIBRARIES} )
target_include_directories( scrypt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" ${OPENSSL_INCLUDE_DIR} )

install( TARGETS
   scrypt

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
