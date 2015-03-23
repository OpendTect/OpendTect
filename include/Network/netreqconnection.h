#ifndef netreqcommunic_h
#define netreqcommunic_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		August 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "networkmod.h"
#include "gendefs.h"
#include "thread.h"
#include "objectset.h"
#include "callback.h"
#include "uistring.h"


namespace Network
{

class Socket;
class Server;
class RequestPacket;


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
			RequestConnection(const char* servername,
					  unsigned short serverport,
					  bool haveeventloop=true,
					  int connectiontimeout=-1);
			//!<Initiates communications
			RequestConnection(Socket*);
			/*!<Socket does NOT become mine. Socket should be
			    connected to whomever is my counterpart. */

			~RequestConnection();

    bool		isOK() const;
    const char*		server() const		{ return servername_; }
    unsigned short	port() const		{ return serverport_; }
    int			ID() const		{ return id_; }

    bool		sendPacket(const RequestPacket&,
				   bool waitforfinish=false);

    RequestPacket*	pickupPacket(od_int32 reqid,int timeout /* in ms */,
					int* errorcode=0);
    RequestPacket*	getNextExternalPacket();

    static int		cInvalidRequest()	{ return 1; }
    static int		cTimeout()		{ return 2; }
    static int		cDisconnected()		{ return 3; }

    Socket*		socket()		{ return socket_; }

    CNotifier<RequestConnection,od_int32> packetArrived;
    Notifier<RequestConnection> connectionClosed;

    uiString		errMsg() const		{ return errmsg_; }

private:


    void			threadFunc(CallBacker*);

    uiString			errmsg_;

    TypeSet<od_int32>		ourrequestids_;
    ObjectSet<RequestPacket>	receivedpackets_;

    Threads::ConditionVar	lock_;
    Socket*			socket_;
    bool			ownssocket_;

    int				id_;

    BufferString		servername_;
    unsigned short		serverport_;

    void			connectToHost(bool haveeventloop,int);
    void			connCloseCB(CallBacker*);
    void			newConnectionCB(CallBacker*);
    void			dataArrivedCB(CallBacker*);

    bool			readFromSocket();
    Network::RequestPacket*	readConnection(int);
    Network::RequestPacket*	getNextAlreadyRead(int);
    void			requestEnded(od_int32);
};



/*! Sets up a listening service at the port and creates
    Network::RequestConnections when there is a connection.
    */

mExpClass(Network) RequestServer : public CallBacker
{ mODTextTranslationClass(RequestServer);
public:
				RequestServer(unsigned short serverport);
				~RequestServer();

    bool			isOK() const;
    Server*			server()		{ return server_; }

    Notifier<RequestServer>	newConnection;
    RequestConnection*		pickupNewConnection();
				//!Becomes yours.

    uiString			errMsg() const		{ return errmsg_; }

private:

    void				newConnectionCB(CallBacker*);

    uiString				errmsg_;

    ObjectSet<RequestConnection>	pendingconns_;

    Threads::Lock			lock_;
    unsigned short			serverport_;
    Server*				server_;
};


}; //Namespace


#endif
