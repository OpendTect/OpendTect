#ifndef strmio_h
#define strmio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id: strmio.h,v 1.1 2010-02-22 04:52:00 cvsnanne Exp $
________________________________________________________________________

-*/

#include "commondefs.h"
#include "plftypes.h"
#include <iostream>

class IOPar;

/*!\brief Class for simple ascii/binary stream read/write */

mClass StreamIO
{
public:
    				StreamIO( std::ostream& s,bool binary )
				    : ostrm_(&s), istrm_(0), binary_(binary) {}

				StreamIO( std::istream& s, bool binary )
				    : istrm_(&s), ostrm_(0), binary_(binary) {}

    void			writeInt16(const od_int16&,char post='\t');
    void			writeInt32(const od_int32&,char post='\t');
    void			writeInt64(const od_int64&,char post='\t');
    void			writeFloat(const float&,char post='\t');

    virtual od_int16		readInt16() const;
    virtual od_int32		readInt32() const;
    virtual od_int64		readInt64() const;
    virtual float		readFloat() const;

    virtual void		fillPar(IOPar&) const		{}
    virtual bool		usePar(const IOPar&)		{ return true; }

protected:

    std::ostream*		ostrm_;
    std::istream*		istrm_;
    bool			binary_;
};

#endif
