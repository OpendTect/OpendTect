#ifndef od_ostream_h
#define od_ostream_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "od_iostream.h"
class IOPar;
class SeparString;
class CompoundKey;
class FixedString;
class BufferString;


/*!\brief OD class for stream write */

mExpClass(Basic) od_ostream : public od_stream
{
public:

    			od_ostream( const char* fnm )
			    : od_stream(fnm,true)	{}
    			od_ostream( const FilePath& fp )
			    : od_stream(fp,true)	{}
    			od_ostream( std::ostream* s )
			    : od_stream(s)		{}
    			od_ostream( std::ostream& s )
			    : od_stream(s)		{}

    od_ostream&		add(const char*);
    od_ostream&		add(od_int16);
    od_ostream&		add(od_uint16);
    od_ostream&		add(od_int32);
    od_ostream&		add(od_uint32);
    od_ostream&		add(od_int64);
    od_ostream&		add(od_uint64);
    od_ostream&		add(float);
    od_ostream&		add(double);

    od_ostream&		add(const BufferString&);
    od_ostream&		add(const FixedString&);
    od_ostream&		add(const IOPar&);
    od_ostream&		add(const SeparString&);
    od_ostream&		add(const CompoundKey&);

    od_ostream&		add(const void*,od_uint64 nrbytes);

    std::ostream&	stdStream();

private:

    od_ostream&		operator =(const od_ostream&);

};


template <class T> inline od_ostream& operator <<( od_ostream& s, const T& t )
{ return s.add( t ); }


#endif
