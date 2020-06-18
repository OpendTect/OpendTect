#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2013
________________________________________________________________________

-*/

#include "basicmod.h"
#include "od_stream.h"

class SeparString;
class CompoundKey;
class FixedString;


/*!\brief OD class for stream write */

mExpClass(Basic) od_ostream : public od_stream
{
public:

			od_ostream()				{}
			od_ostream( const char* fnm, bool useexist=false )
			    : od_stream(fnm,true,useexist)	{}
			od_ostream( const File::Path& fp, bool useexist=false )
			    : od_stream(fp,true,useexist)	{}
			od_ostream( const OS::MachineCommand& mc,
				    const char* workdir=nullptr )
			    : od_stream(mc,workdir,true)	{}
			od_ostream( std::ostream* s )
			    : od_stream(s)			{}
			od_ostream( std::ostream& s )
			    : od_stream(s)			{}
			od_ostream(const od_ostream&) = delete;
			od_ostream(od_ostream&&);
    od_ostream&		operator=(const od_ostream&) = delete;
    od_ostream&		operator=(od_ostream&&);

    bool		open(const char*,bool useexist=false);

    od_ostream&		add(char);
    od_ostream&		add(unsigned char);
    od_ostream&		add(const char*);
    od_ostream&		add(od_int16);
    od_ostream&		add(od_uint16);
    od_ostream&		add(od_int32);
    od_ostream&		add(od_uint32);
    od_ostream&		add(od_int64);
    od_ostream&		add(od_uint64);
#ifdef __lux64__
    od_ostream&		add(long long);
    od_ostream&		add(unsigned long long);
#else
    od_ostream&		add(long);
    od_ostream&		add(unsigned long);
#endif
    //add clean numbers to the string, no 1.50000001 but 1.5
    od_ostream&		add(float);
    od_ostream&		add(double);

    //add precise numbers to the string, 1.50000001 is left as is.
    od_ostream&		addPrecise(float);
    od_ostream&		addPrecise(double);

    od_ostream&		add(const OD::String&);
    od_ostream&		add(const IOPar&);
    od_ostream&		add(const SeparString&);
    od_ostream&		add(const CompoundKey&);
    od_ostream&		add(const DBKey&);
    od_ostream&		addTab()		{ return add( "\t" ); }
    od_ostream&		addNewLine()		{ return add( "\n" ); }

    od_ostream&		add(const void*); //!< produces pErrMsg but works
    od_ostream&		addPtr(const void*);
    od_ostream&		add(od_istream&);
    od_ostream&		add( od_ostream& )	{ return *this; }

    bool		addBin(const void*,Count nrbytes);
    template <class T>
    od_ostream&		addBin(const T&);

    std::ostream&	stdStream();
    void		flush();

    static od_ostream&	nullStream();
    static od_ostream&	logStream(); //!< used by ErrMsg and UsrMsg

    void		setWritePosition(Pos,Ref r=Abs);
    Pos			lastWrittenPosition() const;
};


//!< common access to the user log file, or std::cout in other than od_main
inline od_ostream& od_cout()
{
    return od_ostream::logStream();
}


//!< Never redirected
mGlobal(Basic) od_ostream& od_cerr();



template <class T>
inline od_ostream& operator <<( od_ostream& s, const T& t )
{
    return s.add( t );
}


inline od_ostream& od_endl( od_ostream& strm )
{
    strm.add( od_newline ).flush();
    return strm;
}


std::ostream& od_endl(std::ostream&)	= delete;


typedef od_ostream& (*od_ostreamFunction)(od_ostream&);
inline od_ostream& operator <<( od_ostream& s, od_ostreamFunction fn )
{
    return (*fn)( s );
}


template <class T>
inline od_ostream& od_ostream::addBin( const T& t )
{
    addBin( &t, sizeof(T) );
    return *this;
}
