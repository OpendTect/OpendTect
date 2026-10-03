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

#include "hiddenparam.h"

extern "C" { mGlobal(Basic) void SetLocalHostNameOverrule(const char*); }


static HiddenParam<uiLocalHostGrp,uiGenInput*> localhostgrphpmgr_(nullptr);

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
	mAttachCB( hostnmoverrulefld_->valueChanged,
		   uiLocalHostGrp::hostnmoverruleCB);
	attachobj = hostnmoverrulefld_->attachObj();
    }

    hostaddrfld_ = new uiGenInput( this,
			   uiStrings::phrJoinStrings( txt, tr("Address") ) );
    hostaddrfld_->setReadOnly();
    hostaddrfld_->attach( alignedBelow, attachobj );
    attachobj = hostaddrfld_->attachObj();

    auto* subnetfld = new uiGenInput( this, tr("Subnet Mask") );
    subnetfld->setReadOnly();
    subnetfld->attach( alignedBelow, attachobj );
    attachobj = subnetfld->attachObj();
    localhostgrphpmgr_.setParam( this, subnetfld );

    const StringView domainnm = System::localDomainName();
    if ( !domainnm.isEmpty() )
    {
	domainfld_ = new uiGenInput( this, tr("Domain name") );
	domainfld_->setText( domainnm );
	domainfld_->setReadOnly();
	domainfld_->attach( alignedBelow, attachobj );
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
    localhostgrphpmgr_.removeParam( this );
}


void uiLocalHostGrp::setHSzPol( uiObject::SzPolicy szpol )
{
    hostnmfld_->setElemSzPol( szpol );
    hostaddrfld_->setElemSzPol( szpol );
    subnetfld_()->setElemSzPol( szpol );
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


BufferString uiLocalHostGrp::subnet() const
{
    return subnetfld_()->text();
}


const uiGenInput* uiLocalHostGrp::subnetfld_() const
{
    return localhostgrphpmgr_.getParam( getNonConst(this) );
}


uiGenInput* uiLocalHostGrp::subnetfld_()
{
    return localhostgrphpmgr_.getParam( this );
}


bool uiLocalHostGrp::overruleOK() const
{
    const BufferString overrule( hostnmoverrulefld_->text() );
    if ( overrule.isEmpty() )
	return false;

    const BufferString address = System::hostAddress( overrule );
    if ( address.isEmpty() )
    {
	uiMSG().error(tr("No address found for overrule host: %1")
				.arg(overrule));
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
	    subnetfld_()->setEmpty();
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
    const BufferString addr( System::localAddress() );
    hostaddrfld_->setText( addr );

    BufferString netmask;
    int prefixlength = -1;
    if ( System::getLocalNetMask(addr,netmask,prefixlength) &&
	 prefixlength >= 0 )
    {
	subnetfld_()->setText( BufferString(netmask)
				.add( " (/" ).add( prefixlength ).add( ")" ) );
    }
    else
	subnetfld_()->setEmpty();
}
