#_______________________Pmake___________________________________________________
#
#	Makefile : 	Annotations plugin
# 	Jan 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012/03/29 11:33:24 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_IS_PLUGIN yes)
SET( OD_USECOIN 1)
SET(OD_MODULE_SOURCES
	annotationspi.cc
	measuretoolman.cc
	treeitem.cc
	uiarrowdlg.cc
	uimeasuredlg.cc
	visannotimage.cc
	visarrow.cc
	viscallout.cc )

SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )

OD_INIT_MODULE()
