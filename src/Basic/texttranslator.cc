/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
		    .add( uiString::sODLocalizationApplication() )
		    .add( TextTranslateMgr::cApplicationEnd() )
		    .add( localename )
		    .add( ".qm" );

    const FilePath odfile = GetSetupShareFileInDir( "localizations",
						    filename.str(), true );

#ifndef OD_NO_QT
    QTranslator maintrans;
    if ( maintrans.load(odfile.fullPath().buf()) )
    {
	uiString name = tr("Language Name",0,1);
	name.addLegacyVersion( uiString(name.getString(),
			       "TextTranslateMgr",
			       uiString::sODLocalizationApplication(),0,1) );
	name.translate( maintrans, *languagename_ );
	//Force be a part of the plural setup
    }
    else
    {
	*languagename_ = localename;
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

    BufferStringSet locdirfnms;
    GetSetupShareFilesInDir( "localizations", filenamesearch.str(), locdirfnms,
			     true );
    for ( const auto* filename : locdirfnms )
    {
	BufferString application;
	const char* applicationend =
		filename->find(TextTranslateMgr::cApplicationEnd());
	if ( !applicationend )
	    continue;

	application = filename->str();
	*application.find(TextTranslateMgr::cApplicationEnd()) = 0;

	PtrMan<QTranslator> trans = new QTranslator;
	if ( !trans->load(filename->str()) )
	    continue;

	translators_.add( trans.release() );
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
	: nullptr;
}


const QLocale* TextTranslateMgr::getQLocale() const
{
#ifndef OD_NO_QT
    return languages_.validIdx(currentlanguageidx_)
	? &languages_[currentlanguageidx_]->getLanguageLocale()
        : nullptr;
#else
    return nullptr;
#endif
}


void TextTranslateMgr::loadUSEnglish()
{
    RefMan<TextTranslatorLanguage> english =
				new TextTranslatorLanguage("en_US");
    addLanguage( english.ptr() );
}


bool TextTranslateMgr::loadTranslations()
{
    BufferString libname;
    libname.setBufSize( 256 );
    SharedLibAccess::getLibName( "dGBCommon",
			libname.getCStr(), libname.bufSize() );
    const FilePath fp( GetLibPlfDir(), libname );
    return fp.exists() ? PIM().load( fp.fullPath() ) : false;
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
    const QList<QLocale> allLocales = QLocale::matchingLocales(
					QLocale::AnyLanguage,
					QLocale::AnyScript,
					QLocale::AnyCountry );
    BufferStringSet accepted_languages;
    for  ( int i=0; i<allLocales.size(); i++ )
    {
	const QString localenm = allLocales[i].name();
	accepted_languages.add( localenm );
    }

    //This should really be done on build-level, buy as od6 is released,
    //the installer will not remove those qm-files.

    BufferStringSet locfilenms;
    GetSetupShareFilesInDir( "localizations", "*.qm", locfilenms, true );
    for ( const auto* fnm : locfilenms )
    {
	const FilePath path( fnm->str() );
	const BufferString filename = path.baseName();
	const char* applicationend =
			filename.find( TextTranslateMgr::cApplicationEnd() );
	if ( !applicationend )
	    continue;

	const BufferString locale = applicationend+1;
	if ( getIndexInStringArrCI(locale.buf(),accepted_languages,
				   0,0,-1)==-1 )
	    continue;

	RefMan<TextTranslatorLanguage> trans =
				new TextTranslatorLanguage( locale );
	TrMgr().addLanguage( trans.ptr() );
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
