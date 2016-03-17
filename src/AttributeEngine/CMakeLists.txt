#_______________________Pmake___________________________________________________
#
#	Makefile : 	Attribute Engine classes
# 	Jul 2000	Kris Tingdahl
#_______________________________________________________________________________

set(OD_MODULE_DEPS NLA Seis )
set(OD_MODULE_SOURCES
		attribdataholder.cc
		attribdataholderarray.cc
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
