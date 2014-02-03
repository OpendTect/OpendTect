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
#include "ptrman.h"


#include <QString>
#include <QCoreApplication>

uiString::uiString( const char* str )
    : qstring_( 0 )
    , originalstring_( 0 )
    , translationcontext_( 0 )
    , translationpluralnumber_( -1 )
    , translationdisambiguation_( 0 )
{
    *this = str;
}


uiString::uiString( const char* text, const char* context,
		    const char* disambiguation, int pluralnr )
    : qstring_( (text && *text) ? new QString(text) : 0)
    , originalstring_( text )
    , translationcontext_( context )
    , translationpluralnumber_( pluralnr )
    , translationdisambiguation_( disambiguation )
{
    update( true, true );
}


uiString::uiString( const uiString& str )
    : qstring_( 0 )
    , originalstring_( 0 )
    , translationcontext_( 0 )
    , translationpluralnumber_( -1 )
    , translationdisambiguation_( 0 )
{
    *this = str;
}


uiString::uiString( const FixedString& str )
    : qstring_( 0 )
    , originalstring_( 0 )
    , translationcontext_( 0 )
    , translationpluralnumber_( -1 )
    , translationdisambiguation_( 0 )
{
    *this = str;
}


uiString::uiString( const BufferString& str )
    : qstring_( 0 )
    , originalstring_( 0 )
    , translationcontext_( 0 )
    , translationpluralnumber_( -1 )
    , translationdisambiguation_( 0 )
{
    *this = str;
}


uiString::uiString( const QString& str, const char* original )
    : qstring_( str.isEmpty() ? 0 : new QString(str) )
    , originalstring_( original )
    , translationcontext_( 0 )
    , translationpluralnumber_( -1 )
    , translationdisambiguation_( 0 )
{
}


uiString::~uiString()
{
    delete qstring_;
    deepErase( arguments_ );
}


bool uiString::isEmpty() const
{
    return !qstring_ || qstring_->isEmpty();
}


const char* uiString::getOriginalString() const
{
    return originalstring_;
}

QString emptystring;


const QString& uiString::getQtString() const
{
    if ( isEmpty() )
	return emptystring;

    return *qstring_;
}

#define mSetQString( strempty, srcassign ) \
if ( strempty ) \
{ \
    deleteAndZeroPtr( qstring_ ); \
} \
else \
{ \
    if ( !qstring_ ) qstring_ = new QString(srcassign); \
    else *qstring_ = srcassign; \
}



uiString& uiString::operator=( const uiString& str )
{
    mSetQString( str.isEmpty(), str.getQtString() );

    originalstring_ = str.originalstring_;
    translationcontext_ = str.translationcontext_;
    translationdisambiguation_ = str.translationdisambiguation_;
    translationpluralnumber_ = str.translationpluralnumber_;

    deepErase( arguments_ );
    deepCopy( arguments_, str.arguments_ );

    return *this;
}


uiString& uiString::operator=( const FixedString& str )
{
    mSetQString(str.isEmpty(),str.str());

    originalstring_ = str.str();
    return *this;
}


uiString& uiString::operator=( const BufferString& str )
{
    mSetQString(str.isEmpty(),str.str());

    originalstring_ = 0;
    return *this;
}


uiString& uiString::operator=( const char* str )
{
    mSetQString( !str, str );

    originalstring_ = str;
    return *this;
}


uiString uiString::arg( const uiString& newarg ) const
{
    uiString ret = *this;
    ret.addArgument( new uiString(newarg) );
    ret.update( true, true );
    return ret;
}


uiString uiString::arg( const char* newarg ) const
{
    uiString ret = *this;
    ret.addArgument( new uiString(newarg) );
    ret.update( true, true );
    return ret;
}


void uiString::update( bool translate, bool replace )
{
    for ( int idx=0; idx<arguments_.size(); idx++ )
	arguments_[idx]->update( translate, replace );

    if ( !qstring_ )
	return;

    *qstring_ = originalstring_;

    if ( !translate && !replace )
    {
	return;
    }

    if ( translate )
    {
	*qstring_ = QCoreApplication::translate( translationcontext_,
						 originalstring_,
						 translationdisambiguation_,
						 QCoreApplication::CodecForTr,
						 translationpluralnumber_ );
    }

    if ( replace )
    {
	for ( int idx=0; idx<arguments_.size(); idx++ )
	{
	    *qstring_ = qstring_->arg( arguments_[idx]->getQtString() );
	}
    }
}


void uiString::addArgument( uiString* newarg )
{
    arguments_ += newarg;
}


void uiString::setTranslationContext( const char* ctxt )
{ translationcontext_ = ctxt; }


void uiString::setTranslationDisambiguation( const char* di )
{ translationdisambiguation_ = di; }


void uiString::setTranslationPluralNr( int nr )
{ translationpluralnumber_ = nr; }



#include "uitexttranslator.h"
mGlobal( uiBase )  TextTranslateMgr& TrMgr()
{
    mDefineStaticLocalObject( PtrMan<TextTranslateMgr>, trmgr, = 0 );
    if ( !trmgr ) trmgr = new TextTranslateMgr();
    return *trmgr;
}

