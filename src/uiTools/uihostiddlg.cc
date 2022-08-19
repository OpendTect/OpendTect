/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uihostiddlg.h"

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

#include <QTimeZone>

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
    localhostgrp_ = new uiLocalHostGrp( this, tr("Computer/Host") );
    localhostgrp_->attach( alignedBelow, hostidfld_ );

    timezonefld_ = new uiGenInput( this, tr("Time Zone") );
    timezonefld_->setReadOnly();
    timezonefld_->attach( alignedBelow, localhostgrp_ );

    osfld_ = new uiGenInput( this, tr("Operating System") );
    osfld_->setStretch( 2, 1 );
    osfld_->setReadOnly();
    osfld_->attach( alignedBelow, timezonefld_ );
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
    const QTimeZone qloczone = QTimeZone::systemTimeZone();
    const QTimeZone::TimeType ttyp = QTimeZone::GenericTime;
    const QString qloczoneabbr = qloczone.displayName( ttyp,
						       QTimeZone::ShortName );
    BufferString zonestr( qloczoneabbr );
    if ( !__iswin__ )
    {
	const QString qloczoneoffs = qloczone.displayName( ttyp,
						QTimeZone::OffsetName );
	zonestr.add( " (" ).add( qloczoneoffs ).add( ")" );
    }

    hostidfld_->setText( hostidstext );
    timezonefld_->setText( zonestr );
    osfld_->setText( OD::Platform().longName() );
    productnmfld_->setText( System::productName() );
    usernmfld_->setText( GetUserNm() );

    auto* but = new uiToolButton( this, "clipboard", tr("Copy to Clipboard"),
				  mCB(this,uiHostIDDlg,copyCB) );

    but->attach( rightTo, hostidfld_ );
}


uiHostIDDlg::~uiHostIDDlg()
{
}

void uiHostIDDlg::copyCB( CallBacker* )
{
    BufferString txt;
    txt.add( "HostIDs: " ).add( hostidfld_->text() ).addNewLine()
       .add( "Host name: " ).add( localhostgrp_->hostname() ).addNewLine()
       .add( "IP Address: " ).add( localhostgrp_->address() ).addNewLine()
       .add( "Time Zone: " ).add( timezonefld_->text() ).addNewLine()
       .add( "Operating System: " ).add( osfld_->text() ).addNewLine()
       .add( "Product name: " ).add( productnmfld_->text() ).addNewLine()
       .add( "User name: " ).add( usernmfld_->text() ).addNewLine();
    uiClipboard::setText( txt.buf() );
    uiMSG().message( tr("Information copied to clipboard.\n"
			"Paste in an email and send to support@dgbes.com") );
}
