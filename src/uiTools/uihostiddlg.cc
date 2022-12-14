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
    setOkCancelText( tr("Copy to Clipboard"), uiStrings::sClose() );
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
    hostidfld_->setElemSzPol( uiObject::Wide );

    localhostgrp_ = new uiLocalHostGrp( this, tr("Computer/Host") );
    localhostgrp_->attach( alignedBelow, hostidfld_ );
    localhostgrp_->setHSzPol( uiObject::Wide );

    timezonefld_ = new uiGenInput( this, tr("Time Zone") );
    timezonefld_->setReadOnly();
    timezonefld_->attach( alignedBelow, localhostgrp_ );
    timezonefld_->setElemSzPol( uiObject::Wide );

    osfld_ = new uiGenInput( this, tr("Operating System") );
    osfld_->setReadOnly();
    osfld_->attach( alignedBelow, timezonefld_ );
    osfld_->setElemSzPol( uiObject::Wide );

    productnmfld_ = new uiGenInput( this, tr("OS Product name") );
    productnmfld_->setReadOnly();
    productnmfld_->attach( alignedBelow, osfld_ );
    productnmfld_->setElemSzPol( uiObject::Wide );

    usernmfld_ = new uiGenInput( this, tr("User name") );
    usernmfld_->setReadOnly();
    usernmfld_->attach( alignedBelow, productnmfld_ );
    usernmfld_->setElemSzPol( uiObject::Wide );

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

    mAttachCB( postFinalize(), uiHostIDDlg::finalizeCB );
}


uiHostIDDlg::~uiHostIDDlg()
{
    detachAllNotifiers();
}


void uiHostIDDlg::finalizeCB( CallBacker* )
{
    button(OK)->setIcon( "clipboard" );
}


bool uiHostIDDlg::acceptOK( CallBacker* )
{
    copyToClipboard();
    return false;
}


void uiHostIDDlg::copyToClipboard()
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
			"When requesting a license, you can now paste the\n"
			"contents in an email and send to support@dgbes.com") );
}
