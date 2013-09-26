#ifndef od_istream_h
#define od_istream_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "od_stream.h"
class IOPar;
class SeparString;
class CompoundKey;
class FixedString;
class BufferString;


/*!\brief OD class for stream read */

mExpClass(Basic) od_istream : public od_stream
{
public:

    			od_istream()			{}
    			od_istream( const char* fnm )
			    : od_stream(fnm,false)	{}
    			od_istream( const FilePath& fp )
			    : od_stream(fp,false)	{}
    			od_istream( std::istream* s )
			    : od_stream(s)		{}
    			od_istream( std::istream& s )
			    : od_stream(s)		{}
			od_istream( const od_istream& s )
			    : od_stream(s)		{ *this = s; }
    od_istream&		operator =( const od_istream& s )
    			{ od_stream::operator =(s); return *this; }
    bool		open(const char*);

    od_istream&		get(char&);
    od_istream&		get(unsigned char&);
    od_istream&		get(od_int16&);
    od_istream&		get(od_uint16&);
    od_istream&		get(od_int32&);
    od_istream&		get(od_uint32&);
    od_istream&		get(od_int64&);
    od_istream&		get(od_uint64&);
    od_istream&		get(float&);
    od_istream&		get(double&);

    od_istream&		get(BufferString&);
    			//!< reads one word delimited by whitespace, "" or ''

    od_istream&		get(IOPar&);
    od_istream&		get(SeparString&);
    od_istream&		get(CompoundKey&);

    od_istream&		getC(char*,int maxnrchar);
    od_istream&		get(char*); //!< unsafe - use getC instead -> pErrMsg
    od_istream&		get(FixedString&); //!< does nothing + pErrMsg
    od_istream&		get(void*); //!< does nothing + pErrMsg

    bool		getBin(void*,Count nrbytes);
    bool		getLine(BufferString&);
    bool		getAll(BufferString&);

    char		peek() const;
    void		ignore(Count);
    bool		skipUntil(char);
    bool		skipWord();
    bool		skipLine();

    Count		lastNrBytesRead() const;
    std::istream&	stdStream();

    static od_istream&	nullStream();

};



template <class T> inline od_istream& operator >>( od_istream& s, T& t )
{ return s.get( t ); }


#endif
