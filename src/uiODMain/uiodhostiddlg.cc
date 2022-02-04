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
#include "uilocalhostgrp.h"
#include "uimsg.h"
#include "uitoolbutton.h"

#include "hiddenparam.h"
HiddenParam<uiHostIDDlg,uiLocalHostGrp*>hp_localhostgrp_( nullptr );
uiLocalHostGrp* uiHostIDDlg::localhostgrp()
{
    return hp_localhostgrp_.getParam( this );
}

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
	setTitleText( uiString::emptyString() );
	uiString msg;
	msg.append( errmsg.str(), true );
	msg.append( tr("No HostID found.\n"
			"Please contact support@dgbes.com"), true );
	new uiLabel( this, msg );
	return;
    }

    hostidfld_ = new uiGenInput( this, tr("HostID(s)") );
    hostidfld_->setReadOnly();
    hp_localhostgrp_.setParam( this,
			    new uiLocalHostGrp( this, tr("Computer/Host") ) );
    localhostgrp()->attach( alignedBelow, hostidfld_ );
    osfld_ = new uiGenInput( this, tr("Operating System") );
    osfld_->setStretch( 2, 1 );
    osfld_->setReadOnly();
    osfld_->attach( alignedBelow, localhostgrp() );
    productnmfld_ = new uiGenInput( this, tr("OS Product name") );
    productnmfld_->setStretch( 2, 1 );
    productnmfld_->setReadOnly();
    productnmfld_->attach( alignedBelow, osfld_ );
    usernmfld_ = new uiGenInput( this, tr("User name") );
    usernmfld_->setStretch( 2, 1 );
    usernmfld_->setReadOnly();
    usernmfld_->attach( alignedBelow, productnmfld_ );

    BufferString hostidstext = hostids.cat( " " );
    if ( hostids.size() > 1 )
	hostidstext.quote( '"' );

    hostidfld_->setText( hostidstext );
    osfld_->setText( OD::Platform().longName() );
    productnmfld_->setText( System::productName() );
    usernmfld_->setText( GetUserNm() );

    auto* but = new uiToolButton( this, "clipboard", tr("Copy to Clipboard"),
				  mCB(this,uiHostIDDlg,copyCB) );

    but->attach( rightTo, hostidfld_ );
}


uiHostIDDlg::~uiHostIDDlg()
{
    hp_localhostgrp_.removeParam( this );
}

void uiHostIDDlg::copyCB( CallBacker* )
{
    BufferString txt;
    txt.add( "HostIDs: " ).add( hostidfld_->text() ).addNewLine()
       .add( "Host name: " ).add( localhostgrp()->hostname() ).addNewLine()
       .add( "IP Address: " ).add( localhostgrp()->address() ).addNewLine()
       .add( "Operating System: " ).add( osfld_->text() ).addNewLine()
       .add( "Product name: " ).add( productnmfld_->text() ).addNewLine()
       .add( "User name: " ).add( usernmfld_->text() ).addNewLine();
    uiClipboard::setText( txt.buf() );
    uiMSG().message( tr("Information copied to clipboard.\n"
			"Paste in an email and send to support@dgbes.com") );
}
