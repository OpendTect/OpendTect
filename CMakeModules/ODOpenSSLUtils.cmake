#_______________________CMake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Oct 2020	A.Huck
#_______________________________________________________________________________

macro( OD_FIND_OPENSSLCOMP COMP )

    if ( EXISTS "${QT_DIR}" )
	OD_FIND_QTDIR()
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
    endif( EXISTS "${QT_DIR}" )

    find_package( OpenSSL 1.1.1 QUIET COMPONENTS ${COMP} )

endmacro( OD_FIND_OPENSSLCOMP )

macro( OD_FIND_OPENSSL )
    OD_FIND_OPENSSLCOMP( Crypto )
    OD_FIND_OPENSSLCOMP( SSL )
endmacro(OD_FIND_OPENSSL)

macro( OD_SETUP_OPENSSLCOMP COMP )
    if ( NOT DEFINED OPENSSL_${COMP}_LIBRARY} )

	if ( TARGET OpenSSL::${COMP} )
	    OD_READ_TARGETINFO( OpenSSL::${COMP} )
	    if ( EXISTS "${SOURCEFILE}" )
		get_filename_component( SOURCEFILE ${SOURCEFILE} REALPATH )
		add_definitions( -D__OpenSSL_${COMP}_LIBRARY__="${SOURCEFILE}" )
	    endif()
	else()
	    message( WARNING "OpenSSL component ${COMP} is not available.")
	endif()

    endif()
endmacro( OD_SETUP_OPENSSLCOMP )

macro( OD_SETUP_CRYPTO )

    if ( OD_USECRYPTO OR OD_LINKCRYPTO )
	OD_SETUP_OPENSSLCOMP( Crypto )
    endif()

    if ( EXISTS "${OPENSSL_CRYPTO_LIBRARY}" )
	if ( OD_LINKCRYPTO )
	    list( APPEND OD_MODULE_INCLUDESYSPATH ${OpenSSL_INCLUDE_DIR} )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS OpenSSL::Crypto )
	elseif ( OD_USECRYPTO )
	    list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS OpenSSL::Crypto )
	endif()
    endif()

endmacro( OD_SETUP_CRYPTO )

macro( OD_SETUP_OPENSSL )

    if ( OD_USEOPENSSL OR OD_LINKOPENSSL )
	OD_SETUP_OPENSSLCOMP( SSL )
    endif()

    if ( EXISTS "${OPENSSL_SSL_LIBRARY}" )
	if ( OD_LINKOPENSSL )
	    list( APPEND OD_MODULE_INCLUDESYSPATH ${OpenSSL_INCLUDE_DIR} )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS OpenSSL::SSL )
	elseif( OD_USEOPENSSL )
	    list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS OpenSSL::SSL )
	endif()
    endif()

endmacro( OD_SETUP_OPENSSL )

