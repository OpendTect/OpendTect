#_______________________Pmake___________________________________________________
#
#	Makefile : 	Annotations plugin
# 	Jan 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

set( OD_MODULE_DEPS uiODMain)
set( OD_IS_PLUGIN yes)
set( OD_USECOIN 1)
set( OD_MODULE_SOURCES
	annotationspi.cc
	treeitem.cc
	uiarrowdlg.cc
	uiscalebaritem.cc
	visannotimage.cc
	visarrow.cc
	viscallout.cc
	visscalebar.cc )
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()

