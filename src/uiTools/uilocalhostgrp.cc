/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uilocalhostgrp.h"
#include "settingsaccess.h"
#include "systeminfo.h"

#include "uigeninput.h"
#include "uimsg.h"

extern "C" { mGlobal(Basic) void SetLocalHostNameOverrule(const char*); }


uiLocalHostGrp::uiLocalHostGrp( uiParent* p, const uiString& txt,
				bool withoverride )
	: uiGroup(p,"Local Host")
{
    uiString lbl( txt );
    hostnmfld_ = new uiGenInput( this,
			 uiStrings::phrJoinStrings( txt, uiStrings::sName() ) );
    hostnmfld_->setReadOnly();
    uiObject* attachobj = hostnmfld_->attachObj();

    if ( withoverride )
    {
	hostnmoverrulefld_ = new uiGenInput( this,
	 uiStrings::phrJoinStrings( txt, uiStrings::sName(), tr("Overrule") ) );
	hostnmoverrulefld_->setWithCheck( true );
	hostnmoverrulefld_->attach( alignedBelow, hostnmfld_ );
	mAttachCB( hostnmoverrulefld_->checked,
		   uiLocalHostGrp::overrulecheckedCB);
	mAttachCB( hostnmoverrulefld_->valuechanged,
		   uiLocalHostGrp::hostnmoverruleCB);
	attachobj = hostnmoverrulefld_->attachObj();
    }

    hostaddrfld_ = new uiGenInput( this,
			   uiStrings::phrJoinStrings( txt, tr("Address") ) );
    hostaddrfld_->setReadOnly();
    hostaddrfld_->attach( alignedBelow, attachobj );

    const StringView domainnm = System::localDomainName();
    if ( !domainnm.isEmpty() )
    {
	domainfld_ = new uiGenInput( this, tr("Domain name") );
	domainfld_->setText( domainnm );
	domainfld_->setReadOnly();
	domainfld_->attach( alignedBelow, hostaddrfld_ );
    }

    setHAlignObj( hostnmfld_ );

    hostnmfld_->setText( System::localHostName() );
    if ( hostnmoverrulefld_ )
	hostnmoverrulefld_->setText( SettingsAccess().getHostNameOverrule() );
    lookupaddrCB( nullptr );
}


uiLocalHostGrp::~uiLocalHostGrp()
{
    detachAllNotifiers();
}


void uiLocalHostGrp::setHSzPol( uiObject::SzPolicy szpol )
{
    hostnmfld_->setElemSzPol( szpol );
    hostaddrfld_->setElemSzPol( szpol );
    if ( hostnmoverrulefld_ )
	hostnmoverrulefld_->setElemSzPol( szpol );
    if ( domainfld_ )
	domainfld_->setElemSzPol( szpol );
}


BufferString uiLocalHostGrp::hostname() const
{
    return hostnmfld_->text();
}


BufferString uiLocalHostGrp::address() const
{
    return hostaddrfld_->text();
}


bool uiLocalHostGrp::overruleOK() const
{
    const BufferString overrule( hostnmoverrulefld_->text() );
    if ( overrule.isEmpty() )
	return false;

    const BufferString address = System::hostAddress( overrule );
    if ( address.isEmpty() )
    {
	uiMSG().error(tr("No address found for overrule host: %1").
								arg(overrule));
	return false;
    }
    return true;
}


void uiLocalHostGrp::hostnmoverruleCB( CallBacker* )
{
    if ( hostnmoverrulefld_->isChecked()  )
    {
	if ( overruleOK() )
	{
	    SetLocalHostNameOverrule( hostnmoverrulefld_->text() );
	    SettingsAccess().setHostNameOverrule( hostnmoverrulefld_->text() );
	}
	else
	{
	    hostaddrfld_->setEmpty();
	    return;
	}
    }

    lookupaddrCB( nullptr );
}


void uiLocalHostGrp::overrulecheckedCB( CallBacker* )
{
    if ( !hostnmoverrulefld_->isChecked() )
	SetLocalHostNameOverrule( nullptr );

    hostnmoverruleCB( nullptr );
}


void uiLocalHostGrp::lookupaddrCB( CallBacker* )
{
    hostaddrfld_->setText( System::localAddress() );
}
