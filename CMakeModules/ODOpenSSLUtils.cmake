#_______________________CMake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Oct 2020	A.Huck
#_______________________________________________________________________________

MACRO( OD_SETUP_OPENSSLCOMP COMP )
    if( NOT DEFINED OPENSSL_${COMP}_LIBRARY} )

	if ( DEFINED QTDIR )
	    get_filename_component( QTINSTDIR ${QTDIR} REALPATH )
	    set( OPENSSL_HINT_DIR "${QTINSTDIR}/../../Tools/OpenSSL" )
	    get_filename_component( OPENSSL_HINT_DIR ${OPENSSL_HINT_DIR} REALPATH )
	    if ( EXISTS ${OPENSSL_HINT_DIR} )
		set( OPENSSL_ROOT_DIR ${OPENSSL_HINT_DIR} )
		if ( WIN32 )
		    set( OPENSSL_ROOT_DIR ${OPENSSL_ROOT_DIR}/Win_x64 )
		elseif( NOT DEFINED APPLE )
		    set( OPENSSL_ROOT_DIR ${OPENSSL_ROOT_DIR}/binary )
		endif( WIN32 )
	    endif()
	endif( DEFINED QTDIR )

	find_package( OpenSSL 1.1.1 QUIET COMPONENTS ${COMP} )
	OD_READ_TARGETINFO( OpenSSL::${COMP} )
	if ( EXISTS "${SOURCEFILE}" )
	    add_definitions( -D__OpenSSL_${COMP}_LIBRARY__="${SOURCEFILE}" )
	endif()

    endif()
ENDMACRO( OD_SETUP_OPENSSLCOMP )

MACRO( OD_SETUP_OPENSSL )
    OD_SETUP_OPENSSLCOMP( SSL )
    if ( OD_USEOPENSSL )
	if ( EXISTS "${OPENSSL_SSL_LIBRARY}" )
	    list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS OpenSSL::SSL )
	endif()
    endif()
ENDMACRO( OD_SETUP_OPENSSL )

MACRO( OD_SETUP_CRYPTO )
    OD_SETUP_OPENSSLCOMP( Crypto )
    if ( OD_USECRYPTO )
	if ( EXISTS "${OPENSSL_CRYPTO_LIBRARY}" )
	    list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS OpenSSL::Crypto )
	endif()
    elseif( OD_LINKCRYPTO )
	if ( EXISTS "${OPENSSL_CRYPTO_LIBRARY}" )
	    list( APPEND OD_MODULE_INCLUDESYSPATH ${OpenSSL_INCLUDE_DIR} )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS OpenSSL::Crypto )
	endif()
    endif()
ENDMACRO( OD_SETUP_CRYPTO )
