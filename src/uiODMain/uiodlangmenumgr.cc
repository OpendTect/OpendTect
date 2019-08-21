/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		Nov 2017
________________________________________________________________________

-*/

#include "uiodlangmenumgr.h"

#include "uicombobox.h"
#include "uilanguagesel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uiodstdmenu.h"
#include "uistatusbar.h"
#include "uistrings.h"

#include "oddirs.h"
#include "odnetworkaccess.h"
#include "od_iostream.h"
#include "texttranslation.h"

#include "qlocale.h"



uiODLangMenuMgr::uiODLangMenuMgr( uiODMenuMgr& mm )
    : mnumgr_(mm)
    , langmnu_(0)
    , selfld_(0)
{
    setLanguageMenu();

    if ( uiLanguageSel::haveMultipleLanguages() )
    {
	selfld_ = new uiLanguageSel( &mnumgr_.appl_, false );
	selfld_->setAutoCommit( true );
	mnumgr_.appl_.statusBar()->addObject( selfld_->selFld() );
    }

    mAttachCB( TrMgr().languageChange, uiODLangMenuMgr::languageChangeCB );
}


uiODLangMenuMgr::~uiODLangMenuMgr()
{
    detachAllNotifiers();
}


void uiODLangMenuMgr::setLanguageMenu()
{
    if ( langmnu_ )
	langmnu_->removeAllActions();

    const int nrlang = TrMgr().nrSupportedLanguages();
    if ( nrlang < 2 )
	{ delete langmnu_; langmnu_ = 0; return; }

    if ( !langmnu_ )
	langmnu_ = mnumgr_.addSubMenu( mnumgr_.settMnu(),
			uiStrings::sLanguage(), "language" );

    const int trmgridx = TrMgr().currentLanguageIdx();
    for ( int idx=0; idx<nrlang; idx++ )
    {
	uiAction* itm = new uiAction( TrMgr().languageUserName(idx),
			     mCB(this,uiODLangMenuMgr,languageSelectedCB) );
	itm->setCheckable( true );
	itm->setChecked( idx == trmgridx );
	langmnu_->insertAction( itm, mSettLanguageMnu+idx );
    }

    if ( nrlang )
	langmnu_->insertSeparator();

    uiAction* itm = new uiAction( tr( "Fetch latest Translation" ),
	mCB( this, uiODLangMenuMgr, fetchLatestTranslationCB ) );

    langmnu_->insertAction( itm, mSettLanguageMnu + nrlang );
}


void uiODLangMenuMgr::languageChangeCB(CallBacker*)
{
    setLanguageMenu();
}


void uiODLangMenuMgr::languageSelectedCB( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,itm,cb)
    if ( !itm )
	{ pErrMsg("Huh"); return; }

    const int trmgridx = itm->getID() - mSettLanguageMnu;
    if ( trmgridx == TrMgr().currentLanguageIdx() )
	return;

    uiRetVal uirv = TrMgr().setLanguage( trmgridx );
    if ( !uirv.isOK() )
	gUiMsg().error( uirv );

    TrMgr().storeToUserSettings();
}


void uiODLangMenuMgr::fetchLatestTranslationCB( CallBacker* )
{
    uiFetchLatestQMDlg* dlg = new uiFetchLatestQMDlg( &mnumgr_.appl_ );
    if ( dlg->go() )
    {
	File::Path fp( GetSoftwareDir(true) ); fp.add("Data")
	    .add("translations").add( dlg->getFileName() );
	TextTranslation::LanguageEntry* le =
	    new TextTranslation::LanguageEntry( fp.fullPath(),
							dlg->getLocaleName() );
	TrMgr().addLanguage( le );
	setLanguageMenu();

	if ( dlg->changeToLatestQM() )
	    TrMgr().setLanguageByLocaleKey( dlg->getLocaleName() );
    }
    return;
}


//uiFetchLatestQMDlg
uiFetchLatestQMDlg::uiFetchLatestQMDlg( uiParent* p )
    : uiDialog( p, uiDialog::Setup( tr( "Fetch Latest Translation" ), mNoDlgTitle,
	mODHelpKey( mTrackingSetupGroupHelpID ) )
	.oktext( tr( "Download" ) ) )
{
    fetchlanglist_ = new uiLabeledComboBox( this,
	TextTranslation::TranslateMgr::LanguageDef().strings(),
	uiStrings::phrSelect( uiStrings::sLanguage() ), "Check" );
    makecurrtransl_ = new uiCheckBox( this,
	tr( "Switch to selected latest translation" ) );
    makecurrtransl_->attach( rightAlignedBelow, fetchlanglist_ );
    showAlwaysOnTop();
    setCtrlStyle( uiDialog::OkAndCancel );
}


uiFetchLatestQMDlg::~uiFetchLatestQMDlg()
{
}


bool uiFetchLatestQMDlg::acceptOK()
{
    QList<QLocale> allLocales = QLocale::matchingLocales( QLocale::AnyLanguage,
	QLocale::AnyScript, QLocale::AnyCountry );
    BufferString langnm = fetchlanglist_->box()->text();

    for (int i = 0; i < allLocales.size(); i++)
    {
	BufferString localelangnm = QLocale::languageToString(
	    allLocales[i].language() );
	if (langnm == localelangnm)
	{
	    localenm_ = allLocales[i].uiLanguages()[0]; //will take only one form for any language
	    break;
	}
    }

    generateQMLink();

    return true;
}


void uiFetchLatestQMDlg::generateQMLink()
{
    fnm_ = "od_"; fnm_.add( localenm_ ).add( ".qm" );
    File::Path fp( GetSoftwareDir( true ) ); fp.add( "Data" )
	.add( "translations" ).add( fnm_ );
    BufferString url = "https://translations.opendtect.org:8443/download_qmfile.php?key=H3fGFgZ5ejnJHPeQSRuqjhQtttRAwfMJ&project=24&language=";
    url.add( localenm_ );
    uiString errmsg;
    if (!Network::downloadFile( url, fp.fullPath(), errmsg ))
    {
	fp.mkCleanPath( GetSettingsDir(), File::Path::Local );
	fp.add( fnm_ );
	if (!Network::downloadFile( url, fp.fullPath(), errmsg ))
	    uiMSG().error( tr( "Make sure you have write permission and "
		"connected to internet" ) );
	return;
    }

    BufferString userdetails;
    getUserDetails( userdetails );
    if (!userdetails.isEmpty())
    {
	uiString message = tr( "Following user have contributed towards the "
	    "selected language" );
	message.addMoreInfo( toUiString( userdetails ), true );
	uiMSG().message( message );
    }
    return;
}


void uiFetchLatestQMDlg::getUserDetails( BufferString& userdetails )
{
    BufferString str;
    BufferString url = "https://translations.opendtect.org:8443/display_contributors.php?key=H3fGFgZ5ejnJHPeQSRuqjhQtttRAwfMJ&project=24&language=";
    url.add( localenm_ );
    BufferStringSet alluserdetails;
    File::Path fp( GetSettingsDir() );
    fp.add( "translations" ).add( "userdetails.txt" );
    BufferString fppath = fp.fullPath();
    if (Network::getContent( url, str ))
    {
	od_ostream strm( fp );
	strm << str;
	strm.close();
	od_istream istream( fp );
	while (!istream.atEOF())
	{
	    BufferString line;
	    istream.getLine( line );
	    if (line.find( "<td>" ))
	    {
		line.remove( "</td>" );
		line.remove( "<td>" );
		alluserdetails.add( line );
	    }
	}
    }

    remove( fp.fullPath() );


    userdetails.add( alluserdetails.cat() );
}
