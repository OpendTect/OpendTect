/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2019
-*/


#include "stringbuilder.h"

#ifndef OD_NO_QT
# include <QString>
#endif


StringBuilder::StringBuilder( const char* str )
{
    add( str );
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
	OD::memCopy( buf_, oth.buf_, oth.curpos_ );
	bufsz_ = oth.bufsz_;
	curpos_ = oth.curpos_;
    }

    return *this;
}


bool StringBuilder::operator ==( const StringBuilder& oth ) const
{
    return FixedString(buf_) == FixedString(oth.buf_);
}


bool StringBuilder::operator !=( const StringBuilder& oth ) const
{
    return FixedString(buf_) != FixedString(oth.buf_);
}


StringBuilder& StringBuilder::setEmpty()
{
    delete [] buf_; buf_ = nullptr;
    bufsz_ = curpos_ = 0;
    return *this;
}


StringBuilder& StringBuilder::set( const char* str )
{
    if ( str != buf_ )
    {
	setEmpty();
	add( str );
    }
    return *this;
}


StringBuilder& StringBuilder::add( char ch, size_type nr )
{
    const char str[] = { ch, '\0' };
    for ( int idx=0; idx<nr; idx++ )
	add( str );
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


StringBuilder& StringBuilder::add( const char* str )
{
    if ( !str || !*str )
	return *this;

    if ( !buf_ && !setBufSz(256,false) )
	return *this;

    char* myptr = buf_ + curpos_;
    while ( *str )
    {
	*myptr++ = *str++;
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
