#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "networkcommon.h"

class QEventLoop;

namespace Network
{

class Server;
class Socket;
class RequestPacket;
struct PacketSendData;


/*\brief
  Manages RequestPackets by sending/receiveing them using a Tcp Connection.

  To send a packet, make the RequestPacket and use sendPacket().

  To receive, you want only packets for your request ID. If you set a timeout
  the connection will wait for a packet with your ID if there wasn't already
  one present. On success, the returned packet is yours.

  If the other side can also send requests to us, then you need to periodically
  fetch your packets until getNextExternalPacket() returns null.

 */


mExpClass(Network) RequestConnection : public CallBacker
{ mODTextTranslationClass(RequestConnection);
public:
			RequestConnection(const Authority&,
					  bool multithreaded=true,
					  int connectiontimeout=-1);
			//!<Initiates communications
			RequestConnection(Socket*);
			/*!<Socket does NOT become mine. Socket should be
			    connected to whomever is my counterpart. */

			~RequestConnection();

    bool		isOK() const;		//!< is the conn usable?
    bool		stillTrying() const;	//!< if not OK, may it become?
    BufferString	server() const;
    PortNr_Type		port() const;
    int			ID() const		{ return id_; }

    bool		sendPacket(const RequestPacket&,
				   bool waitforfinish=false);
			/*!<Must be called from same thread as construcor unless
			    'multithreaded' flag was set on constructor.
			*/

    RefMan<RequestPacket> pickupPacket(od_int32 reqid,int timeout /* in ms */,
				     int* errorcode=nullptr);
			/*!<Must be called from same thread as construcor unless
			    'multithreaded' flag was set on constructor.
			*/

    RefMan<RequestPacket> getNextExternalPacket();

    static int		cInvalidRequest()	{ return 1; }
    static int		cTimeout()		{ return 2; }
    static int		cDisconnected()		{ return 3; }

    bool		isMultiThreaded()	{ return socketthread_; }
    Socket*		socket()		{ return socket_; }

    CNotifier<RequestConnection,od_int32> packetArrived;
    Notifier<RequestConnection> connectionClosed;

    uiString		errMsg() const		{ return errmsg_; }

private:

    mutable uiString		errmsg_;

    TypeSet<od_int32>		ourrequestids_;
    ObjectSet<RequestPacket>	receivedpackets_;

    Threads::ConditionVar	lock_;
    Socket*			socket_;
    int				timeout_;
    bool			ownssocket_;

    int				id_;

    Authority*			authority_ = nullptr;

    Threads::Thread*		socketthread_	= nullptr;
    QEventLoop*			eventloop_	= nullptr;
    Threads::ConditionVar*	eventlooplock_	= nullptr;

    ObjectSet<PacketSendData>	sendqueue_;
    void			sendQueueCB(CallBacker*);
				//Called from socketthread

    void			socketThreadFunc(CallBacker*);
    void			connectToHost( bool witheventloop );
    void			connCloseCB(CallBacker*);
    void			newConnectionCB(CallBacker*);
    void			dataArrivedCB(CallBacker*);

    bool			doSendPacket(const RequestPacket&,
				   bool waitforfinish);

    bool			readFromSocket();
    bool			writeToSocket();

    RefMan<RequestPacket>	getNextAlreadyRead(int);
    void			requestEnded(od_int32);

    friend struct		PacketSendData;
    friend class		RequestConnectionSender;
};



/*! Sets up a listening service at the port and creates
    Network::RequestConnections when there is a connection.
    */

mExpClass(Network) RequestServer : public CallBacker
{ mODTextTranslationClass(RequestServer);
public:

				RequestServer(const char* servernm);
				RequestServer(PortNr_Type,SpecAddr=Any);
				RequestServer(const Authority&);
				~RequestServer();

    bool			isOK() const;
    Server*			server()		{ return server_; }
    Authority			getAuthority() const;
				//Just what is needed for a connection

    Notifier<RequestServer>	newConnection;
    RequestConnection*		pickupNewConnection();
				//!Becomes yours.

    uiString			errMsg() const		{ return errmsg_; }

private:

    void			newConnectionCB(CallBacker*);

    uiString			errmsg_;

    ObjectSet<RequestConnection> pendingconns_;

    Threads::Lock		lock_;
    Server*			server_;

    uiString			TCPErrMsg() const;
    uiString			LocalErrMsg() const;

};

} // namespace Network
