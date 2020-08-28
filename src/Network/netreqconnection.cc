/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		March 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "netreqconnection.h"

#include "netreqpacket.h"
#include "netserver.h"
#include "netsocket.h"
#include "ptrman.h"
#include "timefun.h"
#include "uistrings.h"

using namespace Network;

#ifdef __win__
#include <iphlpapi.h>
#pragma comment( lib, "iphlpapi" )
#endif

static Threads::Atomic<int> connid;


RequestConnection::RequestConnection( const Authority& authority,
				      bool multithreaded,
				      int timeout )
    : socket_(0)
    , ownssocket_(true)
    , connectionClosed(this)
    , packetArrived(this)
    , id_(connid++)
    , authority_(new Authority(authority))
    , socketthread_(0)
    , timeout_(timeout)
    , stopflag_(0)
    , packettosend_(0)
    , threadreadstatus_(None)
    , triggerread_(false)
{
    if ( multithreaded )
    {
	Threads::MutexLocker locker( lock_ );
	socketthread_ =
	    new Threads::Thread( mCB(this,RequestConnection,socketThreadFunc),
				 "RequestConnection socket thread" );
	lock_.wait(); //Wait for thread to create connection.
    }
    else
    {
	 connectToHost();
    }
}


RequestConnection::RequestConnection( Socket* sock )
    : socket_( sock )
    , ownssocket_( false )
    , connectionClosed( this )
    , packetArrived( this )
    , id_( connid++ )
    , socketthread_( 0 )
    , timeout_( 0 )
    , stopflag_( 0 )
    , packettosend_( 0 )
    , threadreadstatus_( None )
    , triggerread_( false )
{
    if ( !sock )
	return;

    mAttachCB(socket_->disconnected,RequestConnection::connCloseCB);
    mAttachCB(socket_->readyRead,RequestConnection::dataArrivedCB);
}


RequestConnection::~RequestConnection()
{
    detachAllNotifiers();

    delete authority_;
    if ( socketthread_ )
    {
	lock_.lock();
	stopflag_ = true;
	lock_.signal( true );
	lock_.unLock();

	socketthread_->waitForFinish();
	deleteAndZeroPtr( socketthread_ );
    }
    else
    {
	deleteAndZeroPtr( socket_, ownssocket_ );
    }

    deepErase( receivedpackets_ );
}


void RequestConnection::socketThreadFunc( CallBacker* )
{
    if ( socket_ )
    {
	pErrMsg("Thread started with existing socket!");
	return;
    }

    connectToHost();

    lock_.lock();
    while ( !stopflag_ )
    {
	lock_.unLock();
	const bool hasdata = socket_->bytesAvailable();
	lock_.lock();

	if ( triggerread_ || hasdata )
	{
	    lock_.unLock();
	    readFromSocket();
	    lock_.lock();
	    triggerread_ = false;
	    lock_.signal( true );
	    continue;
	}

	if ( packettosend_ && !sendingfinished_ )
	{
	    sendresult_ = doSendPacket( *packettosend_, sendwithwait_ );
	    sendingfinished_ = true;

	    lock_.signal( true );
	    continue;
	}

	//Wait 100 ms. We should in principle be able to wait infinitly
	//if everything is 100% correct. This is however safer
	lock_.wait( 100 );
    }

    deleteAndZeroPtr( socket_, ownssocket_ );
}


void RequestConnection::connectToHost()
{
    if ( socket_ )
    {
	pErrMsg("I did not expect a socket" );
	return;
    }

    if ( !authority_ )
    {
	pErrMsg("I did not expect no authority" );
	return;
    }

    auto* newsocket = new Socket( authority_->isLocal() );

    if ( timeout_ > 0 )
	newsocket->setTimeout( timeout_ );

    if ( newsocket->connectToHost(*authority_,true) )
	mAttachCB(newsocket->disconnected,RequestConnection::connCloseCB);

    Threads::MutexLocker locker( lock_ );

    socket_ = newsocket;
    //Tell eventual constructor waiting that we have at least tried to connect
    lock_.signal( true );
}


bool RequestConnection::isOK() const
{
    if ( !socket_ )
	return false;

    const bool badsocket = socket_->isBad();
    if ( badsocket && !socket_->errMsg().isEmpty() )
	errmsg_.append( socket_->errMsg(), true );

    return !badsocket;
}


BufferString RequestConnection::server() const
{
    return authority_ ? authority_->getHost() : BufferString::empty();
}


PortNr_Type RequestConnection::port() const
{
    return authority_ ? authority_->getPort() : 0;
}


void RequestConnection::connCloseCB( CallBacker* )
{
    connectionClosed.trigger();
}


void RequestConnection::flush()
{
}


