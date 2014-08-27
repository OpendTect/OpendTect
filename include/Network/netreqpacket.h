#ifndef netreqpacket_h
#define netreqpacket_h

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
class BufferString;

#define mRequestPacketHeaderSize		10


namespace Network
{

/*\brief Standardized packet that can be sent over a Tcp connection

  The request header contains a unique (int) request ID and a (short) sub-ID.
  The sub-ID is a flag that can be used to implement your protocol. Negative
  sub-IDs are special and cannot be freely used.

  */

mExpClass(Network) RequestPacket
{
public:
			RequestPacket(od_int32 payloadsize=-1);
			~RequestPacket();

    bool		isOK() const;
			//!< checks whether the header is reasonable

    static od_int32	headerSize() { return sizeof(Header); }
    static od_int32	getPayloadSize(const void*);

    int			setIsNewRequest(); //!< conveniently returns reqID()
    bool		isNewRequest() const { return subID()==cBeginSubID(); }
    void		setIsRequestEnd(int reqid);
    bool		isRequestEnd() const { return subID()==cEndSubID(); }

    od_int32		requestID() const;
    void		setRequestID(od_int32);
    od_int16		subID() const;
    void		setSubID(od_int16);

    od_int32		payloadSize() const;
    const void*		payload() const;
    void*		payload(bool takeover=false);
			/*!< if takeover, cast to char*, and delete with []. */
    void		getStringPayload(BufferString&) const;

    void		setPayload(void*);
			/*!< should be allocated as new char[payloadSize()] */
    void		setPayload(void*,od_int32 size);
			//!< becomes mine
    void		setStringPayload(const char*);

    static od_int16	cBeginSubID()		{ return -2; }
    static od_int16	cEndSubID()		{ return -1; }
    static od_int16	cNormalSubID()	{ return 0; }

    void*		getRawHeader()		{ return header_.int32s_; }
    const void*		getRawHeader() const	{ return header_.int32s_; }

protected:

    union Header
    {
	od_int32	int32s_[2];
	od_int16	int16s_[5]; //only number 4 is used
    };


    Header		header_;
    char*		payload_;

};


}; //Namespace


#endif
