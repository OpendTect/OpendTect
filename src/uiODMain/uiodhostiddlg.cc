/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		March 2020
___________________________________________________________________

-*/

#include "uiodhostiddlg.h"

#include "bufstring.h"
#include "generalinfo.h"
#include "oddirs.h"
#include "odplatform.h"
#include "systeminfo.h"

#include "uiclipboard.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitoolbutton.h"

uiHostIDDlg::uiHostIDDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Host Information"),mNoDlgTitle,mNoHelpKey))
{
    setCtrlStyle( CloseOnly );
    setTitleText( tr("Information needed to generate a license") );

    BufferStringSet hostids;
    BufferString errmsg;
    OD::getHostIDs( hostids, errmsg );
    if ( hostids.isEmpty() )
    {
	setTitleText( uiString::empty() );
	uiString msg;
	msg.append( errmsg.str(), true );
	msg.append( tr("No HostID found.\n"
			"Please contact support@dgbes.com"), true );
	new uiLabel( this, msg );
	return;
    }

    hostidfld_ = new uiGenInput( this, tr("HostID(s)") );
    hostidfld_->setReadOnly();
    hostnmfld_ = new uiGenInput( this, tr("Computer/Host name") );
    hostnmfld_->setReadOnly();
    hostnmfld_->attach( alignedBelow, hostidfld_ );
    osfld_ = new uiGenInput( this, tr("Operating System") );
    osfld_->setReadOnly();
    osfld_->attach( alignedBelow, hostnmfld_ );
    usernmfld_ = new uiGenInput( this, tr("User name") );
    usernmfld_->setReadOnly();
    usernmfld_->attach( alignedBelow, osfld_ );

    BufferString hostidstext = hostids.cat( " " );
    if ( hostids.size() > 1 )
	hostidstext.quote( '"' );

    hostidfld_->setText( hostidstext );
    hostnmfld_->setText( System::localHostName() );
    osfld_->setText( OD::Platform().longName() );
    usernmfld_->setText( GetUserNm() );

    auto* but = new uiToolButton( this, "clipboard", tr("Copy to Clipboard"),
				  mCB(this,uiHostIDDlg,copyCB) );

    but->attach( rightTo, hostidfld_ );
}


void uiHostIDDlg::copyCB( CallBacker* )
{
    BufferString txt;
    txt.add( "HostIDs: " ).add( hostidfld_->text() ).addNewLine()
       .add( "Host name: " ).add( hostnmfld_->text() ).addNewLine()
       .add( "Operating System: " ).add( osfld_->text() ).addNewLine()
       .add( "User name: " ).add( usernmfld_->text() ).addNewLine();
    uiClipboard::setText( toUiString(txt.buf()) );
    gUiMsg( this ).message( tr("Information copied to clipboard.\n"
			"Paste in an email and send to support@dgbes.com") );
}
