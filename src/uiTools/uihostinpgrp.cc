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

static const char* sKeyLookupMode()		{ return "Lookup Mode"; }
static const char* sKeyAutofill()		{ return "Autofill"; }

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
		BoolInpSpec(true,tr("Autofill"),uiString::empty()) );
    lookupfld_->attach( rightOf, modefld_ );

    hostaddrfld_ = new uiGenInput( this, tr("IP address")  );
    hostaddrfld_->attach( alignedBelow, modefld_ );
    hostaddrfld_->setText( hostdata_.getIPAddress() );

    hostnmfld_ = new uiGenInput( this, uiStrings::sHostName() );
    hostnmfld_->attach( rightOf, hostaddrfld_ );
    hostnmfld_->setText( hostdata_.getHostName() );
    hostnmfld_->setElemSzPol( uiObject::Wide );

    modeChgCB( nullptr );
    lookupCB( nullptr );
    mAttachCB( modefld_->valueChanged, uiHostInpGrp::modeChgCB );
    mAttachCB( hostnmfld_->valueChanged, uiHostInpGrp::lookupCB );
    mAttachCB( hostaddrfld_->valueChanged, uiHostInpGrp::lookupCB );

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
	NotifyStopper ns( hostnmfld_->valueChanged );
	hostnmfld_->setText( hostdata_.getHostName() );
    }
    else
    {
	hostdata_.setHostName( hostnmfld_->text() );
	NotifyStopper ns( hostaddrfld_->valueChanged );
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


void uiHostInpGrp::fillPar( IOPar& par ) const
{
    const int midx = modefld_->getIntValue();
    par.set( sKeyLookupMode(), LookupModeDef().getKeyForIndex(midx) );
    par.setYN( sKeyAutofill(), lookupfld_->getBoolValue() );
    if ( isStaticIP() )
	par.set( sKey::IPAddress(), hostaddrfld_->text() );
    else
	par.set( sKey::Hostname(), hostnmfld_->text() );
}


bool uiHostInpGrp::usePar( const IOPar& par )
{
    LookupMode mode;
    if ( LookupModeDef().parse(par,sKeyLookupMode(),mode) )
	 modefld_->setValue( int(mode) );

    bool autofill;
    if ( par.getYN(sKeyAutofill(),autofill) )
	lookupfld_->setValue( autofill );

    if ( isStaticIP() )
    {
	BufferString ip;
	if ( par.get(sKey::IPAddress(),ip) && !ip.isEmpty() )
	    hostaddrfld_->setText( ip );
    }
    else
    {
	BufferString hnm;
	if ( par.get(sKey::Hostname(),hnm) && !hnm.isEmpty() )
	    hostnmfld_->setText( hnm );
    }

    modeChgCB( nullptr );
    lookupCB( nullptr );
    return true;
}
