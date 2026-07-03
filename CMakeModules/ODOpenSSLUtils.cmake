#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_SETUP_OPENSSLCOMP COMP )
    if ( UNIX )
	list( APPEND OD_MODULE_COMPILE_DEFINITIONS
	      "__OpenSSL_${COMP}_PATH__=\"$<TARGET_FILE_DIR:OpenSSL::${COMP}>\"" )

	get_target_property( OPENSSLLIBTYPE OpenSSL::${COMP} TYPE )
	if ( "${OPENSSLLIBTYPE}" STREQUAL SHARED_LIBRARY )
	    list( APPEND OD_MODULE_COMPILE_DEFINITIONS
		  "__OpenSSL_${COMP}_LIBRARY__=\"$<TARGET_SONAME_FILE_NAME:OpenSSL::${COMP}>\"" )
	else()
	    get_target_property( OPENSSLLIBSONAME OpenSSL::${COMP} IMPORTED_SONAME )
	    if ( OPENSSLLIBSONAME )
		list( APPEND OD_MODULE_COMPILE_DEFINITIONS
		      "__OpenSSL_${COMP}_LIBRARY__=\"${OPENSSLLIBSONAME}\"" )
	    endif()
	    unset( OPENSSLLIBSONAME )
	endif()
	unset( OPENSSLLIBTYPE )
    endif()
endmacro(OD_SETUP_OPENSSLCOMP)

macro( OD_FIND_OPENSSL )

    if ( QT_VERSION VERSION_GREATER_EQUAL 6 )
	set( OpenSSL_REQ_VER 3 )
    elseif ( QT_VERSION VERSION_EQUAL 5 )
	set( OpenSSL_REQ_VER 1.1.1 )
    endif()

    if ( NOT TARGET OpenSSL::SSL OR NOT TARGET OpenSSL::Crypto )
	find_package( OpenSSL ${OpenSSL_REQ_VER} QUIET COMPONENTS SSL Crypto CONFIG GLOBAL PATHS "${OPENSSL_ROOT_DIR}" HINTS "${CMAKE_PREFIX_PATH}" NO_DEFAULT_PATH )
	if ( NOT TARGET OpenSSL::SSL OR NOT TARGET OpenSSL::Crypto )
	    find_package( OpenSSL ${OpenSSL_REQ_VER} QUIET COMPONENTS SSL Crypto GLOBAL )
	    unset( OpenSSL_DIR CACHE )
	endif()
	if ( TARGET OpenSSL::SSL AND TARGET OpenSSL::Crypto )
	    od_setup_external_target( OpenSSL::Crypto )
	    od_setup_external_target( OpenSSL::SSL )
	    unset( OPENSSL_ROOT_DIR CACHE )
	else()
	    message( WARNING "Cannot find/use the OpenSSL installation" )
	endif()
    endif()

endmacro(OD_FIND_OPENSSL)

macro( OD_SETUP_CRYPTO )

    if ( OPENSSL_FOUND AND TARGET OpenSSL::Crypto )
	if ( OD_LINKCRYPTO )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS OpenSSL::Crypto )
	elseif ( OD_USECRYPTO )
	    list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS OpenSSL::Crypto )
	    OD_SETUP_OPENSSLCOMP( Crypto )
	endif()
    else()
	set( OPENSSL_ROOT_DIR "" CACHE PATH "OpenSSL Location" )
    endif()

endmacro(OD_SETUP_CRYPTO)

macro( OD_SETUP_OPENSSL )

    if ( OPENSSL_FOUND AND TARGET OpenSSL::SSL )
	if ( OD_LINKOPENSSL )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS OpenSSL::SSL )
	elseif( OD_USEOPENSSL )
	    list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS OpenSSL::SSL )
	    OD_SETUP_OPENSSLCOMP( SSL )
	endif()
	if ( EXISTS "${OPENSSL_PROGRAM}" )
	    list( APPEND OD_MODULE_COMPILE_DEFINITIONS
		"__OPENSSL_EXEC__=\"${OPENSSL_PROGRAM}\"" )
	endif()
	if ( UNIX AND NOT APPLE AND (OD_LINKCRYPTO OR OD_LINKOPENSSL) )
	    OD_INSTALL_DATADIR_MOD( "OpenSSL" )
	endif()
    else()
	set( OPENSSL_ROOT_DIR "" CACHE PATH "OpenSSL Location" )
	message( SEND_ERROR "Cannot find/use the OpenSSL installation" )
    endif()

endmacro(OD_SETUP_OPENSSL)

