/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: googletranslator.cc,v 1.11 2012/01/24 21:25:50 cvsnanne Exp $";

#include "googletranslator.h"
#include "odhttp.h"

#include "uimsg.h"
#include "uiodmain.h"
#include "uistatusbar.h"

#include <QString>

static const char* sHostAddress()       { return "www.googleapis.com"; }

static void setStatusMessage( const char* msg )
{ ODMainWin()->statusBar()->message( msg, 3 ); }


GoogleTranslator::GoogleTranslator()
    : odhttp_(*new ODHttp())
    , tolanguage_(0)
    , translation_(new wchar_t [256])
{
    odhttp_.readyRead.notify( mCB(this,GoogleTranslator,readyCB) );
    odhttp_.disconnected.notify( mCB(this,GoogleTranslator,disConnCB) );
    odhttp_.messageReady.notify( mCB(this,GoogleTranslator,messageCB) );
    odhttp_.setASynchronous( true );
    init();
}


GoogleTranslator::~GoogleTranslator()
{
    delete [] translation_;
    delete &odhttp_;
}


const char* GoogleTranslator::getIcon() const
{ return "googletranslate.png"; }


void GoogleTranslator::enable()
{ odhttp_.setHttpsHost( sHostAddress() ); }

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

void GoogleTranslator::setAPIKey( const char* key )
{ apikey_ = key; }

static const char* sBasicUrl()
{ return "/language/translate/v2?"; }

void GoogleTranslator::createUrl( BufferString& url, const char* txt )
{
    url = sBasicUrl();
    if ( !apikey_.isEmpty() )
	url.add( "key=" ).add( apikey_ );

    const char* tolang = tolanguage_ ? tolanguage_->code_.buf() : "nl";
    url.add( "&source=en&target=" ).add( tolang ).add( "&q=" ).add( txt );
}


int GoogleTranslator::translate( const char* txt )
{
    BufferString url;
    createUrl( url, txt );
    return odhttp_.get( url );
}


void GoogleTranslator::readyCB( CallBacker* )
{
    wchar_t* buffer = odhttp_.readWCharBuffer();
    if ( buffer )
    {
	QString qresult = QString::fromWCharArray( buffer );
	int idx1 = qresult.indexOf( "translatedText" );
	idx1 += 18;
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


void GoogleTranslator::add( const wchar_t* localnm, const char* englishnm,
			    const char* code )
{
    infos_ += new LanguageInfo( localnm, englishnm, code );
}


void GoogleTranslator::disConnCB( CallBacker* )
{
    uiMSG().error( "Google Translate connection closed.\n"
		   "Please enable again via 'Utilities' menu" );
}


void GoogleTranslator::messageCB( CallBacker* )
{ setStatusMessage( odhttp_.message() ); }


void GoogleTranslator::init()
{
    add(L"Afrikaans","Afrikaans","af");
    add(L"Albanian","Albanian","sq");
    add(L"Arabic","Arabic","ar");
    add(L"Armenian","Armenian","hy");
    add(L"Azerbaijani","Azerbaijani","az");
    add(L"Basque","Basque","eu");
    add(L"Belarusian","Belarusian","be");
    add(L"Bulgarian","Bulgarian","bg");
    add(L"Catalan","Catalan","ca");
    add(L"Chinese_Simplified","Chinese_Simplified","zh-CN");
    add(L"Chinese_Traditional","Chinese_Traditional","zh-TW");
    add(L"Chinese","Chinese","zh");
    add(L"Croatian","Croatian","hr");
    add(L"Czech","Czech","cs");
    add(L"Danish","Danish","da");
    add(L"Dutch","Dutch","nl");
    add(L"English","English","en");
    add(L"Estonian","Estonian","et");
    add(L"Filipino","Filipino","tl");
    add(L"Finnish","Finnish","fi");
    add(L"French","French","fr");
    add(L"Galician","Galician","gl");
    add(L"Georgian","Georgian","ka");
    add(L"German","German","de");
    add(L"Greek","Greek","el");
    add(L"Haitian_Creole","Haitian_Creole","ht");
    add(L"Hebrew","Hebrew","iw");
    add(L"Hindi","Hindi","hi");
    add(L"Hungarian","Hungarian","hu");
    add(L"Icelandic","Icelandic","is");
    add(L"Indonesian","Indonesian","id");
    add(L"Irish","Irish","ga");
    add(L"Italian","Italian","it");
    add(L"Japanese","Japanese","ja");
    add(L"Korean","Korean","ko");
    add(L"Latvian","Latvian","lv");
    add(L"Lithuanian","Lithuanian","lt");
    add(L"Macedonian","Macedonian","mk");
    add(L"Malay","Malay","ms");
    add(L"Maltese","Maltese","mt");
    add(L"Norwegian","Norwegian","no");
    add(L"Persian","Persian","fa");
    add(L"Polish","Polish","pl");
    add(L"Portuguese_Portugal","Portuguese_Portugal","pt-PT");
    add(L"Portuguese","Portuguese","pt");
    add(L"Romanian","Romanian","ro");
    add(L"Russian","Russian","ru");
    add(L"Serbian","Serbian","sr");
    add(L"Slovak","Slovak","sk");
    add(L"Slovenian","Slovenian","sl");
    add(L"Spanish","Spanish","es");
    add(L"Swahili","Swahili","sw");
    add(L"Swedish","Swedish","sv");
    add(L"Tagalog","Tagalog","tl");
    add(L"Thai","Thai","th");
    add(L"Turkish","Turkish","tr");
    add(L"Ukrainian","Ukrainian","uk");
    add(L"Vietnamese","Vietnamese","vi");
    add(L"Welsh","Welsh","cy");
    add(L"Yiddish","Yiddish","yi");

    tolanguage_ = infos_[1];
}
