#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic utilities
# 	July 1995	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

set ( OD_USEQT Network)
set ( OD_MODULE_DEPS General)

set ( QT_MOC_HEADERS
	qnetworkaccessconn.h
	qtcpservercomm.h
	qtcpsocketcomm.h)
set ( OD_MODULE_SOURCES
		cvsaccess.cc
		issuereporter.cc
		jobcommunic.cc
		netreqcommunic.cc
		netreqpacket.cc
		oddlsite.cc
		odnetworkaccess.cc
		odnetworkreply.cc
		svnaccess.cc
		systeminfo.cc
		tcpserver.cc
		tcpsocket.cc)
set ( OD_TEST_PROGS networkaccess.cc )
set ( OD_NIGHTLY_TEST_PROGS netreq.cc tcpsocket.cc echoserver.cc netreqechoserver.cc)
set ( OD_MODULE_PROGS od_ReportIssue.cc )
OD_INIT_MODULE()
