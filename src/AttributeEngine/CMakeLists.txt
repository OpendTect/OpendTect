#_______________________Pmake___________________________________________________
#
#	Makefile : 	Attribute Engine classes
# 	Jul 2000	Kris Tingdahl
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012/01/23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS NLA Seis )
SET(OD_MODULE_SOURCES
		attribdatacubes.cc
		attribdatacubeswriter.cc
		attribdataholder.cc
		attribdataholderarray.cc
		attribdatapack.cc
		attribdesc.cc
		attribdescset.cc
		attribdescsetman.cc
		attribdescsetsholder.cc
		attribdescsettr.cc
		attribengman.cc
		attribfactory.cc
		attriblinebuffer.cc
		attribparam.cc
		attribparambase.cc
		attribposvecoutput.cc
		attribprocessor.cc
		attribprovider.cc
		attribsel.cc
		attribslice.cc
		attribsteering.cc
		attribstorprovider.cc
		attriboutput.cc
		externalattrib.cc
		initattributeengine.cc
)
OD_INIT_MODULE()