bool RequestConnection::readFromSocket()
{
    while ( isOK() )
    {
	PtrMan<RequestPacket> nextreceived = new RequestPacket;
	Socket::ReadStatus readres = socket_->read( *nextreceived );
	if ( readres==Socket::ReadError )
	{
	    errmsg_ = socket_->errMsg();
	    socket_->disconnectFromHost();
	    if ( errmsg_.isEmpty() )
		errmsg_ = tr("Error reading from socket");
	    return false;
	}
	else if ( readres==Socket::ReadOK )
	{
	    if ( !nextreceived->isOK() )
	    {
		socket_->disconnectFromHost();
		errmsg_ = tr("Garbled network packet received. Disconnected.");
		return false;
	    }

	    const od_int32 receivedid = nextreceived->requestID();
	    Threads::MutexLocker locker ( lock_ );
	    if ( nextreceived->isNewRequest() ||
		 ourrequestids_.isPresent( receivedid ) )
	    {
		receivedpackets_ += nextreceived.release();
		lock_.signal( true );
		locker.unLock();

		packetArrived.trigger(receivedid);
	    }
	}

	if ( !socket_->bytesAvailable() ) //Not sure this works
	    return true;
    }

    return false;
}


bool RequestConnection::doSendPacket( const RequestPacket& pkt,
				      bool waitforfinish )
{
    if ( !isOK() )
	return	false;

    const od_int32 reqid = pkt.requestID();

    if ( !ourrequestids_.isPresent( reqid ) )
    {
	if ( pkt.isNewRequest() )
	    ourrequestids_ += reqid;
	else
	{
	    pErrMsg(
		BufferString("Packet send requested for unknown ID: ",reqid) );
	    return false;
	}
    }

    lock_.unLock();
    const bool result = socket_->write( pkt, waitforfinish );
    lock_.lock();

    if ( !result )
	requestEnded( pkt.requestID() );

    return result;
}



bool RequestConnection::sendPacket( const RequestPacket& pkt,
				    bool waitforfinish )
{
    if ( !pkt.isOK() )
	return false;

    if ( !socketthread_ && Threads::currentThread()!=socket_->thread() )
	return false;

    Threads::MutexLocker locker( lock_ );
    //We're non-threaded. Just send
    if ( !socketthread_ )
    {
	return doSendPacket( pkt, waitforfinish );
    }

    //Someone else is sending now. Wait
    while ( packettosend_ )
	lock_.wait();

    //Setup your batch job and poke thread
    packettosend_ = &pkt;
    sendwithwait_ = waitforfinish;
    sendingfinished_ = false;
    lock_.signal( true );

    //Wait for sending to finish
    while ( !sendingfinished_ )
    {
	lock_.wait();
    }

    //Pickup sending result
    const bool result = sendresult_;

    //Tell someone who may be waiting that he can send now.
    packettosend_ = 0;
    lock_.signal( true );

    return result;
}


RequestPacket* RequestConnection::pickupPacket( od_int32 reqid, int timeout,
						int* errorcode )
{
    if ( !socketthread_ && Threads::currentThread()!=socket_->thread() )
	return 0;

    Threads::MutexLocker locker( lock_ );
    const int idxof = ourrequestids_.indexOf( reqid );
    if ( idxof < 0 )
    {
	if ( errorcode ) *errorcode = cInvalidRequest();
	return 0;
    }

    RequestPacket* pkt = getNextAlreadyRead( reqid );
    if ( !pkt )
    {
	const int starttm = Time::getMilliSeconds();
	bool isok = false;
	do
	{
	    const int remaining = timeout - Time::getMilliSeconds() + starttm;
	    if ( remaining<=0 )
		break;

	    if ( !socketthread_ )
	    {
		locker.unLock();
		if( !readFromSocket() )
		{
		    if ( errorcode )
			*errorcode = isOK() ? cTimeout() : cDisconnected();
		    locker.lock();
		    requestEnded( reqid );
		    return 0;
		}

		isok = isOK();

		locker.lock();
	    }
	    else
	    {
		triggerread_ = true;
		lock_.signal( true );
		lock_.wait( remaining );

		locker.unLock();
		isok = isOK();
		locker.lock();
	    }

	    pkt = getNextAlreadyRead( reqid );

	} while ( !pkt && isok );

	if ( !pkt )
	{
	    if ( errorcode )
		*errorcode = isok ? cTimeout() : cDisconnected();
	    return 0;
	}
    }

    if ( pkt->isRequestEnd() )
	requestEnded( reqid );

    return pkt;
}


RequestPacket* RequestConnection::getNextAlreadyRead( int reqid )
{
    for ( int idx=0; idx<receivedpackets_.size(); idx++ )
    {
	if ( reqid < 0 || receivedpackets_[idx]->requestID()==reqid )
	    return receivedpackets_.removeSingle( idx );
    }

    return 0;
}


RequestPacket* RequestConnection::getNextExternalPacket()
{
    Threads::MutexLocker locker( lock_ );

    for ( int idx=0; idx<receivedpackets_.size(); idx++ )
    {
	RequestPacket* pkt = receivedpackets_[idx];
	if ( !ourrequestids_.isPresent(pkt->requestID()) )
	    return receivedpackets_.removeSingle(idx);
    }
    return 0;
}


