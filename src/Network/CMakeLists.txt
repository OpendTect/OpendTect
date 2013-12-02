#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic utilities
# 	July 1995	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_USEQT Network)
SET(OD_MODULE_DEPS General)

SET(QT_MOC_HEADERS 
	qlocalservercomm.h
	qlocalsocketcomm.h
	qnetworkaccessconn.h
	qtcpservercomm.h
	qtcpsocketcomm.h)
SET(OD_MODULE_SOURCES 
		cvsaccess.cc
		issuereporter.cc
		jobcommunic.cc
		localserver.cc
		localsocket.cc
		oddlsite.cc
		odnetworkaccess.cc
		odnetworkreply.cc
		svnaccess.cc
		systeminfo.cc
		tcpserver.cc
		tcpsocket.cc)
SET ( OD_TEST_PROGS networkaccess.cc )
SET ( OD_MODULE_PROGS od_ReportIssue.cc )
OD_INIT_MODULE()
