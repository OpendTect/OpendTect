#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic utilities
# 	July 1995	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-01-11 13:55:38 cvskris Exp $
#_______________________________________________________________________________

SET(LIB_NAME Network)
SET(OD_USEQT Core)
SET(DEPS General)

SET(QT_MOC_HEADERS 
	    qftpconn.h
	    qhttpconn.h
	    qlocalservercomm.h
	    qlocalsocketcomm.h
	    qtcpservercomm.h
	    qtcpsocketcomm.h)
SET(LIB_SOURCES 
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
OD_INIT_MODULE()
