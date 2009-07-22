#ifndef databuf_h
#define databuf_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		1-9-95
 RCS:		$Id: databuf.h,v 1.7 2009-07-22 16:01:15 cvsbert Exp $
________________________________________________________________________

*/

#include <rawarray.h>


/*!\brief Raw data array with memory management. */

mClass DataBuffer : public RawDataArray
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
