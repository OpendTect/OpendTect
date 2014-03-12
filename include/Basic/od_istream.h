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
class SeparString;
class CompoundKey;
class FixedString;


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
			    : od_stream(s)		{}
    od_istream&		operator =( const od_istream& s )
			    { od_stream::operator =(s); return *this; }
    bool		open(const char*);
    bool		reOpen();

    od_istream&		get(char&);
    od_istream&		get(unsigned char&);
    od_istream&		get(od_int16&);
    od_istream&		get(od_uint16&);
    od_istream&		get(od_int32&);
    od_istream&		get(od_uint32&);
    od_istream&		get(od_int64&);
    od_istream&		get(od_uint64&);
#ifdef __lux__
    od_istream&		get(long long&);
    od_istream&		get(unsigned long long&);
#else
    od_istream&		get(long&);
    od_istream&		get(unsigned long&);
#endif
    od_istream&		get(float&);
    od_istream&		get(double&);

    od_istream&		get( BufferString& bs, bool allowgotonextline=true )
			{ getWord(bs,allowgotonextline); return *this; }

    od_istream&		get(IOPar&);
    od_istream&		get(SeparString&);
    od_istream&		get(CompoundKey&);

    od_istream&		getC(char*,int maxnrchar);
    od_istream&		get(char*); //!< unsafe - use getC instead -> pErrMsg
    od_istream&		get(FixedString&); //!< does nothing + pErrMsg
    od_istream&		get(void*); //!< does nothing + pErrMsg

    bool		getWord(BufferString&,bool allowgotonextline=true);
    bool		getLine(BufferString&,bool* newline_found=0);
    bool		getAll(BufferString&);

    bool		getBin(void*,Count nrbytes);

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
