#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic utilities
# 	July 1995	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

if ( UNIX )
    configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/backend-server/upload.sh.in
		    ${CMAKE_BINARY_DIR}/dtect/backend-server-upload.sh @ONLY )
endif( UNIX )

add_definitions( -DmNoAutoUiString )

set ( OD_USEQT Network)
set ( OD_MODULE_DEPS General)

set ( QT_MOC_HEADERS
	qnetworkaccessconn.h
	qtcpservercomm.h
	qtcpsocketcomm.h)
set ( OD_MODULE_SOURCES
	cvsaccess.cc
	hostdata.cc
	issuereporter.cc
	jobcommunic.cc
	netreqconnection.cc
	netreqpacket.cc
	oddlsite.cc
	odnetworkaccess.cc
	odnetworkreply.cc
	svnaccess.cc
	systeminfo.cc
	netserver.cc
	netsocket.cc
)
set ( OD_TEST_PROGS networkaccess.cc systeminfo.cc )
set ( OD_NIGHTLY_TEST_PROGS netreq.cc netsocket.cc echoserver.cc netreqechoserver.cc)
OD_INIT_MODULE()
