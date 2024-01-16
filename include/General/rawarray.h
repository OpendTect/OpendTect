#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "commondefs.h"


/*!\brief Raw binary data access.  */

mExpClass(General) RawDataArray
{
public:
			~RawDataArray();

    inline bool		isEmpty() const		{ return !data_ || !nelem_; }

    inline int		size() const		{ return nelem_; }
    inline int		bytesPerSample() const	{ return bytes_; }
    bool		isZero() const;

    inline unsigned char*	data()		{ return data_; }
    inline const unsigned char* data() const	{ return data_; }

protected:
			RawDataArray(int byts=4);

    int			nelem_				= 0;
    int			bytes_;
    unsigned char*	data_				= nullptr;

};
