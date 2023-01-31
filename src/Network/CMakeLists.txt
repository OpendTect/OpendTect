#_______________________Pmake___________________________________________________
#
#	Makefile : 	Network utilities
# 	July 1995	Bert
#_______________________________________________________________________________

if ( UNIX )
    configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/backend-server/upload.sh.in
		    ${CMAKE_BINARY_DIR}/dtect/backend-server-upload.sh @ONLY )
endif( UNIX )

set ( OD_USEQT Network )
set ( OD_USEOPENSSL Yes )
set ( OD_USECRYPTO Yes )
set ( OD_MODULE_DEPS General )

set ( QT_MOC_HEADERS
	qnetworkaccessconn.h
	qtcpservercomm.h
	qtcpsocketcomm.h)
set ( OD_MODULE_SOURCES
	clientservicebase.cc
	cvsaccess.cc
	hostdata.cc
	initnetwork.cc
	issuereporter.cc
	jobcommunic.cc
	netreqconnection.cc
	netreqpacket.cc
	netserver.cc
	netservice.cc
	netsocket.cc
	oddlsite.cc
	odnetworkaccess.cc
	odnetworkreply.cc
	servicemgrbase.cc
	serverservicebase.cc
	svnaccess.cc
	systeminfo.cc
)

set ( OD_MODULE_TESTPROGS
	test_client_connection.cc
	test_listening_server.cc
	test_netreqechoserver.cc
	test_netsocketechoserver.cc
)
set ( OD_TEST_PROGS networkaccess.cc systeminfo.cc )
set ( OD_NIGHTLY_TEST_PROGS netreq.cc netsocket.cc )
OD_INIT_MODULE()

# The test programs are independant, but it is handy to compile the server app
# when the client app is build and run as the target:
add_dependencies( test_netreq test_netreqechoserver )
add_dependencies( test_netsocket test_netsocketechoserver )
