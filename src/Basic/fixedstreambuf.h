#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include <streambuf>
#include <istream>
#include <ostream>


namespace std
{

/*!
\brief Adapter to use a fixed buffer as a stream.

  Usage like:

  std::fixedstreambuf fsb( mybuf, mybufsize );
  std::istream strm( &fsb );

  Tested for istream with char* only.
*/

mClass(Basic) fixedstreambuf : public streambuf
{
public:

fixedstreambuf( char_type* b, off_type sz, bool manbuf=false )
    : mine_(manbuf)
    , newpos_(0)
{
    setbuf( b, sz );
}

~fixedstreambuf()
{
    if ( mine_ )
	delete [] buf_;
}

fixedstreambuf* setbuf( char_type* b, streamsize n ) override
{
    buf_ = b; sz_ = n;
    setg( buf_, buf_, buf_ + sz_ );
    setp( buf_, buf_ + sz_ );

    return this;
}

pos_type seekoff( off_type offs, ios_base::seekdir sd,
		  ios_base::openmode which ) override
{
    newpos_ = offs;
    if ( sd == ios_base::cur )
	newpos_ += which == ios_base::in ? gptr() - buf_ : pptr() - buf_;
    else if ( sd == ios_base::end )
	newpos_ = sz_ + newpos_;

    return seekpos( newpos_, which );
}

pos_type seekpos( pos_type newpos, ios_base::openmode which ) override
{
    if ( newpos_ < 0 || newpos_ >= sz_ )
	newpos_ = -1;
    else if ( which == ios_base::in )
	setg( buf_, buf_+newpos_, buf_ + sz_ );
    else
	setp( buf_+newpos_, buf_ + sz_ );

    return newpos_;
}

streamsize xsgetn( char_type* s, streamsize n ) override
{
    streamsize toget = n;
    const od_int64 memsz = epptr() - pptr();
    if ( toget > memsz && memsz >= 0  )
	toget = memsz;

    OD::memCopy( s, gptr(), (size_t)toget );
    gbump( (int) toget );

    return toget;
}

streamsize xsputn( const char_type* s, streamsize n ) override
{
    streamsize toput = n;
    const od_int64 memsz = epptr() - pptr();
    if ( toput > memsz && memsz >= 0  )
	toput = memsz;

    OD::memCopy( pptr(), s, (size_t)toput );
    pbump( (int)toput );

    return toput;
}

    char_type*	buf_;
    off_type	sz_;
    bool	mine_;
    off_type	newpos_;
};

class fixedistream : public istream
{
public:

fixedistream( fixedstreambuf* sb )
    : istream(sb)
{}

~fixedistream()
{ delete rdbuf(); }

};

class fixedostream : public ostream
{
public:

fixedostream( fixedstreambuf* sb )
    : ostream(sb)
{}

~fixedostream()
{ delete rdbuf(); }

};

} // namespace std
