/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Sep 2010
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "uigmtinfodlg.h"

#include "uibutton.h"
#include "uidesktopservices.h"
#include "uifiledlg.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uimsg.h"

#include "envvars.h"
#include "file.h"
#include "pixmap.h"

uiGMTInfoDlg::uiGMTInfoDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("GMT Mapping Tool",mNoDlgTitle,mNoHelpID))
{
    setOkText( "Continue" );

    BufferString msg = "You need to install the GMT mapping tool package\n";
    msg += "before you can use this utility\n";
    msg += "Also make sure that the environment";
    msg += " variable GMT_SHAREDIR is to set ";

    uiLabel* lbl = new uiLabel( this, msg );
    lbl->setAlignment( Alignment::HCenter );

    uiPushButton* gmtbut = new uiPushButton( this, "Download GMT",
	    			      mCB(this,uiGMTInfoDlg,gmtPushCB),true );
    gmtbut->setToolTip( "Click to go to the Download centre" );
    gmtbut->attach( centeredBelow, lbl );

    chkbut_ = new uiCheckBox( this, "Software installed already",
	    				 mCB(this,uiGMTInfoDlg,selCB) );
    chkbut_->attach( alignedBelow, gmtbut );

    label_ = new uiLabel( this, "Please set GMT_SHAREDIR path" );
    label_->attach( alignedBelow, chkbut_ );
    label_->display( false );

    gmtpath_ = new uiFileInput( this, "Enter path",
		    uiFileInput::Setup(uiFileDialog::Gen).directories(true) );
    gmtpath_->attach( alignedBelow, label_ );
    gmtpath_->display( false );
}


void uiGMTInfoDlg::gmtPushCB( CallBacker* )
{
    uiDesktopServices::openUrl(
	    __islinux__ ? "http://www.opendtect.org/index.php/download.html"
	    		: "http://www.soest.hawaii.edu/gmt" );
}


bool uiGMTInfoDlg::acceptOK( CallBacker* )
{
    const char* envvar = GetEnvVar( "GMT_SHAREDIR" );
    if ( envvar )
	return true;

    chkbut_->setChecked( true );
    const BufferString path = gmtpath_->text();
    if ( path.isEmpty() )
    {
	uiMSG().message( "Please enter path" );
	return false;
    }

    if ( !File::exists( path ) )
    {
	uiMSG().message( "Directory not existed" );
	return false;
    }

    const int envwrite = WriteEnvVar( "GMT_SHAREDIR", path );
    if ( !envwrite )
	return false;
    
    uiMSG().message( "This option will work after restart OpendTect" );

    return true;
}


void uiGMTInfoDlg::selCB( CallBacker* )
{
    const char* envvar = GetEnvVar( "GMT_SHAREDIR" );
    if ( !envvar )
    {
	label_->display( chkbut_->isChecked() );
	gmtpath_->display( chkbut_->isChecked() );
    }
}
