/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stringbuilder.h"

#ifndef OD_NO_QT
# include <QString>
#endif


StringBuilder::StringBuilder( const char* inpstr )
{
    add( inpstr );
}


StringBuilder& StringBuilder::operator =( const StringBuilder& oth )
{
    if ( this == &oth )
	return *this;

    delete [] buf_; buf_ = nullptr;
    if ( oth.buf_ )
	{ mTryAlloc( buf_, char [oth.bufsz_] ); }

    if ( !buf_ )
	bufsz_ = curpos_ = 0;
    else
    {
	OD::memCopy( buf_, oth.buf_, oth.curpos_+1 );
	bufsz_ = oth.bufsz_;
	curpos_ = oth.curpos_;
    }

    return *this;
}


bool StringBuilder::operator ==( const StringBuilder& oth ) const
{
    return StringView(buf_) == StringView(oth.buf_);
}


bool StringBuilder::operator !=( const StringBuilder& oth ) const
{
    return StringView(buf_) != StringView(oth.buf_);
}


StringBuilder& StringBuilder::setEmpty()
{
    delete [] buf_; buf_ = nullptr;
    bufsz_ = curpos_ = 0;
    return *this;
}


StringBuilder& StringBuilder::set( const char* inpstr )
{
    if ( inpstr != buf_ )
    {
	setEmpty();
	add( inpstr );
    }
    return *this;
}


StringBuilder& StringBuilder::add( char ch, size_type nr )
{
    const char chstr[] = { ch, '\0' };
    for ( int idx=0; idx<nr; idx++ )
	add( chstr );
    return *this;
}


bool StringBuilder::setBufSz( size_type newsz, bool cp_old )
{
    char* newbuf;
    mTryAlloc( newbuf, char [newsz] );
    if ( !newbuf )
	return false;

    if ( buf_ )
    {
	if ( cp_old )
	    OD::memCopy( newbuf, buf_, curpos_ );
	delete [] buf_;
    }

    buf_ = newbuf;
    bufsz_ = newsz;
    return true;
}


StringBuilder& StringBuilder::add( const char* addstr )
{
    if ( !addstr || !*addstr )
	return *this;

    if ( !buf_ && !setBufSz(256,false) )
	return *this;

    char* myptr = buf_ + curpos_;
    while ( *addstr )
    {
	*myptr++ = *addstr++;
	curpos_++;
	if ( curpos_ >= bufsz_ )
	{
	    if ( !setBufSz(bufsz_*2,true) )
		return *this;
	    myptr = buf_ + curpos_;
	}
    }

    *myptr = '\0';
    return *this;
}


StringBuilder& StringBuilder::add( const mQtclass(QString)& qstr )
{
#ifdef OD_NO_QT
    return *this;
#else
    const QByteArray qba = qstr.toUtf8();
    return add( qba.constData() );
#endif
}


char* StringBuilder::getCStr( int minlen )
{
    if ( bufsz_ < minlen )
	setBufSz( minlen, true );
    if ( !buf_ )
	setBufSz( 256, false );
    return buf_;
}
