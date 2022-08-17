/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Feb 2014
________________________________________________________________________

-*/

#include "texttranslator.h"

#include "dirlist.h"
#include "filepath.h"
#include "oddirs.h"
#include "ptrman.h"
#include "plugins.h"
#include "settings.h"

#ifndef OD_NO_QT
# include <QTranslator>
# include <QLocale>
#else
# include "keystrs.h"
#endif

void TextTranslateMgr::GetLocalizationDir( FilePath& res )
{
    res = FilePath( mGetSWDirDataDir(), "localizations" );
}


TextTranslatorLanguage::TextTranslatorLanguage( const char* localename )
    : loaded_( false )
#ifndef OD_NO_QT
    , locale_( new QLocale( localename ) )
    , languagename_( new QString )
    , localename_( localename )
#endif
{
    translators_.setNullAllowed();
    const BufferString filename = BufferString()
	.add(uiString::sODLocalizationApplication())
	.add(TextTranslateMgr::cApplicationEnd())
	.add(localename)
	.add(".qm");

    FilePath locdir;
    TextTranslateMgr::GetLocalizationDir(locdir);
    const FilePath odfile( locdir, filename );

#ifndef OD_NO_QT
    QTranslator maintrans;
    if ( !maintrans.load(odfile.fullPath().buf()) )
    {
	*languagename_ = localename;
    }
    else
    {
	uiString name = tr("Language Name",0,1);
	name.addLegacyVersion( uiString(name.getOriginalString(),
			       "TextTranslateMgr",
			       uiString::sODLocalizationApplication(), 0, 1 ));
	name.translate( maintrans, *languagename_ );
	//Force be a part of the plural setup
    }

    locale_->setNumberOptions( QLocale::OmitGroupSeparator );
#endif
}


TextTranslatorLanguage::~TextTranslatorLanguage()
{
#ifndef OD_NO_QT
    deepErase( translators_ );
    delete locale_;
    delete languagename_;
#endif
}

const QTranslator*
TextTranslatorLanguage::getTranslator( const char* appl ) const
{
    const int idx = applications_.indexOf( appl );
    return translators_.validIdx(idx) ? translators_[idx] : 0;
}

BufferString TextTranslatorLanguage::getLocaleName() const
{
#ifndef OD_NO_QT
    return BufferString( locale_->name() );
#else
    return sKey::EmptyString();
#endif
}


const mQtclass(QString)& TextTranslatorLanguage::getLanguageName() const
{ return *languagename_; }


const mQtclass(QLocale)& TextTranslatorLanguage::getLanguageLocale() const
{ return *locale_; }


bool TextTranslatorLanguage::load()
{
    if ( loaded_ )
	return true;

    loaded_ = true;

#ifdef OD_NO_QT
    return false;
#else
    const BufferString filenamesearch = BufferString("*")
	.add( TextTranslateMgr::cApplicationEnd() )
	.add( localename_ )
	.add( ".qm" );

    FilePath basedir;
    TextTranslateMgr::GetLocalizationDir(basedir);
    DirList dl( basedir.fullPath(), File::FilesInDir, filenamesearch.buf() );

    for( int idx=0; idx<dl.size(); idx++ )
    {
	const FilePath filepath = dl.fullPath( idx );
	BufferString filename = filepath.baseName();

	BufferString application;
	char* applicationend =
		filename.find(TextTranslateMgr::cApplicationEnd());
	if ( !applicationend )
	    continue;

	application = filename;
	*application.find(TextTranslateMgr::cApplicationEnd()) = 0;

	BufferString language = applicationend+1;

	QTranslator* trans = new QTranslator;
	if ( !trans->load(filepath.fullPath().buf()) )
	{
	    delete trans;
	    continue;
	}

	translators_ += trans;
	applications_.add( application );
    }

    return true;
#endif
}


namespace TextTranslation
{
static Threads::Lock mgrlock_( Threads::Lock::SmallWork );
} // namespace TextTranslation


// TextTranslateMgr
mGlobal(Basic) TextTranslateMgr& TrMgr()
{
    Threads::Locker lock( TextTranslation::mgrlock_, Threads::Locker::ReadLock);
    mDefineStaticLocalObject( PtrMan<TextTranslateMgr>, trmgr, = 0 );

    if ( !trmgr )
    {
	if ( lock.convertToWriteLock() )
	{
	    if ( trmgr.setIfNull(new TextTranslateMgr,true) )
	    {
//		trmgr->reInit();
		return *trmgr;
	    }
	}

	//Fallback, not thread safe
	trmgr = new TextTranslateMgr;
//	trmgr->reInit();
    }
    return *trmgr;
}


TextTranslateMgr::TextTranslateMgr()
    : dirtycount_(0)
    , currentlanguageidx_(-1)
    , languageChange(this)
{
    loadUSEnglish();
    uiString err;
    setLanguage( 0, err );
}


TextTranslateMgr::~TextTranslateMgr()
{
    deepUnRef( languages_ );
}


