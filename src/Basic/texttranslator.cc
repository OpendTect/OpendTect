/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Feb 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "texttranslator.h"

#include "dirlist.h"
#include "filepath.h"
#include "oddirs.h"
#include "ptrman.h"

#include <QTranslator>

class TranslatorLanguageInfo
{
public:
				TranslatorLanguageInfo()
				    : loaded_( false )
				{ translators_.allowNull( true ); }
				~TranslatorLanguageInfo()
				{ deepErase( translators_ ); }

    bool			load();

    BufferString		name_;
    QString			username_;
    bool			loaded_;

    ObjectSet<QTranslator>	translators_;
    BufferStringSet		applications_;

    const QTranslator*		getTranslator(const char* appl) const;
};


const QTranslator*
TranslatorLanguageInfo::getTranslator( const char* appl ) const
{
    const int idx = applications_.indexOf( appl );
    if ( idx>=0 )
	return translators_[idx];

    return 0;
}


static FilePath GetLocalizationDir()
{
    return FilePath( GetSoftwareDir(false), "data", "localizations" );
}


bool TranslatorLanguageInfo::load()
{
    BufferString filenamepostfix;
    filenamepostfix.add( TextTranslateMgr::cApplicationEnd() )
		   .add( name_ ).add( ".qm" );
    for ( int idx=0; idx<applications_.size(); idx++ )
    {
	if ( translators_[idx] )
	    continue;

	const BufferString filename( applications_[idx]->buf(),
				     filenamepostfix );

	const FilePath filepath( GetLocalizationDir().fullPath(), filename );

	QTranslator* trans = new QTranslator;
	if ( trans->load( filepath.fullPath().buf() ) )
	{
	    delete trans;
	    continue;
	}

	translators_.replace( idx, trans );
    }

    loaded_ = true;

    return true;
}


mGlobal( Basic )  TextTranslateMgr& TrMgr()
{
    mDefineStaticLocalObject( PtrMan<TextTranslateMgr>, trmgr, = 0 );
    if ( !trmgr ) trmgr = new TextTranslateMgr();
    return *trmgr;
}


TextTranslateMgr::~TextTranslateMgr()
{
    deepErase( languages_ );
}


int TextTranslateMgr::nrSupportedLanguages() const
{
    return languages_.size()+1;
}


uiString TextTranslateMgr::getLanguageUserName(int idx) const
{
    idx--; //Compensate for default language
    if ( languages_.validIdx(idx) )
    {
	uiString ret;
	ret.setFrom( languages_[idx]->username_ );
	return ret;
    }

    return uiString("English");
}


BufferString TextTranslateMgr::getLanguageName(int idx) const
{
    idx--; //Compensate for default language

    if ( languages_.validIdx(idx) )
	return languages_[idx]->name_;

    return "en_US";
}


bool TextTranslateMgr::setLanguage( int idx, uiString& errmsg )
{
    idx--; //Compensate for default language
    if ( idx==currenttranslatoridx_ )
	return true;

    if ( languages_.validIdx(idx) )
    {
	if ( !languages_[idx]->loaded_ )
	{
	    if ( !languages_[idx]->load() )
	    {
		errmsg = tr("Cannot load %1").arg(getLanguageUserName(idx) );
		return false;
	    }
	}
    }

    currenttranslatoridx_ = idx;
    dirtycount_++;
    languageChange.trigger();

    return true;
}


const QTranslator*
TextTranslateMgr::getQTranslator( const char* application ) const
{
    return languages_.validIdx(currenttranslatoridx_)
	? languages_[currenttranslatoridx_]->getTranslator( application )
	: 0;
}


void TextTranslateMgr::loadInfo()
{
    FilePath basedir( GetSoftwareDir(false), "data", "localizations" );
    DirList dl( basedir.fullPath(), DirList::FilesOnly, "*.ts");

    BufferStringSet applications;

    for( int idx=0; idx<dl.size(); idx++ )
    {
	const FilePath path = dl[0]->str();
	BufferString filename = path.baseName();

	BufferString application;
	char* applicationend = filename.find(cApplicationEnd());
	if ( !applicationend )
	    continue;

	application = filename;
	*application.find(cApplicationEnd()) = 0;

	BufferString language = applicationend+1;

	TranslatorLanguageInfo* tli = 0;
	for ( int idy=0; idy<languages_.size(); idy++ )
	{
	    if ( languages_[idy]->name_==language )
	    {
		tli = languages_[idy];
		break;
	    }
	}

	if ( !tli )
	{
	    tli = new TranslatorLanguageInfo;
	    tli->name_ = language;
	    languages_ += tli;
	}

	tli->applications_.add( application );
	tli->translators_ += 0;

	 if ( application==uiString::sODLocalizationApplication() )
	 {
	     QTranslator maintrans;
	     if ( !maintrans.load( path.fullPath().buf() ))
	     {
		 tli->username_ = tli->name_;
	     }
	     else
	     {
		 tr("Language Name").translate( maintrans, tli->username_ );
	     }
	 }
    }
}


