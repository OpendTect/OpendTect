#ifndef databuf_h
#define databuf_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		1-9-95
 RCS:		$Id: databuf.h,v 1.9 2012-08-03 13:00:22 cvskris Exp $
________________________________________________________________________

*/

#include "generalmod.h"
#include "rawarray.h"


/*!\brief Raw data array with memory management. */

mClass(General) DataBuffer : public RawDataArray
{
public:
			DataBuffer(int n,int byts=4,bool setnull=false);
			~DataBuffer();
			DataBuffer( const DataBuffer& b )
			: RawDataArray(0)			{ *this = b; }
    DataBuffer&  	operator=(const DataBuffer&);

    inline bool		isOk() const		{ return data_ ? true : false; }

    void		reSize(int,bool copydata=true);
    void		reByte(int,bool copydata=true);
    void		zero();

};


#endif

