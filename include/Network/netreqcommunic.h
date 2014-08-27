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


mExpClass(Network) RequestCommunicator : CallBacker
{
public:
			RequestCommunicator(const char* servername,
					    short serverport);
			~RequestCommunicator();

    bool		isOK();

    od_int32		getNewRequestID();

    bool		sendPacket(const RequestPacket&);

    RequestPacket*	pickupPacket(od_int32 reqid,
				     int* errorcode=0,
				     int timeout=0 /* in ms */ );
			/*!<If packet is returned, it's your's.
			    If timeout is more than 0, process will block
			    until packet has arrived, or timeout is reached.
			    If no packet has arrived, errorcode is filled with:
			    0 - no error,
			    1-timeout,
			    2-Request encountered an error. */
    static int		cCancelled()		{ return 2; }
    static int		cTimeout()		{ return 1; }

    void		reportFinished(od_int32);
			/*!<Let me know when you have received everything.*/
    void		cancelRequest(od_int32);

private:

    void			cancelRequestInternal(od_int32);

    TypeSet<od_int32>		activerequestids_;
    ObjectSet<RequestPacket>	receivedpackets_;

    Threads::ConditionVar	lock_;
    TcpConnection*		tcpsocket_;

    BufferString		servername_;
    short			serverport_;
};


}; //Namespace

#endif

