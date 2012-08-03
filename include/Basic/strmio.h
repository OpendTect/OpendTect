#ifndef strmio_h
#define strmio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id: strmio.h,v 1.4 2012-08-03 13:00:15 cvskris Exp $
________________________________________________________________________

-*/

#include "basicmod.h"
#include "commondefs.h"
#include "plftypes.h"
#include <iostream>

class IOPar;

/*!\brief Class for simple ascii/binary stream read/write */

mClass(Basic) StreamIO
{
public:
    				StreamIO( std::ostream& s,bool binary )
				    : ostrm_(&s), istrm_(0), binary_(binary) {}

				StreamIO( std::istream& s, bool binary )
				    : istrm_(&s), ostrm_(0), binary_(binary) {}

    bool			writeInt16(const od_int16&,
	    				   const char* post="\t");
    bool			writeInt32(const od_int32&,
	    				   const char* post="\t");
    bool			writeInt64(const od_int64&,
	    				   const char* post="\t");
    bool			writeFloat(const float&,
	    				   const char* post="\t");

    virtual od_int16		readInt16() const;
    virtual od_int32		readInt32() const;
    virtual od_int64		readInt64() const;
    virtual float		readFloat() const;

    od_int64			tellg() const;
    od_int64			tellp() const;
    bool			seekg(od_int64,
	    			    std::ios_base::seekdir=std::ios_base::beg);
    bool			seekp(od_int64,
	    			    std::ios_base::seekdir=std::ios_base::beg);

    virtual void		fillPar(IOPar&) const		{}
    virtual bool		usePar(const IOPar&)		{ return true; }

protected:

    std::ostream*		ostrm_;
    std::istream*		istrm_;
    bool			binary_;
};

#endif

