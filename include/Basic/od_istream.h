#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "od_stream.h"
class SeparString;
class CompoundKey;
class StringView;


/*!\brief OD class for stream read */

mExpClass(Basic) od_istream : public od_stream
{
public:

			od_istream()				{}
			od_istream( const char* fnm )
			    : od_stream(fnm,false)		{}
			od_istream( const FilePath& fp )
			    : od_stream(fp,false)		{}
			od_istream( const OS::MachineCommand& mc,
				    const char* workdir=nullptr )
			    : od_stream(mc,workdir,false)	{}
			od_istream( std::istream* s )
			    : od_stream(s)			{}
			od_istream( std::istream& s )
			    : od_stream(s)			{}

			od_istream(const od_istream&) = delete;
			od_istream(od_istream&&);
    od_istream&		operator=(const od_istream&) = delete;
    od_istream&		operator=(od_istream&&);

    bool		open(const char*);
    bool		reOpen();

    od_istream&		get(char&);
			/*<!Use this function to get regular text inputs.
			  If you want to analyse an alien stream char-by-char,
			  use peek()-ignore(1) call pairs.*/

    od_istream&		get(unsigned char&);
    od_istream&		get(od_int16&);
    od_istream&		get(od_uint16&);
    od_istream&		get(od_int32&);
    od_istream&		get(od_uint32&);
    od_istream&		get(od_int64&);
    od_istream&		get(od_uint64&);
#ifdef __lux64__
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

    od_istream&		getC(char*,int sz,int maxnrchar);
    od_istream&		get(char*) = delete;
    od_istream&		get(StringView&) = delete;
    od_istream&		get(void*) = delete;

    bool		getWord(BufferString&,bool allowgotonextline=true);
    bool		getLine(BufferString&,bool* newline_found=0);
    bool		getAll(BufferString&);

    bool		getBin(void*,Count nrbytes);
    template <class T>
    od_istream&		getBin(T&);

    char		peek() const;
    void		ignore(Count);
    bool		skipUntil(char);
    bool		skipWord();
    bool		skipLine();
    void		setReadPosition(Pos,Ref r=Abs);
    Pos			endPosition() const;

    Count		lastNrBytesRead() const;
    std::istream&	stdStream();
    bool		atEOF() const;
				//!< equivalent to: !isOK() && !isBad()
				//!< avoid using it in normal circumstances

    static od_istream&	nullStream();

};



//!< common access to the std::cin
mGlobal(Basic) od_istream& od_cin();


template <class T> inline od_istream& operator >>( od_istream& s, T& t )
{ return s.get( t ); }

template <class T>
inline od_istream& od_istream::getBin( T& t )
{
    getBin( &t, sizeof(T) );
    return *this;
}
