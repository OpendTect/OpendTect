#ifndef tcpconnection_h
#define tcpconnection_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		August 2014
 RCS:		$Id$
________________________________________________________________________

-*/


#include "networkmod.h"
#include "callback.h"
#include "uistring.h"

#include "threadlock.h"

namespace Network
{
    class RequestPacket;
}

template <class T> class DataInterpreter;
class QTcpSocket;
class BufferString;

/*!Enables the connection and sending of binary and text data through
   a socket both with and without event-loops.

   After construction, you need to connect to a host on a port. If you choose
   not to wait then (dis-)connecting only fails if it is already in progress.

   Strings are transferred without trailing '\0', but with a leading integer
   for the size.

 */

mExpClass(Network) TcpConnection : public CallBacker
{ mODTextTranslationClass(TcpConnection);

public:

		TcpConnection(bool haveeventloop=true);
		~TcpConnection();

    void	setTimeout(int ms) { timeout_ = ms; }

    bool	connectToHost(const char* host,int port,
			      bool wait=false);
    bool	disconnectFromHost(bool wait=false);

    bool	isBad() const;
    bool	isConnected() const;
    bool	anythingToRead() const;
    uiString	errMsg() const	{ return errmsg_; }
    void	abort();	//!<Just stops all pending operations.

    bool	writeChar(char);
    bool	writeShort(short);
    bool	writeInt32(od_int32);
    bool	writeInt64(od_int64);
    bool	writeFloat(float);
    bool	writeDouble(double);
    bool	write(const Network::RequestPacket&);
    bool	write(const OD::String&);
    bool	write(const uiString&);
    int		write(const IOPar&);

    bool	writeArray(const void*,od_int64,bool wait=false);
    bool	writeShortArray(const short*,od_int64,bool wait=false);
    bool	writeInt32Array(const od_int32*,od_int64,bool wait=false);
    bool	writeInt64Array(const od_int64*,od_int64,bool wait=false);
    bool	writeFloatArray(const float*,od_int64,bool wait=false);
    bool	writeDoubleArray(const double*,od_int64,bool wait=false);

    bool	readChar(char&);
    bool	readShort(short&);
    bool	readInt32(od_int32&);
    bool	readInt64(od_int64&);
    bool	readFloat(float&);
    bool	readDouble(double&);
    bool	read(BufferString&);
    void	read(IOPar&) const;
    bool	read(Network::RequestPacket&);

    bool	readArray(void*,od_int64);
    bool	readShortArray(short*,od_int64);
    bool	readInt32Array(od_int32*,od_int64);
    bool	readInt64Array(od_int64*,od_int64);
    bool	readFloatArray(float*,od_int64);
    bool	readDoubleArray(double*,od_int64);

    Notifier<TcpConnection>	Closed; //!< usually remote host terminates.

private:

    bool			waitForConnected();
				//!<\note Lock should be unlocked when calling
    bool			waitForNewData();
				//!<\note Lock should be locked when calling
    bool			waitForWrite(bool all);
				//!<\note Lock should be locked when calling

    template <class T> bool	writeArray(const DataInterpreter<T>*,
					   const T* arr,od_int64,bool wait);
    template <class T> bool	readArray(const DataInterpreter<T>*,T*,
					  od_int64);

    mutable uiString		errmsg_;
    Threads::Lock		lock_;
    QTcpSocket*			qtcpsocket_;
    int				timeout_;
    bool			noeventloop_;

    DataInterpreter<short>*	shortinterpreter_;
    DataInterpreter<od_int32>*	od_int32interpreter_;
    DataInterpreter<od_int64>*	od_int64interpreter_;
    DataInterpreter<float>*	floatinterpreter_;
    DataInterpreter<double>*	doubleinterpreter_;

};


#endif

