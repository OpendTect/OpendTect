#_______________________CMake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Oct 2020	A.Huck
#_______________________________________________________________________________

MACRO( OD_SETUP_OPENSSLCOMP COMP )
	if( NOT DEFINED OPENSSL_${COMP}_LIBRARY} )

	if ( DEFINED QTDIR )
	set( OPENSSL_HINT_DIR "${QTDIR}/../../Tools/OpenSSL" )
	if ( EXISTS ${OPENSSL_HINT_DIR} )
		set( OPENSSL_ROOT_DIR ${OPENSSL_HINT_DIR} )
	    if ( WIN32 )
	        set( OPENSSL_ROOT_DIR ${OPENSSL_ROOT_DIR}/Win_x64 )
	    elseif( NOT DEFINED APPLE )
	        set( OPENSSL_ROOT_DIR ${OPENSSL_ROOT_DIR}/binary )
	    endif( WIN32 )
	endif()
	endif( DEFINED QTDIR )

	find_package( OpenSSL 1.1.1 REQUIRED COMPONENTS ${COMP} )

    endif()
ENDMACRO( OD_SETUP_OPENSSLCOMP )

MACRO( OD_SETUP_OPENSSL )
    OD_SETUP_OPENSSLCOMP( SSL )
ENDMACRO( OD_SETUP_OPENSSL )

MACRO( OD_SETUP_CRYPTO )
    OD_SETUP_OPENSSLCOMP( Crypto )
ENDMACRO( OD_SETUP_CRYPTO )
