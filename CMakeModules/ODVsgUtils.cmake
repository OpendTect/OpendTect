#________________________________________________________________________
#
# Copyright:	(C) 1995-2026 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_FIND_VSG )

    if ( NOT OD_NO_VSG )
	if ( NOT VSG_FOUND )
	    if ( NOT TARGET Vulkan::Vulkan )
		find_package( Vulkan QUIET GLOBAL CONFIG
		    PATHS "${VULKAN_ROOT}" "${VSG_ROOT}"
		    HINTS "${CMAKE_PREFIX_PATH}" NO_DEFAULT_PATH )
		if ( NOT TARGET Vulkan::Vulkan )
		    find_package( Vulkan QUIET GLOBAL )
		    unset( Vulkan_DIR CACHE )
		endif()
		if ( TARGET Vulkan::Vulkan )
		    od_setup_external_target( Vulkan::Vulkan )
		    unset( VULKAN_ROOT CACHE )
		endif()
	    endif()
	endif()

	if ( TARGET Vulkan::Vulkan )
	    # A VSG built with shader-compiler support (VSG_SUPPORTS_ShaderCompiler)
	    # looks for GLSLang in its own config file, so it has to be found first.
	    set( GLSLANG_ROOT "" CACHE PATH "GLSLang Location (only needed by a VSG with shader-compiler support)" )
	    if ( NOT TARGET glslang::glslang )
		find_package( glslang QUIET CONFIG GLOBAL
			PATHS "${GLSLANG_ROOT}" HINTS "${CMAKE_PREFIX_PATH}" NO_DEFAULT_PATH )
		if ( NOT TARGET glslang::glslang )
		    find_package( glslang QUIET CONFIG GLOBAL )
		endif()
	    endif()

	    find_package( vsg QUIET CONFIG GLOBAL
		    PATHS "${VSG_ROOT}" HINTS "${CMAKE_PREFIX_PATH}" NO_DEFAULT_PATH )
	    if ( NOT TARGET vsg::vsg )
		find_package( vsg QUIET GLOBAL )
		unset( vsg_DIR CACHE )
	    endif()

	    if ( TARGET vsg::vsg )
		od_setup_external_target( vsg::vsg )
	    endif()
	endif()
    endif()

endmacro( OD_FIND_VSG )

macro( OD_USE_VSG )

    if ( NOT OD_NO_VSG )
	if ( TARGET Vulkan::Vulkan AND TARGET vsg::vsg )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS vsg::vsg )
	else()
	    set( VSG_ROOT "" CACHE PATH "VulkanSceneGraph (VSG) Location" )
	    if ( NOT TARGET Vulkan::Vulkan )
		message( SEND_ERROR "Cannot find the Vulkan installation" )
	    else()
		message( SEND_ERROR "Cannot find the VulkanSceneGraph (VSG) installation" )
	    endif()
	endif()
    endif()

endmacro( OD_USE_VSG )

macro( OD_FIND_VSGXCHANGE )

    if ( NOT OD_NO_VSG )
	if ( TARGET vsg::vsg AND NOT TARGET vsgXchange::vsgXchange )
	    find_package( vsgXchange QUIET CONFIG GLOBAL
		    PATHS "${VSGXCHANGE_ROOT}" HINTS "${CMAKE_PREFIX_PATH}" NO_DEFAULT_PATH )
	    if ( NOT TARGET vsgXchange::vsgXchange )
		find_package( vsgXchange QUIET GLOBAL )
		unset( vsgXchange_DIR CACHE )
	    endif()
	    if ( TARGET vsgXchange::vsgXchange )
		od_setup_external_target( vsgXchange::vsgXchange )
	    endif()
	endif()
    endif()

endmacro( OD_FIND_VSGXCHANGE )
