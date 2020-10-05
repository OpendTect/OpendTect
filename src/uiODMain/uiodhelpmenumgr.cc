/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Dec 2003
________________________________________________________________________

-*/

#include "uiodhelpmenumgr.h"

#include "uidialog.h"
#include "uidesktopservices.h"
#include "uigeninput.h"
#include "uihelpview.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uiodstdmenu.h"
#include "uitextedit.h"

#include "buildinfo.h"
#include "file.h"
#include "legal.h"
#include "oddirs.h"
#include "od_istream.h"
#include "odver.h"
#include "oscommand.h"
#include "osgver.h"


#define mAddHelpAction(txt,id,icon) \
    mnumgr_.addAction( helpmnu_, txt, icon, id )


uiODHelpMenuMgr::uiODHelpMenuMgr( uiODMenuMgr& mm )
    : helpmnu_( mm.helpMnu() )
    , mnumgr_( mm )
{
    docmnu_ = mnumgr_.addSubMenu( helpmnu_, tr("Documentation"),
				    "documentation" );
    uiAction* act = mnumgr_.addAction( docmnu_, uiStrings::sOpendTect(),
					"od", mUserDocMnuItm );
    act->setShortcut( "F1" );

    mnumgr_.addAction( docmnu_, tr("Programmer"), "programmer",
			mProgrammerDocMnuItm );

    mnumgr_.addAction( docmnu_, tr("System Administration"),
			"sysadm", mAdminDocMnuItm );

    mAddHelpAction( tr("How-To Instructions"), mWorkflowsMnuItm, "howto" );
    mAddHelpAction( tr("Training Manual"), mTrainingManualMnuItm, "training" );
    mAddHelpAction( tr("Attributes Table"), mAttribMatrixMnuItm,
		    "attributematrix" );
    mAddHelpAction( tr("Online Support"), mSupportMnuItm, "internet" );
    act = mAddHelpAction( tr("Keyboard shortcuts"), mShortcutsMnuItm,
			  "keyboard" );
    act->setShortcut( "?" );
    mAddHelpAction( uiStrings::sAbout(), mAboutMnuItm, "info" );

    if ( legalInformation().size() )
	mAddHelpAction( tr("Legal"), mLegalMnuItm, "legal" );
}


uiODHelpMenuMgr::~uiODHelpMenuMgr()
{
}


uiMenu* uiODHelpMenuMgr::getDocMenu()
{
    return docmnu_;
}


void uiODHelpMenuMgr::handle( int id )
{
    switch( id )
    {
	case mLegalMnuItm:
	{
	    showLegalInfo();
	} break;
	case mAboutMnuItm:
	{
	    gUiMsg().aboutOpendTect( getAboutString() );
	} break;
	case mAdminDocMnuItm:
	{
	    HelpProvider::provideHelp( HelpKey("admin",0) );
	} break;
	case mProgrammerDocMnuItm:
	{
	    HelpProvider::provideHelp( HelpKey("dev",0) );
	} break;
	case mSupportMnuItm:
	{
	    const HelpKey key( WebsiteHelp::sKeyFactoryName(),
			       WebsiteHelp::sKeySupport() );
	    HelpProvider::provideHelp( key );
	} break;
	case mTrainingManualMnuItm:
	{
	    HelpProvider::provideHelp( HelpKey("tm",0) );
	} break;
	case mWorkflowsMnuItm:
	{
	    HelpProvider::provideHelp( HelpKey("wf",0) );
	} break;
	case mAttribMatrixMnuItm:
	{
	    const HelpKey key( WebsiteHelp::sKeyFactoryName(),
			       WebsiteHelp::sKeyAttribMatrix() );
	    HelpProvider::provideHelp( key );
	} break;
	case mShortcutsMnuItm:
	{
	    showShortKeys();
	} break;
	default:
	{
	    HelpProvider::provideHelp( HelpKey("od",0) );
	}
    }
}


void uiODHelpMenuMgr::showShortKeys()
{
    const BufferString imgpath =
	GetSetupDataFileName( ODSetupLoc_SWDirOnly, "shortkeys.png", false );
    if ( !File::exists(imgpath.buf()) )
	return;

    const char* title = "Keyboard Shortcuts and Mouse Controls";
    OS::MachineCommand machcomm( "od_ImageViewer", imgpath, title );
    OS::CommandLauncher cl( machcomm );
    if ( !cl.execute(OS::RunInBG) )
	gUiMsg().error( cl.errorMsg() );
}


uiString uiODHelpMenuMgr::getAboutString()
{
    BufferString str( "<html>" );
    str.set( "<h2>OpendTect v" ).add( GetFullODVersion() ).add("</h2><br>");

    str.add( "Built on " ).add( mBUILD_DATE ).add( "<br>" )
       .add( "From revision " ).add( mVCS_VERSION ).add( "<br><br>" );

    str.add( "Based on Qt " ).add( GetQtVersion() )
       .add( ", OSG " ).add( GetOSGVersion() ).add( ",<br>" )
       .add( GetCompilerVersionStr() ).add( "<br><br>" );

    str.add( mCOPYRIGHT_STRING ).add( "<br>" )
       .add( "OpendTect is released under a triple licensing scheme. "
	     "For more information, click "
	     "<a href=\"http://dgbes.com/index.php/products/licenses\">"
	     "here</a>.<br>" );
    str.add( "</html>" );
    return toUiString(str);
}


class uiODLegalInfo : public uiDialog
{ mODTextTranslationClass(uiODLegalInfo);
public:

uiODLegalInfo(uiParent* p)
    : uiDialog(p,uiDialog::Setup(tr("Legal information"),
				 mNoDlgTitle,mNoHelpKey))
{
    setCtrlStyle( CloseOnly );
    uiLabel* label = new uiLabel( this,
	    tr("OpendTect has incorporated code from various "
	       "open source projects that are licensed under different "
	       "licenses.") );

    textsel_ = new uiGenInput( this, uiStrings::sProject(),
		StringListInpSpec(legalInformation().getUserNames()) );
    textsel_->valuechanged.notify( mCB(this,uiODLegalInfo,selChgCB) );
    textsel_->attach( alignedBelow, label );

    textfld_ = new uiTextEdit( this );
    textfld_->attach( alignedBelow, textsel_ );

    selChgCB( 0 );
}

private:

void selChgCB( CallBacker* )
{
    const int sel = textsel_->getIntValue();
    const char* key = legalInformation().key( sel );

    PtrMan<uiString> newtext = legalInformation().create( key );
    if ( newtext )
	textfld_->setText( *newtext );
    else
	textfld_->setText( "" );
}

    uiGenInput*		textsel_;
    uiTextEdit*		textfld_;
};


void uiODHelpMenuMgr::showLegalInfo()
{
    uiODLegalInfo dlg( ODMainWin() );
    dlg.go();
}
