/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Feb 2014
________________________________________________________________________

-*/

#include "texttranslation.h"

#include "dirlist.h"
#include "filepath.h"
#include "oddirs.h"
#include "ptrman.h"
#include "settings.h"
#include "uistrings.h"

#ifndef OD_NO_QT
# include <QTranslator>
# include <QLocale>
# include <QCoreApplication>
#endif

static const char* sLocalizationKey() { return "Language"; }


namespace OD
{

    mGlobal(Basic) void loadLocalization();
    void loadLocalization()
    {
	Settings& setts = Settings::common();
	BufferString locale;
	setts.get( sLocalizationKey(), locale );
	if ( locale.isEmpty() )
	    locale = "en";

	TrMgr().setLanguageByLocaleKey( locale );
    }

} // namespace OD


static BufferString getLocaleFromFnm( const BufferString& inpfnm,
				      BufferString& pkgnm )
{
    File::Path fp( inpfnm ); fp.setExtension( 0 );
    pkgnm = fp.fileName();
    const char* pkgsep
	= TextTranslation::TranslateMgr::sPackageLanguageSeparator();
    char* localenmptr = pkgnm.find( pkgsep );
    if ( localenmptr )
    {
	*localenmptr = '\0';
	localenmptr += FixedString( pkgsep ).size();
    }
    return BufferString( localenmptr );
}


TextTranslation::TranslatorData::~TranslatorData()
{
#ifndef OD_NO_QT
    delete qtranslator_;
#endif
}


mDefineEnumUtils( TextTranslation::TranslateMgr, Language, "Language" )
{
    "English", "Chinese", "Spanish", 0
};

template <>
void EnumDefImpl<TextTranslation::TranslateMgr::Language>::init()
{
    uistrings_ += mEnumTr("English",0);
    uistrings_ += mEnumTr("Chinese",0);
    uistrings_ += mEnumTr("Spanish",0);
}


TextTranslation::LanguageEntry::LanguageEntry( const char* qmfnm,
					       const char* localekey )
    : localekey_(localekey)
#ifndef OD_NO_QT
    , qlocale_(new QLocale(localekey))
    , qlangname_(new QString(localekey))
    , addtoentry_(false)
#endif
{
#ifndef OD_NO_QT
    BufferString pkgky;
    getLocaleFromFnm( qmfnm, pkgky );
    setLangName( qmfnm );
    addData( qmfnm, pkgky );
    qlocale_->setNumberOptions( QLocale::OmitGroupSeparator );
#endif
}


TextTranslation::LanguageEntry::LanguageEntry( const char* localekey,
					       const BufferStringSet& pkgnms )
    : localekey_(localekey)
#ifndef OD_NO_QT
    , qlocale_(new QLocale(localekey))
    , qlangname_(new QString(localekey))
    , qt_transl_(0)
    , addtoentry_(false)
#endif
{
#ifndef OD_NO_QT
    addData4Dir( TrMgr().userlocdir_, pkgnms );
    addData4Dir( TrMgr().applocdir_, pkgnms );
    addData4Dir( TrMgr().instlocdir_, pkgnms );
    qlocale_->setNumberOptions( QLocale::OmitGroupSeparator );
#endif
}


TextTranslation::LanguageEntry::~LanguageEntry()
{
    deepErase( trdata_ );
#ifndef OD_NO_QT
    delete qlocale_;
    delete qlangname_;
#endif
}


void TextTranslation::LanguageEntry::addData4Dir( const File::Path& dirfp,
						const BufferStringSet& pkgnms )
{
#ifndef OD_NO_QT
    for ( int ipkg=0; ipkg<pkgnms.size(); ipkg++ )
    {
	const BufferString& pkgnm = pkgnms.get( ipkg );
	const BufferString filename = BufferString()
	    .add( pkgnm )
	    .add( TranslateMgr::sPackageLanguageSeparator() )
	    .add( localekey_ )
	    .add( ".qm" );
	const BufferString qmfnm = File::Path( dirfp, filename ).fullPath();
	if ( getData4Pkg(pkgnm) || !File::exists(qmfnm) )
	    continue;

	addtoentry_ = true;

	if ( pkgnm == "od" )
	    setLangName( qmfnm );

	addData( qmfnm, pkgnm );
    }
#endif
}


void TextTranslation::LanguageEntry::setLangName( const char* qmfnm )
{
    QTranslator maintrans;
    if ( maintrans.load(QString(qmfnm)) )
    {
	uiString langnm = tr("LANGUAGE_NAME",0,1);
	uiString oldnm( "Language Name", "TextTranslatorLanguage", "od", 0, 1 );
	langnm.addAlternateVersion( oldnm );
	langnm.translate( maintrans, *qlangname_ );
    }
}


void TextTranslation::LanguageEntry::addData( const char* qmfnm,
					      const char* pkgnm )
{
    TranslatorData* data = new TranslatorData;
    data->filename_ = qmfnm;
    data->pkgnm_ = pkgnm;
    data->qtranslator_ = 0;
    trdata_ += data;
}


