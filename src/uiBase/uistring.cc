/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistring.h"

#include "bufstring.h"


#include <QString>

uiString::uiString( const char* str )
    : qstring_( new QString )
    , originalstring_( 0 )
{
    *this = str;
}


uiString::uiString( const uiString& str )
    : qstring_( new QString )
    , originalstring_( 0 )
{
    *this = str;
}


uiString::uiString( const FixedString& str )
    : qstring_( new QString )
    , originalstring_( 0 )
{
    *this = str;
}


uiString::uiString( const BufferString& str )
    : qstring_( new QString )
    , originalstring_( 0 )
{
    *this = str;
}


uiString::uiString( const QString& str, const char* original )
    : qstring_( new QString(str) )
    , originalstring_( original )
{
}


uiString::~uiString()
{ delete qstring_; }


uiString& uiString::operator=( const uiString& str )
{
    *qstring_ = str.getQtString();
    originalstring_ = str.originalstring_;
    return *this;
}


uiString& uiString::operator=( const FixedString& str )
{
    *qstring_ = str.str();
    originalstring_ = str.str();
    return *this;
}


const char* uiString::getOriginalString() const
{
    return originalstring_;
}


uiString& uiString::operator=( const BufferString& str )
{
    *qstring_ = str.str();
    originalstring_ = 0;
    return *this;
}


uiString& uiString::operator=( const char* str )
{
    if ( str )
	*qstring_ = str;
    originalstring_ = str;
    return *this;
}


uiString uiString::arg( const uiString& newarg ) const
{
    return uiString( qstring_->arg( newarg.getQtString() ), originalstring_ );
}


uiString uiString::arg( const char* newarg ) const
{
    return uiString( qstring_->arg( newarg ), originalstring_ );
}


