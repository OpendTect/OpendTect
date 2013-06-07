#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to build a release
# Author:       K. Tingdahl
# Date:		August 2012		
#RCS:           $Id: packagelist.od.cmake,v 1.1 2012/08/02 13:41:44 cvsnageswara Exp $


set ( DOC_FILELIST "doc" )
set ( DOC_PLFDEP )

set ( DEVEL_FILELIST "CMakeModules" )
if ( WIN32 )
    list ( APPEND DEVEL_FILELIST "/bin/${PLFSUBDIR}/Debug" )
endif ( WIN32 )
set ( DEVEL_PLFDEP 1 )

set ( PACKAGELIST "doc" "devel" )
