/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Sep 2010
________________________________________________________________________

-*/


#include "uigmtinfodlg.h"

#include "uibutton.h"
#include "uidesktopservices.h"
#include "uifiledlg.h"
#include "uifilesel.h"
#include "uilabel.h"
#include "uimsg.h"

#include "envvars.h"
#include "file.h"

uiGMTInfoDlg::uiGMTInfoDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("GMT Mapping Tool"),mNoDlgTitle,mNoHelpKey))
{
    setOkText( uiStrings::sContinue() );

    uiString msg = tr("You need to install the GMT mapping tool package\n"
                      "before you can use this utility\n"
                      "Also make sure that the environment"
                      " variable GMT_SHAREDIR is to set ");

    uiLabel* lbl = new uiLabel( this, msg );
    lbl->setAlignment( OD::Alignment::HCenter );

    uiPushButton* gmtbut = new uiPushButton( this, tr("Download GMT"),
				      mCB(this,uiGMTInfoDlg,gmtPushCB),true );
    gmtbut->setToolTip( tr("Click to go to the Download centre") );
    gmtbut->attach( centeredBelow, lbl );

    chkbut_ = new uiCheckBox( this, tr("Software installed already"),
					 mCB(this,uiGMTInfoDlg,selCB) );
    chkbut_->attach( alignedBelow, gmtbut );

    label_ = new uiLabel( this, tr("Please set GMT_SHAREDIR") );
    label_->attach( alignedBelow, chkbut_ );
    label_->display( false );

    uiFileSel::Setup fssu; fssu.selectDirectory();
    gmtpath_ = new uiFileSel( this, uiStrings::sDirectory(), fssu );
    gmtpath_->attach( alignedBelow, label_ );
    gmtpath_->display( false );
}


void uiGMTInfoDlg::gmtPushCB( CallBacker* )
{
    uiDesktopServices::openUrl(
	    __islinux__ ? "http://www.opendtect.org/index.php/download"
		: "http://gmt.soest.hawaii.edu/projects/gmt/wiki/Download" );
}


bool uiGMTInfoDlg::acceptOK()
{
    const BufferString envvar = GetEnvVar( "GMT_SHAREDIR" );
    if ( !envvar.isEmpty() )
	return true;

    chkbut_->setChecked( true );
    const BufferString path = gmtpath_->text();
    if ( path.isEmpty() )
    {
	uiMSG().error( uiStrings::phrEnter(tr("GMT Path")) );
	return false;
    }

    if ( !File::exists( path ) )
    {
	uiMSG().error( tr("Directory does not exist") );
	return false;
    }

    const int envwrite = WriteEnvVar( "GMT_SHAREDIR", path );
    if ( !envwrite )
	return false;

    uiMSG().message( tr("This option will work after you restart OpendTect") );
    return true;
}


void uiGMTInfoDlg::selCB( CallBacker* )
{
    const BufferString envvar = GetEnvVar( "GMT_SHAREDIR" );
    if ( envvar.isEmpty() )
    {
	label_->display( chkbut_->isChecked() );
	gmtpath_->display( chkbut_->isChecked() );
    }
}
