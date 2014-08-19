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

#include "thread.h"

template <class T> class DataInterpreter;
class QTcpSocket;
class BufferString;

/*!Enables the connection and sending of binary and text data through
   a socket both with and without event-loops. */

mExpClass(Network) TcpConnection : public CallBacker
{ mODTextTranslationClass(TcpConnection);

public:
		TcpConnection();
		~TcpConnection();

    void	setTimeout(int ms) { timeout_ = ms; }

    void	setNoEventLoop(bool yn);
		/*!<If program does not have an event-loop, set to true.
		    Default is false. */

    bool	connectToHost(const char* host,int port,
			      bool wait=false);
		/*!<If wait is false, it only returns false if already
		    connected. Otherwise, errors will come when
		    attempting to read/write. */

    bool	disconnectFromHost(bool wait=false);
		/*!<If wait is false, it only returns false if already
		    dis-connecting. Otherwise, errors will come when
		    attempting to do other things. */

    void	abort();
		//!<Just stops all pending operations.

    bool	writeChar(char);
    bool	writeShort(short);
    bool	writeInt32(od_int32);
    bool	writeInt64(od_int64);
    bool	writeFloat(float);
    bool	writeDouble(double);
    bool	write(const OD::String&);
		/*!<Writes short with size followed by buffer
		    (without 0 at the end). */
    bool	write(const uiString&);
		/*!<Writes short with size followed by wchar buffer
		    (without 0 at the end). */
    int		write(const IOPar&);

    bool	writeArray(const char*,od_int64,bool wait=false);
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
    bool	read(BufferString&); //Until NULL arrives
    void	read(IOPar&) const;

    bool	readArray(char*,od_int64);
    bool	readShortArray(short*,od_int64);
    bool	readInt32Array(od_int32*,od_int64);
    bool	readInt64Array(od_int64*,od_int64);
    bool	readFloatArray(float*,od_int64);
    bool	readDoubleArray(double*,od_int64);

    uiString	errorMsg() const;

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

    Threads::Mutex		lock_;

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

