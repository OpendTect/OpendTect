#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic utilities
# 	July 1995	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012-01-23 20:55:23 cvskris Exp $
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
OD_INIT_MODULE()
