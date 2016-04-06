#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic utilities
# 	July 1995	A.H. Bril
#_______________________________________________________________________________

set ( OD_USEQT Network)
set ( OD_MODULE_DEPS General)

set ( QT_MOC_HEADERS
	qnetworkaccessconn.h
	qtcpservercomm.h
	qtcpsocketcomm.h)
set ( OD_MODULE_SOURCES
	cvsaccess.cc
	hostdata.cc
	initnetwork.cc
	issuereporter.cc
	jobcommunic.cc
	netfilecache.cc
	netreqconnection.cc
	netreqpacket.cc
	netserver.cc
	netsocket.cc
	oddlsite.cc
	odnetworkaccess.cc
	odnetworkreply.cc
	svnaccess.cc
	systeminfo.cc
	webstreamsource.cc
)
set ( OD_TEST_PROGS networkaccess.cc systeminfo.cc )
set ( OD_NIGHTLY_TEST_PROGS netreq.cc netsocket.cc echoserver.cc netreqechoserver.cc)
OD_INIT_MODULE()
