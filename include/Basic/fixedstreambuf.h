#ifndef fixedstreambuf_h
#define fixedstreambuf_h
/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Feb 2009
 RCS:		$Id: fixedstreambuf.h,v 1.1 2009-02-05 10:54:41 cvsbert Exp $
________________________________________________________________________

*/

#include <streambuf>


namespace std
{

/*!\brief Adapter to use a fixed buffer as a stream.

  Usage like:

  std::fixedstreambuf fsb( mybuf, mybufsize );
  std::istream strm( &fsb );

  Tested for istream with char* only.

*/

class fixedstreambuf : public streambuf
{
public:

fixedstreambuf( char_type* b, off_type sz, bool manbuf=false )
    : mine_(manbuf)
{
    setbuf( b, sz );
}

~fixedstreambuf()
{
    if ( mine_ )
	delete [] buf_;
}

virtual fixedstreambuf* setbuf( char_type* b, streamsize n )
{
    buf_ = b; sz_ = n;
    setg( buf_, buf_, buf_ + sz_ );
    setp( buf_, buf_ + sz_ );
}

virtual pos_type seekoff( off_type offs, ios_base::seekdir sd,
			  ios_base::openmode which )
{
    pos_type newpos = offs;
    if ( sd == ios_base::cur )
	newpos += which == ios_base::in ? gptr() - buf_ : pptr() - buf_;
    else if ( sd == ios_base::end )
	newpos = sz_ - 1 - newpos;

    return seekpos( newpos, which );
}


virtual pos_type seekpos( pos_type newpos, ios_base::openmode which )
{
    if ( newpos < 0 || newpos >= sz_ )
	newpos = -1;
    else if ( which == ios_base::in )
	setg( buf_, buf_+newpos, buf_ + sz_ );
    else
	setp( buf_+newpos, buf_ + sz_ );
    return newpos;
}

streamsize xsgetn( char_type* s, streamsize n )
{
    streamsize toget = n;
    if ( toget > (egptr() - gptr()) )
	toget = egptr() - gptr();
    memcpy( s, gptr(), toget );
}

streamsize xsputn( const char_type* s, streamsize n )
{
    streamsize toput = n;
    if ( toput > (epptr() - pptr()) )
	toput = epptr() - pptr();
    memcpy( pptr(), s, toput );
}

    char_type*	buf_;
    off_type	sz_;
    bool	mine_;

};

} // namespace std


#endif
