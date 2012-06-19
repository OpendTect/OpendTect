#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic utilities
# 	July 1995	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.4 2012-06-19 12:07:05 cvskris Exp $
#_______________________________________________________________________________

SET(OD_USEQT Core)
SET(OD_MODULE_DEPS General)

SET(QT_MOC_HEADERS 
	    qftpconn.h
	    qhttpconn.h
	    qlocalservercomm.h
	    qlocalsocketcomm.h
	    qtcpservercomm.h
	    qtcpsocketcomm.h)
SET(OD_MODULE_SOURCES 
		cvsaccess.cc
		ftptask.cc
		httptask.cc
		jobcommunic.cc
		localserver.cc
		localsocket.cc
		oddlsite.cc
		odftp.cc
		odhttp.cc
		systeminfo.cc
		tcpserver.cc
		tcpsocket.cc)
SET ( OD_MODULE_PROGS od_ReportIssue.cc )
OD_INIT_MODULE()
