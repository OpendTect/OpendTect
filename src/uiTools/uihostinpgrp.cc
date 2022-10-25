/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uihostinpgrp.h"
#include "systeminfo.h"

#include "uigeninput.h"
#include "uimsg.h"


mDefineEnumUtils(uiHostInpGrp,LookupMode,"Host resolution")
{
    "Static IP",
    "Hostname DNS",
    nullptr
};


uiHostInpGrp::uiHostInpGrp( uiParent* p, const uiString& txt )
    : uiGroup(p,"Host input")
    , hostdata_(System::localHostName())
{
    modefld_ = new uiGenInput( this, txt,
			       StringListInpSpec(LookupModeDef().strings()) );

    lookupfld_ = new uiGenInput( this, uiString::empty(),
				 BoolInpSpec(false, tr("Autofill"),
					     uiString::empty()) );
    lookupfld_->attach( alignedBelow, modefld_ );
    lookupfld_->setValue( true );


    hostnmfld_ = new uiGenInput( this, uiStrings::sName() );
    hostnmfld_->attach( rightOf, modefld_ );
    hostnmfld_->setText( hostdata_.getHostName() );

    hostaddrfld_= new uiGenInput( this, tr("IP")  );
    hostaddrfld_->attach( alignedBelow, hostnmfld_ );
    hostaddrfld_->setText( hostdata_.getIPAddress() );


    modeChgCB( nullptr );
    lookupCB( nullptr );
    mAttachCB( modefld_->valuechanged, uiHostInpGrp::modeChgCB );
    mAttachCB( hostnmfld_->valuechanged, uiHostInpGrp::lookupCB );
    mAttachCB( hostaddrfld_->valuechanged, uiHostInpGrp::lookupCB );

    setHAlignObj( modefld_ );

}


uiHostInpGrp::~uiHostInpGrp()
{
    detachAllNotifiers();
}


bool uiHostInpGrp::isStaticIP() const
{
    return
	LookupModeDef().getEnumValForIndex(modefld_->getIntValue())==StaticIP;
}


void uiHostInpGrp::modeChgCB( CallBacker* )
{
    const bool isstatic = isStaticIP();
    hostnmfld_->setSensitive( !isstatic );
    hostaddrfld_->setSensitive( isstatic );
}


void uiHostInpGrp::lookupCB( CallBacker* )
{
    if ( !lookupfld_->getBoolValue() )
	return;

    if ( isStaticIP() )
    {
	const BufferString ipaddr( hostaddrfld_->text() );
	if ( !System::isValidIPAddress(ipaddr) )
	{
	    uiMSG().error( tr("Invalid IP address") );
	    return;
	}

	hostdata_.setIPAddress( ipaddr );
	NotifyStopper ns( hostnmfld_->valuechanged );
	hostnmfld_->setText( hostdata_.getHostName() );
    }
    else
    {
	hostdata_.setHostName( hostnmfld_->text() );
	NotifyStopper ns( hostaddrfld_->valuechanged );
	hostaddrfld_->setText( hostdata_.getIPAddress() );
    }
}


HostData uiHostInpGrp::hostData() const
{
    return hostdata_;
}


void uiHostInpGrp::setHostData( const HostData& hd )
{
    hostdata_ = hd;
    lookupCB( nullptr );
}
