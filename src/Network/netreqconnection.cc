/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		March 2009
________________________________________________________________________

-*/

#include "netreqconnection.h"

#include "applicationdata.h"
#include "envvars.h"
#include "netreqpacket.h"
#include "netsocket.h"
#include "netserver.h"
#include "timefun.h"
#include "ptrman.h"
#include "uistrings.h"
#include "systeminfo.h"

#ifndef OD_NO_QT
# include <QObject>
# include <QCoreApplication>
# include <QTcpSocket>
#endif

namespace Network
{

mUseType( RequestConnection, port_nr_type );

struct PacketSendData : public RefCount::Referenced
{
    PacketSendData(const RequestPacket&,bool wait);
    ConstRefMan<RequestPacket>  packet_;
    bool                        waitforfinish_;

    enum SendStatus             { NotAttempted, Sent, Failed };
    SendStatus			sendstatus_;
				//Protected by connections' lock_
};


static Threads::Atomic<int> connid;


RequestConnection::RequestConnection( const char* servername,
				      port_nr_type servport,
				      bool multithreaded,
				      int timeout )
    : socket_( 0 )
    , ownssocket_( true )
    , servername_( servername )
    , serverport_( servport )
    , connectionClosed( this )
    , packetArrived( this )
    , id_( connid++ )
    , socketthread_( 0 )
    , eventloop_( 0 )
    , timeout_( timeout )
{
    if ( multithreaded )
    {
	socketthread_ =
	    new Threads::Thread( mCB(this,RequestConnection,socketThreadFunc),
				 "RequestConnection socket thread" );

	Threads::ConditionVar eventlooplock;
        eventlooplock.lock();
	eventlooplock_ = &eventlooplock;
	while ( !eventloop_ )
	    eventlooplock.wait(); //Wait for thread to create connection.
        eventlooplock.unLock();
	eventlooplock_ = 0;
    }
    else
    {
	 connectToHost( false );
    }
}


RequestConnection::RequestConnection( Network::Socket* sock )
    : socket_( sock )
    , ownssocket_( false )
    , serverport_( mUdf(port_nr_type) )
    , connectionClosed( this )
    , packetArrived( this )
    , id_( connid++ )
    , socketthread_( 0 )
    , eventloop_( 0 )
    , timeout_( 0 )
{
    if ( !sock )
	return;

    mAttachCB(socket_->disconnected,RequestConnection::connCloseCB);
    mAttachCB(socket_->readyRead,RequestConnection::dataArrivedCB);
}


RequestConnection::~RequestConnection()
{
    detachAllNotifiers();

    if ( eventloop_ )
    {
	eventloop_->exit();

	socketthread_->waitForFinish();
	deleteAndZeroPtr( socketthread_ );
    }
    else
    {
	deleteAndZeroPtr( socket_, ownssocket_ );
    }

    if ( sendqueue_.size() )
    {
        pErrMsg("Queue should be empty");
    }
}


PacketSendData::PacketSendData( const RequestPacket& packet, bool wait )
    : packet_( &packet )
    , waitforfinish_( wait )
    , sendstatus_( NotAttempted )
{}


void RequestConnection::socketThreadFunc( CallBacker* )
{
    if ( socket_ )
    {
	pErrMsg("Thread started with existing socket!");
	return;
    }

    connectToHost( true );

    mAttachCB(socket_->disconnected,RequestConnection::connCloseCB);
    mAttachCB(socket_->readyRead,RequestConnection::dataArrivedCB);
    QEventLoop* eventloop = new QEventLoop( socket_->qSocket() );

    createReceiverForCurrentThread();


    //Tell constructor we are up and running!
    eventlooplock_->lock();
    eventloop_ = eventloop;
    eventlooplock_->signal( true );
    eventlooplock_->unLock();

    eventloop_->exec();


    //Go through send queue and make sure eventual waiting threads are
    //notified
    lock_.lock();

    for ( int idx=0; idx<sendqueue_.size(); idx++ )
	sendqueue_[idx]->sendstatus_ = PacketSendData::Failed;

    lock_.signal( true );

    deepUnRef( sendqueue_ );
    lock_.unLock();

    removeReceiverForCurrentThread();
    deleteAndZeroPtr( eventloop_ );
    deleteAndZeroPtr( socket_, ownssocket_ );
}


void RequestConnection::connectToHost( bool witheventloop )
{
    if ( socket_ )
    {
	pErrMsg("I did not expect a socket" );
	return;
    }

    Network::Socket* newsocket = new Network::Socket( witheventloop );

    if ( timeout_ > 0 )
	newsocket->setTimeout( timeout_ );

    if ( newsocket->connectToHost(servername_,serverport_) )
	mAttachCB(newsocket->disconnected,RequestConnection::connCloseCB);

    Threads::MutexLocker locker( lock_ );
    socket_ = newsocket;
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


bool RequestConnection::stillTrying() const
{
    //TODO
    return false;
}


void RequestConnection::connCloseCB( CallBacker* )
{
    lock_.lock();
    lock_.signal(true);  //isOK has changed
    lock_.unLock();

    connectionClosed.trigger();
}


bool RequestConnection::readFromSocket()
{
    while ( isOK() )
    {
	RefMan<RequestPacket> nextreceived = new RequestPacket;
	Network::Socket::ReadStatus readres = socket_->read( *nextreceived );

	if ( readres==Network::Socket::ReadOK )
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
	    else
	    {
		pErrMsg("Invalid packet arrived");
	    }
	}
	else //Error or timeout
	{
	    errmsg_ = socket_->errMsg();
	    socket_->disconnectFromHost();
	    if ( errmsg_.isEmpty() )
		errmsg_ = uiStrings::phrErrDuringRead( tr("socket","network") );
	    return false;
	}

	//Break the loop when there is nothing left to read
	if ( !socket_->bytesAvailable() )
	    return true;
    }

    return false;
}


bool RequestConnection::doSendPacket( const RequestPacket& pkt,
				      bool waitforfinish )
{
    if ( !isOK() )
	return	false;

    lock_.unLock();
    const bool result = socket_->write( pkt, waitforfinish );
    lock_.lock();

    if ( !result )
	requestEnded( pkt.requestID() );

    return result;
}


void RequestConnection::sendQueueCB(CallBacker*)
{
    lock_.lock();
    RefObjectSet<PacketSendData> localqueue = sendqueue_;
    deepUnRef( sendqueue_ );
    lock_.unLock();

    for ( int idx=0; idx<localqueue.size(); idx++ )
    {
	if ( localqueue[idx]->sendstatus_!=PacketSendData::NotAttempted )
	   //Threadsafe as only I will set it
	   //apart from constructor
	    continue;

	lock_.lock();

	localqueue[idx]->sendstatus_ = doSendPacket( *localqueue[idx]->packet_,
					       localqueue[idx]->waitforfinish_ )
	     ? PacketSendData::Sent
	     : PacketSendData::Failed;

	lock_.signal( true );
	lock_.unLock();
    }
}


bool RequestConnection::sendPacket( const RequestPacket& pkt,
				    bool waitforfinish )
{
    if ( !pkt.isOK() )
	return false;

    if ( !socketthread_ && Threads::currentThread()!=socket_->thread() )
	return false;

    Threads::MutexLocker locker( lock_ );

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

    //We're non-threaded. Just send
    if ( !socketthread_ )
    {
	return doSendPacket( pkt, waitforfinish );
    }

    RefMan<PacketSendData> senddata = new PacketSendData(pkt,waitforfinish);
    sendqueue_ += senddata;
    senddata->ref(); //Class assumes all objects in sendqueue is reffed

    //Trigger thread if I'm first. If size is larger, it should already be
    //triggered
    if ( sendqueue_.size()==1 )
    {
	CallBack::addToThread( socketthread_->threadID(),
			   mCB(this, RequestConnection, sendQueueCB) );
    }

    if ( !waitforfinish )
	return true;

    while ( !socket_
	    && senddata->sendstatus_==PacketSendData::NotAttempted )
    {
	lock_.wait();
    }

    return senddata->sendstatus_==PacketSendData::Sent;
}


RefMan<RequestPacket> RequestConnection::pickupPacket( od_int32 reqid,
						int timeout,
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

    RefMan<RequestPacket> pkt = getNextAlreadyRead( reqid );
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


RefMan<RequestPacket> RequestConnection::getNextAlreadyRead( int reqid )
{
    for ( int idx=0; idx<receivedpackets_.size(); idx++ )
    {
	RequestPacket* pkg = receivedpackets_[idx];
	if ( reqid<0 || pkg->requestID()==reqid )
	{
	    receivedpackets_.removeSingle( idx );
	    return pkg;
	}
    }

    return 0;
}


RefMan<RequestPacket> RequestConnection::getNextExternalPacket()
{
    Threads::MutexLocker locker( lock_ );

    for ( int idx=0; idx<receivedpackets_.size(); idx++ )
    {
	RefMan<RequestPacket> pkt = receivedpackets_[idx];
	if ( !ourrequestids_.isPresent(pkt->requestID()) )
	{
	    receivedpackets_.removeSingle(idx);
	    return pkt;
	}
    }

    return 0;
}


void RequestConnection::requestEnded( od_int32 reqid )
{
    ourrequestids_ -= reqid;

    for ( int idx=receivedpackets_.size()-1; idx>=0; idx-- )
    {
	if ( receivedpackets_[idx]->requestID()==reqid )
	    receivedpackets_.removeSingle( idx );
    }
}


void RequestConnection::dataArrivedCB( CallBacker* cb )
{
    readFromSocket();
}


RequestServer::RequestServer( port_nr_type servport, const char* addr )
    : serverport_( servport )
    , server_( new Network::Server )
    , newConnection( this )
{
    if ( !server_ )
	return;

    mAttachCB( server_->newConnection, RequestServer::newConnectionCB );
    if ( !server_->listen(addr,serverport_) )
    {
	errmsg_ = tr("Cannot start listening on port %1").arg( serverport_ );
    }
}


RequestServer::~RequestServer()
{
    detachAllNotifiers();

    deepErase( pendingconns_ );
    deleteAndZeroPtr( server_ );
}


bool RequestServer::isOK() const
{
    return server_ && server_->isListening();
}



RequestConnection* RequestServer::pickupNewConnection()
{
    Threads::Locker locker( lock_ );
    return pendingconns_.size() ? pendingconns_.removeSingle( 0 ) : 0;
}


void RequestServer::newConnectionCB(CallBacker* cb)
{
    mCBCapsuleUnpack(int,socketid,cb);
    Network::Socket* sock = server_->getSocket(socketid);

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


bool RequestConnection::isPortFree( port_nr_type port, uiString* errmsg )
{
    const BufferString addr( System::localAddress() );
    const RequestServer reqserv( port, addr );
    const bool ret = reqserv.isOK();
    if ( errmsg && !reqserv.errMsg().isEmpty() )
	errmsg->set( reqserv.errMsg() );

    return ret;
}


static Threads::Atomic<port_nr_type> lastusableport_ = 0;


port_nr_type RequestConnection::getNextCandidatePort()
{
    if ( lastusableport_ == 0 )
	lastusableport_ = (port_nr_type)GetEnvVarIVal( "OD_START_PORT", 20049 );
    return lastusableport_ + 1;
}


port_nr_type RequestConnection::getUsablePort( port_nr_type portnr )
{
    uiRetVal uirv;
    return getUsablePort( uirv, portnr, 100 );
}


port_nr_type RequestConnection::getUsablePort( uiRetVal& uirv,
					 port_nr_type portnr, int nrtries )
{
    uiString errmsg;
    if ( portnr == 0 )
	portnr = getNextCandidatePort();

    for ( int idx=0; idx<nrtries; idx++, portnr++ )
    {
	if ( isPortFree(portnr,&errmsg) )
	    { lastusableport_ = portnr; return portnr; }
    }

    uirv = errmsg;
    return 0;
}

}; //Network