TextTranslation::TranslatorData*
TextTranslation::LanguageEntry::getData4Pkg( const char* pkg ) const
{
    for ( int idx=0; idx<trdata_.size(); idx++ )
	if ( trdata_[idx]->pkgnm_ == pkg )
	    return const_cast<TranslatorData*>( trdata_[idx] );
    return 0;
}


const QTranslator* TextTranslation::LanguageEntry::getQTranslator(
					const char* pkg ) const
{
    const TranslatorData* data = getData4Pkg( pkg );
    return data ? data->qtranslator_ : 0;
}


bool TextTranslation::LanguageEntry::isLoaded() const
{
    if ( trdata_.isEmpty() )
	return true;
    for ( int idx=0; idx<trdata_.size(); idx++ )
	if ( trdata_[idx]->qtranslator_ )
	    return true;
    return false;
}


bool TextTranslation::LanguageEntry::load()
{
#ifndef OD_NO_QT

    int nrloaded = 0;
    for ( int idx=0; idx<trdata_.size(); idx++ )
    {
	TranslatorData& trdata = *trdata_[idx];
	if ( trdata.qtranslator_ )
	    { nrloaded++; continue; }

	QTranslator* trans = new QTranslator;
	if ( !trans->load(QString(trdata.filename_.str())) )
	    delete trans;
	else
	{
	    nrloaded++;
	    trdata.qtranslator_ = trans;
	}
    }

    if ( nrloaded < 1 )
	return false;

    BufferString locale;
    Settings::common().get( sLocalizationKey(), locale );

    if ( locale.isEmpty() )
	locale = "en";

    if ( !qt_transl_ && (locale==localekey_) )
    {
	qt_transl_ = new QTranslator;
	const BufferString qtfnm( "qt_", localekey_, ".qm" );
	const BufferString instdir( TrMgr().instlocdir_.fullPath() );
	if ( qt_transl_->load(qtfnm.str(), instdir.str()) )
	    QCoreApplication::installTranslator( qt_transl_ );
    }

#endif

    return true;
}


namespace TextTranslation {

static Threads::Lock mgrlock_( Threads::Lock::SmallWork );

};


// TranslateMgr
mGlobal(Basic) TextTranslation::TranslateMgr& TrMgr()
{
    Threads::Locker lock( TextTranslation::mgrlock_, Threads::Locker::ReadLock);
    mDefineStaticLocalObject( PtrMan<TextTranslation::TranslateMgr>, trmgr,= 0);

    if ( !trmgr )
    {
	if ( lock.convertToWriteLock() )
	{
	    if ( trmgr.setIfNull(new TextTranslation::TranslateMgr,true) )
	    {
		trmgr->reInit();
		return *trmgr;
	    }
	}

	//Fallback, not thread safe
	trmgr = new TextTranslation::TranslateMgr;
	trmgr->reInit();
    }
    return *trmgr;
}


TextTranslation::TranslateMgr::TranslateMgr()
    : curentryidx_(-1)
    , dirtycount_(0)
    , languageChange(this)
    , userlocdir_(GetSettingsFileName(sLocalizationSubDirName()) )
    , applocdir_(mGetApplSetupDataDir(),sLocalizationSubDirName())
    , instlocdir_(mGetSWDirDataDir(),sLocalizationSubDirName())
{
}


void TextTranslation::TranslateMgr::reInit()
{
    const BufferString curlocalekey = localeKey( curentryidx_ );
    deepErase( langentries_ );
    pkgnms_.setEmpty();

    BufferStringSet localekeys;

    QList<QLocale> allLocales = QLocale::matchingLocales( QLocale::AnyLanguage,
	QLocale::AnyScript, QLocale::AnyCountry );
    for (int i = 0; i < allLocales.size(); i++)
	localekeys.add( allLocales[i].name() );

    findLocales( userlocdir_, localekeys );
    findLocales( applocdir_, localekeys );
    findLocales( instlocdir_, localekeys );

    for ( int iloc=0; iloc<localekeys.size(); iloc++ )
    {
	LanguageEntry* le = new LanguageEntry( localekeys.get(iloc), pkgnms_ );
	if ( le->addToEntry() )
	    addEntry( le );
	else
	    delete le;
    }

    if ( !curlocalekey.isEmpty() )
	setLanguageByLocaleKey( curlocalekey );
}


TextTranslation::TranslateMgr::~TranslateMgr()
{
    deepErase( langentries_ );
}


void TextTranslation::TranslateMgr::findLocales( const File::Path& dirfp,
						 BufferStringSet& localekeys )
{
    const DirList dl( dirfp.fullPath(), File::FilesInDir, "*.qm" );
    BufferString fp = dirfp.fullPath();
    for( int idx=0; idx<dl.size(); idx++ )
    {
	BufferString pkgnm;
	const BufferString localenm = getLocaleFromFnm( dl.get(idx), pkgnm );
	if ( !localenm.isEmpty() )
	{
	    pkgnms_.addIfNew( pkgnm );
	    localekeys.addIfNew( localenm );
	}
    }
}


