/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: googletranslator.cc,v 1.5 2010-12-03 11:35:03 cvsnanne Exp $";

#include "googletranslator.h"
#include "odhttp.h"

#include "uimsg.h"
#include "uiodmain.h"
#include "uistatusbar.h"

#include <QString>

static const char* sHostAddress()       { return "ajax.googleapis.com"; }

void setStatusMessage( const char* msg )
{ ODMainWin()->statusBar()->message( msg, 3 ); }


GoogleTranslator::GoogleTranslator()
    : odhttp_(*new ODHttp())
    , tolanguage_(0)
    , translation_(new wchar_t [256])
{
    odhttp_.readyRead.notify( mCB(this,GoogleTranslator,readyCB) );
    odhttp_.disconnected.notify( mCB(this,GoogleTranslator,disConnCB) );
    odhttp_.messageReady.notify( mCB(this,GoogleTranslator,messageCB) );
    init();
}


GoogleTranslator::~GoogleTranslator()
{
    delete [] translation_;
    delete &odhttp_;
}


void GoogleTranslator::enable()
{ odhttp_.setHost( sHostAddress() ); }

void GoogleTranslator::disable()
{ odhttp_.close(); }

bool GoogleTranslator::enabled() const
{ return odhttp_.state() != ODHttp::Unconnected; }

int GoogleTranslator::nrSupportedLanguages() const
{ return infos_.size(); }

const wchar_t* GoogleTranslator::getLanguageUserName( int idx ) const
{ return infos_.validIdx(idx) ? infos_[idx]->username_ : 0; }

const char* GoogleTranslator::getLanguageName( int idx ) const
{ return infos_.validIdx(idx) ? infos_[idx]->name_.buf() : 0; }


bool GoogleTranslator::supportsLanguage( const char* lang ) const
{
    for ( int idx=0; idx<infos_.size(); idx++ )
	if ( infos_[idx]->name_ == lang )
	    return true;

    return false;
}


bool GoogleTranslator::setToLanguage( const char* lang )
{
    for ( int idx=0; idx<infos_.size(); idx++ )
    {
	if ( infos_[idx]->name_ != lang )
	    continue;

	tolanguage_ = infos_[idx];
	return true;
    }

    return false;
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
    if ( buffer )
    {
	QString qresult = QString::fromWCharArray( buffer );
	int idx1 = qresult.indexOf( "translatedText" );
	idx1 += 17;
	int idx2 = qresult.indexOf( '"', idx1 );
	QString qtrl = qresult.mid( idx1, idx2-idx1 );

	QString before( "\\u0026#39;" );
	QString after( "'" );
	qtrl.replace( before, after );

	const int sz = qtrl.size();
	qtrl.toWCharArray( translation_ );
	translation_[sz] = L'\0';
    }
    else
	wcscpy(translation_,L"?");

    ready.trigger( odhttp_.currentRequestID() );
}


const wchar_t* GoogleTranslator::get() const
{ return translation_; }


void GoogleTranslator::init()
{
#ifndef __win__
    infos_ += new LanguageInfo( L"中国\0", "Chinese", "zh-CN" );
    infos_ += new LanguageInfo( L"Nederlands", "Dutch", "nl" );
    infos_ += new LanguageInfo( L"Deutch", "German", "de" );
    infos_ += new LanguageInfo( L"Español", "Spanish", "es" );
    infos_ += new LanguageInfo( L"Français", "French", "fr" );
    infos_ += new LanguageInfo( L"हिन्दी", "Hindi", "hi" );
    infos_ += new LanguageInfo( L"Italiano", "Italian", "it" );
    infos_ += new LanguageInfo( L"日本", "Japanese", "ja" );
    infos_ += new LanguageInfo( L"Português", "Portuguese", "pt" );
    infos_ += new LanguageInfo( L"Русский", "Russian", "ru" );
#else
    infos_ += new LanguageInfo( L"Chinese", "Chinese", "zh-CN" );
    infos_ += new LanguageInfo( L"Nederlands", "Dutch", "nl" );
    infos_ += new LanguageInfo( L"Deutch", "German", "de" );
    infos_ += new LanguageInfo( L"Español", "Spanish", "es" );
    infos_ += new LanguageInfo( L"Français", "French", "fr" );
    infos_ += new LanguageInfo( L"Hindi", "Hindi", "hi" );
    infos_ += new LanguageInfo( L"Italiano", "Italian", "it" );
    infos_ += new LanguageInfo( L"Japanese", "Japanese", "ja" );
    infos_ += new LanguageInfo( L"Português", "Portuguese", "pt" );
    infos_ += new LanguageInfo( L"Russian", "Russian", "ru" );
#endif

    tolanguage_ = infos_[1];
}


void GoogleTranslator::disConnCB( CallBacker* )
{
    uiMSG().error( "Google Translate connection closed.\n"
		   "Please enable again via 'Utilities' menu" );
}


void GoogleTranslator::messageCB( CallBacker* )
{ setStatusMessage( odhttp_.message() ); }
