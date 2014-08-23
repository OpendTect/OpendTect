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

#define mRequestPacketHeaderSize		10

namespace Network
{

mExpClass(Network) RequestPacket
{
public:
			RequestPacket(od_int32 payloadsize=-1);
			~RequestPacket();

    static od_int32	getHeaderSize() { return sizeof(Header); }
    bool		isOK() const;
			//!<Reads the header and checks that it is reasonable

    od_int32		getRequestID() const;
    void		setRequestID(od_int32);

    od_int16		getSubID() const;
    void		setSubID(od_int16);

    od_int32		getPayloadSize() const;
    void		setStringPayload(const char*);
    void		setPayload(void*);
			//!<Size should be identical to getPayloadSize
    void		setPayload(void*,od_int32 size);
			//!<Becomes mine
    void*		getPayload(bool takeover=false);

    void*		getRawHeader()	{ return header_.int32s_; }

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

