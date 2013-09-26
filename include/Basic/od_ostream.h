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
#include "od_stream.h"
class IOPar;
class SeparString;
class CompoundKey;
class FixedString;
class BufferString;


/*!\brief OD class for stream write */

mExpClass(Basic) od_ostream : public od_stream
{
public:

    			od_ostream()			{}
    			od_ostream( const char* fnm )
			    : od_stream(fnm,true)	{}
    			od_ostream( const FilePath& fp )
			    : od_stream(fp,true)	{}
    			od_ostream( std::ostream* s )
			    : od_stream(s)		{}
    			od_ostream( std::ostream& s )
			    : od_stream(s)		{}
			od_ostream( const od_ostream& s )
			    : od_stream(s)		{ *this = s; }
    od_ostream&		operator =( const od_ostream& s )
    			{ od_stream::operator =(s); return *this; }
    bool		open(const char*);

    od_ostream&		add(char);
    od_ostream&		add(unsigned char);
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

    od_ostream&		add(const void*); //!< produces pErrMsg but works
    od_ostream&		addPtr(const void*);

    bool		addBin(const void*,Count nrbytes);
    std::ostream&	stdStream();
    void		flush();

    static od_ostream&	nullStream();
    static od_ostream&	logStream(); //!< used by ErrMsg and UsrMsg

};


template <class T>
inline od_ostream& operator <<( od_ostream& s, const T& t )
{ return s.add( t ); }


#define od_tab '\t'
#define od_newline '\n'


#endif
