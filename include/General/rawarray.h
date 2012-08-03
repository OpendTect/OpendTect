#ifndef rawarray_h
#define rawarray_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		1-9-95
 RCS:		$Id: rawarray.h,v 1.8 2012-08-03 13:00:25 cvskris Exp $
________________________________________________________________________

*/

#include "generalmod.h"
#include "general.h"


/*!\brief Raw binary data access.  */

mClass(General) RawDataArray
{
public:
			RawDataArray( int byts=4 )
			: nelem_(0), bytes_(byts), data_(0)
						{}
    inline bool		isEmpty() const		{ return !data_ || !nelem_; }

    inline int		size() const		{ return nelem_; }
    inline int		bytesPerSample() const	{ return bytes_; }
    bool		isZero() const;

    inline unsigned char*	data()		{ return data_; }
    inline const unsigned char* data() const	{ return data_; }

protected:

    int			nelem_;
    int			bytes_;
    unsigned char*	data_;

};


#endif