int TextTranslateMgr::nrSupportedLanguages() const
{
    return languages_.size();
}


bool TextTranslateMgr::addLanguage( TextTranslatorLanguage* language )
{
    if ( !language )
	return false;

    language->ref();

    const BufferString newlocale = language->getLocaleName();

    for ( int idx=0; idx<languages_.size(); idx++ )
    {
	if ( languages_[idx]->getLocaleName()==newlocale )
	{
	    language->unRef();
	    return false;
	}
    }

    languages_ += language;
    languageChange.trigger();
    return true;
}


uiString TextTranslateMgr::getLanguageUserName(int idx) const
{
    if ( languages_.validIdx(idx) )
    {
	uiString ret;
#ifdef OD_NO_QT
	ret = languages_[idx]->getLocaleName();
#else
	ret.setFrom( languages_[idx]->getLanguageName() );
#endif
	return ret;
    }

    return toUiString("Unknown language");
}


BufferString TextTranslateMgr::getLocaleName(int idx) const
{
    if ( languages_.validIdx(idx) )
	return languages_[idx]->getLocaleName();

    return "en_US";
}


bool TextTranslateMgr::setLanguage( int idx, uiString& errmsg )
{
    if ( idx==currentlanguageidx_ )
	return true;

    if ( languages_.validIdx(idx) )
    {
	if ( !languages_[idx]->load() )
	{
	    errmsg = tr("Cannot load %1").arg(getLanguageUserName(idx) );
	    return false;
	}
    }
    else if ( idx>=0 )
    {
	errmsg = tr("Cannot find selected language");
	return false;
    }

    currentlanguageidx_ = idx;
    dirtycount_++;
    languageChange.trigger();

    return true;
}


int TextTranslateMgr::currentLanguage() const
{ return currentlanguageidx_; }


const QTranslator*
    TextTranslateMgr::getQTranslator( const char* application ) const
{
    return languages_.validIdx(currentlanguageidx_)
	? languages_[currentlanguageidx_]->getTranslator( application )
	: 0;
}


const QLocale* TextTranslateMgr::getQLocale() const
{
#ifndef OD_NO_QT
    return languages_.validIdx(currentlanguageidx_)
	? &languages_[currentlanguageidx_]->getLanguageLocale()
        : 0;
#else
    return 0;
#endif
}


void TextTranslateMgr::loadUSEnglish()
{
    RefMan<TextTranslatorLanguage> english =
				new TextTranslatorLanguage("en_US");
    addLanguage( english );
}


bool TextTranslateMgr::loadTranslations()
{
    BufferString libname;
    libname.setBufSize( 256 );
    SharedLibAccess::getLibName( "dGBCommon",
			libname.getCStr(), libname.bufSize() );
    return PIM().load( FilePath( GetLibPlfDir(), libname ).fullPath() );
}


namespace OD
{

static const char* sLocalizationKey() { return "Language"; }

static void languageChange( CallBacker* )
{
    Settings& setts = Settings::common();
    const BufferString locale =
		TrMgr().getLocaleName( TrMgr().currentLanguage() );
    BufferString curlocale;
    if ( setts.get(sLocalizationKey(),curlocale) && curlocale==locale )
	return;

    setts.set( sLocalizationKey(), locale );
    setts.write();
}


void loadLocalization()
{
    FilePath basedir;
    TextTranslateMgr::GetLocalizationDir( basedir );
    DirList dl( basedir.fullPath(), File::FilesInDir, "*.qm");

    QList<QLocale> allLocales = QLocale::matchingLocales(
            QLocale::AnyLanguage,
            QLocale::AnyScript,
            QLocale::AnyCountry);

    BufferStringSet accepted_languages;
    for  ( int i=0; i<allLocales.size(); i++ )
    {
	QString localenm = allLocales[i].name();
	BufferString str(localenm);
	accepted_languages.add(localenm);
    }

    //This should really be done on build-level, buy as od6 is released,
    //the installer will not remove those qm-files.

    for( int idx=0; idx<dl.size(); idx++ )
    {
	const FilePath path = dl.fullPath( idx );
	const BufferString filename = path.baseName();
	const char* applicationend =
		filename.find( TextTranslateMgr::cApplicationEnd() );
	if ( !applicationend )
	    continue;

	const BufferString locale = applicationend+1;
	if ( getIndexInStringArrCI(locale.buf(),accepted_languages,0,0,-1)==-1 )
	    continue;

	RefMan<TextTranslatorLanguage> trans =
		new TextTranslatorLanguage( locale );
	TrMgr().addLanguage( trans );
    }

    Settings& setts = Settings::common();
    BufferString locale;
    if ( setts.get(sLocalizationKey(),locale) )
    {
	for ( int idx=0; idx<TrMgr().nrSupportedLanguages(); idx++ )
	{
	    if ( TrMgr().getLocaleName(idx)==locale )
	    {
		uiString err;
		TrMgr().setLanguage( idx, err );
		break;
	    }
	}
    }

    TrMgr().languageChange.notify( mSCB(languageChange) );
}

} // namespace OD
