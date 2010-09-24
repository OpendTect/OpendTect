/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: googletranslator.cc,v 1.1 2010-09-24 12:03:31 cvsnanne Exp $";

#include "googletranslator.h"
#include "odhttp.h"

#include <QString>

static const char* sHostAddress()       { return "ajax.googleapis.com"; }

GoogleTranslator::GoogleTranslator()
    : odhttp_(*new ODHttp())
    , tolanguage_(0)
{
    odhttp_.readyRead.notify( mCB(this,GoogleTranslator,readyCB) );
    init();
}


GoogleTranslator::~GoogleTranslator()
{
    odhttp_.readyRead.remove( mCB(this,GoogleTranslator,readyCB) );
}


void GoogleTranslator::enable()
{ odhttp_.setHost( sHostAddress() ); }

void GoogleTranslator::disable()
{ odhttp_.close(); }


int GoogleTranslator::nrSupportedLanguages() const
{ return infos_.size(); }

const wchar_t* GoogleTranslator::getLanguageUserName( int idx ) const
{ return infos_.validIdx(idx) ? infos_[idx]->username_.c_str() : 0; }

const char* GoogleTranslator::getLanguageName( int idx ) const
{ return infos_.validIdx(idx) ? infos_[idx]->name_.buf() : 0; }

void GoogleTranslator::setToLanguage( const char* lang )
{
    for ( int idx=0; idx<infos_.size(); idx++ )
    {
	if ( infos_[idx]->name_ != lang )
	    continue;

	tolanguage_ = infos_[idx];
	break;
    }
}


const char* GoogleTranslator::getToLanguage() const
{ return tolanguage_ ? tolanguage_->name_.buf() : 0; }


static const char* sBasicUrl()
{ return "/ajax/services/language/translate?v=1.0&q="; }

static void createUrl( BufferString& url, const char* txt, const char* fromlang,
                       const char* tolang )
{
    url = sBasicUrl();
    url.add( txt ).add( "&langpair=" )
       .add( fromlang ).add( "|" ).add( tolang );
}


int GoogleTranslator::translate( const char* txt )
{
    BufferString url;
    const char* from = "en";
    const char* to = tolanguage_ ? tolanguage_->code_.buf() : "nl";
    createUrl( url, txt, from, to );
    return odhttp_.get( url );
}


void GoogleTranslator::readyCB( CallBacker* )
{
    wchar_t* buffer = odhttp_.readWCharBuffer();
    QString qresult = QString::fromWCharArray( buffer );
    int idx1 = qresult.indexOf( "translatedText" );
    idx1 += 17;
    int idx2 = qresult.indexOf( '"', idx1 );
    translation_ = qresult.mid( idx1, idx2-idx1 ).toStdWString();
    ready.trigger( odhttp_.currentRequestID() );
}


const wchar_t* GoogleTranslator::get() const
{ return translation_.c_str(); }


void GoogleTranslator::init()
{
    infos_ += new LanguageInfo( L"Nederlands", "Dutch", "nl" );
    infos_ += new LanguageInfo( L"Deutch", "German", "de" );
    infos_ += new LanguageInfo( L"Español", "Spanish", "es" );
    infos_ += new LanguageInfo( L"Français", "French", "fr" );
    infos_ += new LanguageInfo( L"Italiano", "Italian", "it" );
    infos_ += new LanguageInfo( L"Português", "Portuguese", "pt" );
    infos_ += new LanguageInfo( L"Русский", "Russian", "ru" );

    tolanguage_ = infos_[0];
}
