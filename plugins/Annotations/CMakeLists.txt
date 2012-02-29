#_______________________Pmake___________________________________________________
#
#	Makefile : 	Annotations plugin
# 	Jan 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-02-29 13:14:14 cvsnageswara Exp $
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
OD_INIT_MODULE()
