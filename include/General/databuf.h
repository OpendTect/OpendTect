#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		1-9-95
________________________________________________________________________

*/

#include "generalmod.h"
#include "bufstring.h"


/*!\brief Resizable buffer of elements */

mExpClass(General) DataBuffer
{
public:

    typedef int		size_type;
    typedef int		obj_size_type;
    typedef unsigned char buf_type;

			DataBuffer(size_type nrelem,int bytsperelem=4,
				      bool setnull=false);
			~DataBuffer();
			DataBuffer( const DataBuffer& b )
			: nelem_(0), data_(0)	{ *this = b; }
    DataBuffer&		operator=(const DataBuffer&);

    inline bool		isOk() const		{ return data_ ? true : false; }
    inline bool		isEmpty() const		{ return !data_ || !nelem_; }

    inline size_type	size() const		{ return nelem_; }
    inline obj_size_type bytesPerElement() const { return elembytes_; }
    bool		isZero() const;
    inline od_int64	totalBytes() const
			{ return ((od_int64)nelem_)*elembytes_; }

    inline buf_type*	data()			{ return data_; }
    inline const buf_type* data() const		{ return data_; }

    void		reSize(size_type,bool copydata=true);
    void		reByte(obj_size_type,bool copydata=true);
    void		zero();

    bool		fitsInString() const;
    BufferString	getString() const;

protected:

    size_type		nelem_;
    obj_size_type	elembytes_;
    buf_type*		data_;

};