int TextTranslation::TranslateMgr::nrSupportedLanguages() const
{
    return langentries_.size();
}


void TextTranslation::TranslateMgr::addLanguage( LanguageEntry* lentry )
{
    if ( !lentry )
	return;

    addEntry( lentry );
    handleChange();
}


void TextTranslation::TranslateMgr::addEntry( LanguageEntry* lentry )
{
    if ( !lentry )
	return;

    const BufferString newkey = lentry->localeKey();
    if ( getEntry4Locale(lentry->localeKey()) )
	delete lentry;
    else
	langentries_ += lentry;
}


void TextTranslation::TranslateMgr::addLanguages( const char* dirnm,
						  const char* pkgky )
{
#ifdef OD_NO_QT
    return;
#else
    const char* pkgsep = sPackageLanguageSeparator();
    BufferString maskstr( pkgky, pkgsep, "*.qm" );
    const DirList dl( dirnm, File::FilesInDir, maskstr );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	BufferString pkgnm;
	const BufferString locnm = getLocaleFromFnm( dl.get(idx), pkgnm );
	if ( !locnm.isEmpty() && !pkgnm.isEmpty() )
	{
	    LanguageEntry* langentry = getEntry4Locale( locnm );
	    if ( langentry )
	    {
		langentry->addData( dl.fullPath(idx), pkgnm );
		if ( langentry->isLoaded() )
		    langentry->load();
	    }
	    else
	    {
		BufferStringSet pkgnms; pkgnms.add( pkgnm );
		langentry = new LanguageEntry( locnm, pkgnms );
		langentries_ += langentry;
	    }
	}
    }

#endif
}


TextTranslation::LanguageEntry*
TextTranslation::TranslateMgr::getEntry4Locale( const char* lkey ) const
{
    for ( int idx=0; idx<langentries_.size(); idx++ )
	if ( langentries_[idx]->localeKey() == lkey )
	    return const_cast<LanguageEntry*>( langentries_[idx] );
    return 0;
}


uiString TextTranslation::TranslateMgr::languageUserName( int idx ) const
{
    if ( langentries_.validIdx(idx) )
    {
#ifdef OD_NO_QT
	return toUiString( langentries_[idx]->localeKey() );
#else
	uiString ret;
	ret.setFrom( langentries_[idx]->qName() );
	return ret;
#endif
    }

    return ::toUiString( "Unknown language" );
}


BufferString TextTranslation::TranslateMgr::localeKey( int idx ) const
{
    return langentries_.validIdx(idx) ? langentries_[idx]->localeKey()
				      : BufferString();
}


void TextTranslation::TranslateMgr::storeToUserSettings()
{
    Settings& setts = Settings::common();
    const BufferString lky = localeKey( currentLanguageIdx() );
    setts.set( sLocalizationKey(), lky );
    setts.write();
}


void TextTranslation::TranslateMgr::handleChange()
{
    dirtycount_++;
    languageChange.trigger();
}


uiRetVal TextTranslation::TranslateMgr::setLanguage( int idx )
{
    if ( idx < 0 )
	idx = 0;
    if ( idx == curentryidx_ )
	return uiRetVal::OK();

    if ( !langentries_.validIdx(idx) )
	return uiRetVal( mINTERNAL("Bad language index") );

    LanguageEntry& lentry = *langentries_[idx];
    if ( !lentry.load() )
	return uiRetVal( tr("Cannot load %1").arg( languageUserName(idx) ) );

#ifndef OD_NO_QT
    QLocale::setDefault( lentry.qLocale() );
#endif

    curentryidx_ = idx;
    handleChange();
    return uiRetVal::OK();
}


uiRetVal TextTranslation::TranslateMgr::setLanguageByLocaleKey(
					const char* lky )
{
    for ( int idx=0; idx<TrMgr().nrSupportedLanguages(); idx++ )
    {
	if ( localeKey(idx) == lky )
	    { setLanguage( idx ); return uiRetVal::OK(); }
    }
    return uiRetVal( tr("Cannot find locale '%1'").arg( lky ) );
}


int TextTranslation::TranslateMgr::currentLanguageIdx() const
{
    return curentryidx_;
}


const QTranslator* TextTranslation::TranslateMgr::getQTranslator(
					const char* package ) const
{
    const int useidx = langentries_.validIdx(curentryidx_) ? curentryidx_ : 0;
    return langentries_.validIdx(useidx)
	 ? langentries_[useidx]->getQTranslator( package ) : 0;
}


const QLocale* TextTranslation::TranslateMgr::getQLocale() const
{
    const int useidx = langentries_.validIdx(curentryidx_) ? curentryidx_ : 0;
#ifndef OD_NO_QT
    return langentries_.validIdx(useidx) ? &langentries_[useidx]->qLocale() : 0;
#else
    return 0;
#endif
}
