#ifndef databuf_h
#define databuf_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		1-9-95
 RCS:		$Id: databuf.h,v 1.5 2003-11-07 12:21:51 bert Exp $
________________________________________________________________________

*/

#include <rawarray.h>


/*!\brief Raw data array with memory management. */

class DataBuffer : public RawDataArray
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
