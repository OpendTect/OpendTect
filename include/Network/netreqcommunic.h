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

class TcpConnection;


namespace Network
{

class RequestPacket;


/*\brief
  Manages RequestPackets by sending/receiveing them using a Tcp Connection.

  To send a packet, make the RequestPacket and use sendPacket().

  To receive, you want only packets for your request ID. If you set a timeout
  the communicator will wait for a packet with your ID if there wasn't already
  one present. On success, the returned packet is yours.

  If the other side can also send requests to us, then you need to periodically
  fetch your packets until getNextExternalPacket() returns null.

 */


mExpClass(Network) RequestCommunicator : public CallBacker
{
public:
			RequestCommunicator(const char* servername,
					    int serverport);
			~RequestCommunicator();

    bool		isOK() const;

    bool		sendPacket(const RequestPacket&);

    RequestPacket*	pickupPacket(od_int32 reqid,int timeout /* in ms */,
					int* errorcode=0);
    RequestPacket*	getNextExternalPacket();

    static int		cInvalidRequest()	{ return 1; }
    static int		cTimeout()		{ return 2; }

    TcpConnection*	tcpConnection()		{ return tcpconn_; }

    Notifier<RequestCommunicator> connectionClosed;

private:

    TypeSet<od_int32>		ourrequestids_;
    ObjectSet<RequestPacket>	receivedpackets_;

    Threads::Lock		lock_;
    TcpConnection*		tcpconn_;

    BufferString		servername_;
    short			serverport_;

    void			connectToHost();
    void			connCloseCB(CallBacker*);
    Network::RequestPacket*	readConnection(int);
    Network::RequestPacket*	getNextAlreadyRead(int);
    void			requestEnded(od_int32);

};


}; //Namespace


#endif
