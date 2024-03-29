# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindSQLite3
-----------

.. versionadded:: 3.14

Find the SQLite libraries, v3

IMPORTED targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``SQLite::SQLite3``

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``SQLite3_INCLUDE_DIRS``
  where to find sqlite3.h, etc.
``SQLite3_LIBRARIES``
  the libraries to link against to use SQLite3.
``SQLite3_VERSION``
  version of the SQLite3 library found
``SQLite3_FOUND``
  TRUE if found

#]=======================================================================]

# Look for the necessary header
find_path(SQLite3_INCLUDE_DIR NAMES sqlite3.h)
mark_as_advanced(SQLite3_INCLUDE_DIR)

# Look for the necessary library
find_library(SQLite3_LIBRARY NAMES sqlite3 sqlite)
get_filename_component( SQLite3_LIBRARY "${SQLite3_LIBRARY}" REALPATH )
mark_as_advanced(SQLite3_LIBRARY)

# Extract version information from the header file
if(SQLite3_INCLUDE_DIR)
    file(STRINGS ${SQLite3_INCLUDE_DIR}/sqlite3.h _ver_line
         REGEX "^#define SQLITE_VERSION  *\"[0-9]+\\.[0-9]+\\.[0-9]+\""
         LIMIT_COUNT 1)
    string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+"
           SQLite3_VERSION "${_ver_line}")
    unset(_ver_line)
endif()


include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
find_package_handle_standard_args(SQLite3
    REQUIRED_VARS SQLite3_INCLUDE_DIR SQLite3_LIBRARY
    VERSION_VAR SQLite3_VERSION)

# Create the imported target
if(SQLite3_FOUND)
    set(SQLite3_INCLUDE_DIRS ${SQLite3_INCLUDE_DIR})
    set(SQLite3_LIBRARIES ${SQLite3_LIBRARY})
    if(NOT TARGET SQLite::SQLite3)
        add_library(SQLite::SQLite3 SHARED IMPORTED)
        set_target_properties(SQLite::SQLite3 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${SQLite3_INCLUDE_DIR}")
	if ( WIN32 )
	    od_get_dll( "${SQLite3_LIBRARY}" SQLITE_LOCATION )
	    set_target_properties( SQLite::SQLite3 PROPERTIES
		IMPORTED_LOCATION "${SQLITE_LOCATION}"
		IMPORTED_IMPLIB "${SQLite3_LIBRARY}" )
	    unset( SQLITE_LOCATION )
	else()
	    set_target_properties( SQLite::SQLite3 PROPERTIES
		IMPORTED_LOCATION "${SQLite3_LIBRARY}" )
	    get_filename_component( SQLITE_SONAME "${SQLite3_LIBRARY}" REALPATH )
	    if ( APPLE )
		get_filename_component( SQLITE_SONAME "${SQLITE_SONAME}" NAME_WE )
		set( SQLITE_SONAME "@rpath/${SQLITE_SONAME}.dylib" )
	    else()
		get_filename_component( SQLITE_SONAME "${SQLITE_SONAME}" NAME_WLE )
		get_filename_component( SQLITE_SONAME "${SQLITE_SONAME}" NAME_WLE )
	    endif()
	    set_target_properties( SQLite::SQLite3 PROPERTIES
		IMPORTED_SONAME "${SQLITE_SONAME}" )
	    unset( SQLITE_SONAME )
	endif()
    endif()
endif()
