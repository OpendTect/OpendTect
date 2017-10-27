/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
#include "filepath.h"
#include "file.h"
#include "legal.h"
#include "oddirs.h"
#include "od_istream.h"
#include "odver.h"
#include "oscommand.h"
#include "osgver.h"


#define mInsertItem(mnu,txt,id,sc) \
{ \
    uiAction* itm = new uiAction(txt,mCB(mnumgr_,uiODMenuMgr,handleClick));\
    mnu->insertItem( itm, id ); \
    itm->setShortcut( sc ); \
}


uiODHelpMenuMgr::uiODHelpMenuMgr( uiODMenuMgr* mm )
    : helpmnu_( mm->helpMnu() )
    , mnumgr_( mm )
{
    docmnu_ = new uiMenu( tr("Documentation") );
    helpmnu_->addMenu( docmnu_ );
    mInsertItem( docmnu_, tr("OpendTect"), mUserDocMnuItm, "F1" );

    if ( HelpProvider::hasHelp(HelpKey(DevDocHelp::sKeyFactoryName(),0)))
	mInsertItem( docmnu_, tr("Programmer"), mProgrammerMnuItm, 0 );

    if ( HelpProvider::hasHelp(HelpKey("appman",0)) )
	mInsertItem( docmnu_, tr("Admin"), mAdminMnuItm, 0 );

    mInsertItem( helpmnu_, tr("How-To Instructions"), mWorkflowsMnuItm, 0 );
    mInsertItem( helpmnu_, tr("Training Manual"), mTrainingManualMnuItm, 0 );
    mInsertItem( helpmnu_, tr("Attributes Table"), mAttribMatrixMnuItm, 0 );
    mInsertItem( helpmnu_, tr("Online Support"), mSupportMnuItm, 0 );
    mInsertItem( helpmnu_, tr("Keyboard shortcuts"),
		 mShortcutsMnuItm, "?" );
    mInsertItem( helpmnu_, tr("About"), mAboutMnuItm, 0);

    if ( legalInformation().size() )
	mInsertItem( helpmnu_, tr("Legal"), mLegalMnuItm, 0);
}


uiODHelpMenuMgr::~uiODHelpMenuMgr()
{
}


uiMenu* uiODHelpMenuMgr::getDocMenu()
{ return docmnu_; }


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
	    uiMSG().aboutOpendTect( getAboutString() );
	} break;
	case mAdminMnuItm:
	{
	    HelpProvider::provideHelp( HelpKey("appman",0) );
	} break;
	case mProgrammerMnuItm:
	{
	    HelpProvider::provideHelp(HelpKey(DevDocHelp::sKeyFactoryName(),0));
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

    const BufferString title = "Keyboard Shortcuts and Mouse Controls";
    BufferString cmd = "od_ImageViewer --bg ";
    cmd.add( imgpath ).addSpace().add( "'" ).add( title ).add( "'" );
    const bool res = OS::ExecCommand( cmd.buf(), OS::RunInBG );
    if ( !res )
	uiMSG().error( tr("Could not launch ShortKeys table") );
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

    textsel_ = new uiGenInput( this, tr("Project"),
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
    const BufferString key = legalInformation().getNames()[sel]->buf();

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
