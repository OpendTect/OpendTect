#ifndef databuf_H
#define databuf_H

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		1-9-95
 Contents:	A Data trace is an XFunction with iteration
 RCS:		$Id: databuf.h,v 1.2 2001-02-13 17:15:57 bert Exp $
________________________________________________________________________

*/

#include <general.h>


/*!\brief Raw data buffer.

Management of memory (resize) in terms of elements of a certain size in bytes.

*/


class DataBuffer
{
public:
			DataBuffer(int n,int byts=4,bool setnull=NO);
			DataBuffer( const DataBuffer& db )
			: nelem_(0), bytes_(0), data_(0)	{ *this = db; }
			~DataBuffer();
    DataBuffer&  	operator=(const DataBuffer&);

    inline bool		isOk() const		{ return data_ ? true : false; }
    inline int		size() const		{ return nelem_; }
    inline int		bytesPerSample() const	{ return bytes_; }
    bool		isZero() const;

    void		reSize(int);
    void		reByte(int);
    void		zero();

    inline unsigned char*	data()		{ return data_; }
    inline const unsigned char* data() const	{ return data_; }
    inline unsigned char* operator[]( int idx )
				{ return data_ + (bytes_ * idx); }
    inline const unsigned char* operator[]( int idx ) const
				{ return data_ + (bytes_ * idx); }

protected:

    int			nelem_;
    int			bytes_;
    unsigned char*	data_;

};


#endif
