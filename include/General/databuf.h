#ifndef databuf_h
#define databuf_h

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		1-9-95
 RCS:		$Id: databuf.h,v 1.4 2001-04-18 14:45:36 bert Exp $
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