void RequestConnection::requestEnded( od_int32 reqid )
{
    ourrequestids_ -= reqid;

    for ( int idx=receivedpackets_.size()-1; idx>=0; idx-- )
    {
	if ( receivedpackets_[idx]->requestID()==reqid )
	    delete receivedpackets_.removeSingle( idx );
    }
}


void RequestConnection::dataArrivedCB( CallBacker* cb )
{
    readFromSocket();
}



RequestServer::RequestServer( PortNr_Type servport, SpecAddr specaddr )
    : server_( new Server(Network::isLocal(specaddr)) )
    , newConnection( this )
{
    if ( !server_ )
	return;

    mAttachCB( server_->newConnection, RequestServer::newConnectionCB );
    if ( !server_->listen(specaddr,servport) )
	errmsg_ = TCPErrMsg().arg( servport );
}


RequestServer::RequestServer( const char* servernm )
    : server_(new Server(true))
    , newConnection(this)
{
    if ( !server_ )
	return;

    mAttachCB( server_->newConnection, RequestServer::newConnectionCB );
    uiRetVal ret;
    if ( !server_->listen(servernm,ret) )
    {
	errmsg_ = LocalErrMsg().arg( servernm );
	errmsg_.append( ret, true );
    }
}


RequestServer::RequestServer( const Network::Authority& auth, SpecAddr spcadr )
    : newConnection(this)
{
    server_ = new Server( auth.isLocal() );

    if ( !server_ )
	return;

    mAttachCB( server_->newConnection, RequestServer::newConnectionCB );
    const bool islocal = auth.isLocal();
    uiRetVal ret;
    const bool islistening = islocal ?
				server_->listen( auth.getServerName(), ret ) :
				server_->listen( spcadr, auth.getPort() );

    if ( !islistening )
	errmsg_ = islocal ?
		LocalErrMsg().arg(auth.getServerName()).append(ret,true) :
					    TCPErrMsg().arg(auth.getPort());

}


RequestServer::~RequestServer()
{
    detachAllNotifiers();

    deepErase( pendingconns_ );
    delete server_;
}



uiString RequestServer::TCPErrMsg() const
{
    return tr("Cannot start listening on port %1","port id<number>");
}


uiString RequestServer::LocalErrMsg() const
{
    return tr("Cannot start listening to %1","server name");
}


bool RequestServer::isOK() const
{
    return server_ && server_->isListening();
}


Authority RequestServer::getAuthority() const
{
    return server_ ? server_->authority() : Authority();
}



RequestConnection* RequestServer::pickupNewConnection()
{
    Threads::Locker locker( lock_ );
    return pendingconns_.size() ? pendingconns_.removeSingle( 0 ) : 0;
}


void RequestServer::newConnectionCB(CallBacker* cb)
{
    mCBCapsuleUnpack(int,socketid,cb);
    auto* sock = server_->getSocket(socketid);

    if ( !sock )
	return;

    if ( !sock->isConnected() || sock->isBad() )
    {
	sock->disconnectFromHost();
	return;
    }

    Threads::Locker locker( lock_ );
    pendingconns_ += new RequestConnection( sock );
    locker.unlockNow();

    newConnection.trigger();
}

#ifdef __win__
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
#endif

bool Network::isPortFree( PortNr_Type port, uiString* errmsg )
{
#ifdef __win__
    PMIB_TCPTABLE pTcpTable;
    pTcpTable = (MIB_TCPTABLE *) MALLOC(sizeof (MIB_TCPTABLE));
    if ( !pTcpTable )
	return true;
    DWORD dwSize = sizeof (MIB_TCPTABLE);
    DWORD dwRetVal = GetTcpTable( pTcpTable, &dwSize, TRUE);
    if ( dwRetVal == ERROR_INSUFFICIENT_BUFFER )
    {
	FREE(pTcpTable);
	pTcpTable = (MIB_TCPTABLE *) MALLOC(dwSize);
	if ( !pTcpTable )
	    return true;
    }
    dwRetVal = GetTcpTable( pTcpTable, &dwSize, TRUE );
    if ( dwRetVal != NO_ERROR )
    {
	FREE(pTcpTable);
	return true;
    }
    const int nrentries = pTcpTable->dwNumEntries;
    bool isfound = false;
    for ( int idx=0; idx<nrentries; idx++ )
    {
	const auto entry = pTcpTable->table[idx];
	if ( entry.dwState != MIB_TCP_STATE_LISTEN )
	    continue;
	const auto localport = ntohs((u_short)entry.dwLocalPort);
	if ( localport == port )
	{
	    isfound = true;
	    break;
	}
    }

    if ( pTcpTable != NULL )
    {
	FREE(pTcpTable);
	pTcpTable = NULL;
    }

    return !isfound;
#else
    const RequestServer reqserv( port );
    const bool ret = reqserv.isOK();
    if ( errmsg && !reqserv.errMsg().isEmpty() )
	*errmsg = reqserv.errMsg();

    return ret;
#endif
}

#ifdef __win__
#undef MALLOC
#undef FREE
#endif

