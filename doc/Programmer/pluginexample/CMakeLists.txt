#_______________________Pmake___________________________________________________
#
#	Makefile : 	Tutorial plugin
# 	Jan 2012	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-03-29 07:23:42 cvskris Exp $
#_______________________________________________________________________________


cmake_minimum_required (VERSION 2.6)
PROJECT(OpendTect_Tutorial_Plugin)

SET ( OD_SUBSYSTEM "tut_plugin" )
SET ( OpendTect_DIR "" CACHE PATH "OpendTect directory" )

LIST(APPEND CMAKE_MODULE_PATH "${OpendTect_DIR}/CMakeModules")
FIND_PACKAGE( OpendTect )

OPTION ( OD_BUILD_LOCAL "Build in local directory" ON )

#Setup the modules
SET ( PLUGINS Tut uiTut )
OD_ADD_MODULES( plugins ${PLUGINS} )

OD_WRITE_ALOFILES( ${CMAKE_CURRENT_BINARY_DIR} )
