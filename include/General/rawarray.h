#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		1-9-95
________________________________________________________________________

*/

#include "generalmod.h"
#include "gendefs.h"


/*!\brief Raw binary data access.  */

mExpClass(General) RawDataArray
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


