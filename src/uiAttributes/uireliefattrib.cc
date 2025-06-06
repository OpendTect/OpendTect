/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uireliefattrib.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "energyattrib.h"
#include "hilbertattrib.h"
#include "reliefattrib.h"
#include "instantattrib.h"
#include "od_helpids.h"

#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"

using namespace Attrib;

mInitAttribUI(uiReliefAttrib,Relief,"Pseudo Relief",
	      sKeyBasicGrp())

uiReliefAttrib::uiReliefAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d,mODHelpKey(mReliefAttribHelpID))

{
    inpfld_ = createInpFld( is2d );

    gatefld_ = new uiGenInput( this, gateLabel(),
			FloatInpIntervalSpec().setName("Z start",0)
					      .setName("Z stop",1) );
    gatefld_->attach( alignedBelow, inpfld_ );

    setHAlignObj( inpfld_ );
}


uiReliefAttrib::~uiReliefAttrib()
{}


bool uiReliefAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != Relief::attribName() )
	return false;

    ConstRefMan<Desc> instdesc = desc.getInput( 0 );
    ConstRefMan<Desc> energydesc;
    if ( instdesc )
	energydesc = instdesc->getInput( 0 );

    mDynamicCastGet(const ZGateParam*,par,
	energydesc ? energydesc->getValParam( Energy::gateStr() ) : 0)
    Interval<float> zgate( 0, 0 );
    if ( par )
	zgate = par->getValue();

    gatefld_->setValue( zgate );
    return true;
}


bool uiReliefAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiReliefAttrib::setOutput( const Desc& )
{ return true; }


bool uiReliefAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName() != Relief::attribName() )
	return false;
    return true;
}


static DescID hasDesc( const char* attrnm, const char* usrref,
		       const DescSet& ds, const DescID& inpid,
		       const DescID& inpid2=DescID::undef() )
{
    for ( int idx=0; idx<ds.size(); idx++ )
    {
	ConstRefMan<Desc> desc = ds.desc( idx );
	if ( !desc )
	    continue;

	const StringView usrrefstr = desc->userRef();
	if ( desc->attribName()!=attrnm || usrrefstr!=usrref )
	    continue;

	ConstRefMan<Desc> inputdesc = desc->getInput( 0 );
	if ( !inputdesc || inputdesc->id()!=inpid )
	    continue;

	if ( inpid2.isValid() )
	{
	    ConstRefMan<Desc> inputdesc2 = desc->getInput( 1 );
	    if ( !inputdesc2 || inputdesc2->id()!=inpid2 )
		continue;
	}

	return desc->id();
    }

    return DescID::undef();
}


static BufferString createUserRef( const char* inpnm, const char* attrnm )
{
    BufferString usrref = "_";
    usrref.add( inpnm ).add( "_" ).add( attrnm ).add( "_" );
    return usrref;
}


static DescID addEnergyAttrib( DescSet& ds, const DescID& inpid,
			       const Interval<float>& zgate, const char* ref )
{
    ConstRefMan<Desc> inpdesc = ds.getDesc( inpid );
    if ( !inpdesc )
	return DescID::undef();

    const char* attribnm = Energy::attribName();
    const BufferString usrref = createUserRef( ref, attribnm );
    DescID descid = hasDesc( attribnm, usrref, ds, inpid );

    bool newdesc = false;
    RefMan<Desc> energydesc;
    if ( descid.isValid() )
	energydesc = ds.getDesc( descid );
    else
    {
	energydesc = PF().createDescCopy( attribnm );
	if ( !energydesc )
	    return DescID::undef();

	newdesc = true;
    }

    energydesc->setInput( 0, inpdesc.ptr() );
    energydesc->selectOutput( 0 );
    energydesc->setHidden( true );

    mDynamicCastGet(ZGateParam*,param,energydesc->getParam(Energy::gateStr()))
    if ( param )
	param->setValue( zgate );

    energydesc->setUserRef( usrref );
    if ( newdesc )
	descid = ds.addDesc( energydesc.ptr() );

    return descid;
}


static DescID addHilbertAttrib( DescSet& ds, const DescID& inpid )
{
    ConstRefMan<Desc> inpdesc = ds.getDesc( inpid );
    if ( !inpdesc )
	return DescID::undef();

    const char* attribnm = Hilbert::attribName();
    const BufferString usrref = createUserRef( inpdesc->userRef(), attribnm );
    const DescID descid = hasDesc( attribnm, usrref, ds, inpid );
    if ( descid.isValid() )
	return descid;

    RefMan<Desc> newdesc = PF().createDescCopy( attribnm );
    if ( !newdesc )
	return DescID::undef();

    newdesc->setInput( 0, inpdesc.ptr() );
    newdesc->selectOutput( 0 );
    newdesc->setHidden( true );
    newdesc->setUserRef( usrref );
    return ds.addDesc( newdesc.ptr() );
}


static DescID addInstantaneousAttrib( DescSet& ds, const DescID& realid,
				      const DescID& imagid )
{
    ConstRefMan<Desc> realdesc = ds.getDesc( realid );
    ConstRefMan<Desc> imagdesc = ds.getDesc( imagid );
    if ( !realdesc || !imagdesc )
	return DescID::undef();

    const char* attribnm = Instantaneous::attribName();
    const BufferString usrref = createUserRef( realdesc->userRef(), attribnm );
    const DescID descid = hasDesc( attribnm, usrref, ds, realid, imagid );
    if ( descid.isValid() )
	return descid;

    RefMan<Desc> newdesc = PF().createDescCopy( attribnm );
    if ( !newdesc )
	return DescID::undef();

    newdesc->setInput( 0, realdesc.ptr() );
    newdesc->setInput( 1, imagdesc.ptr() );
    newdesc->selectOutput( 13 ); // Phase rotation
    newdesc->setHidden( true );
    mDynamicCastGet(FloatParam*,param,
		    newdesc->getParam(Instantaneous::rotateAngle()))
    if ( param )
	param->setValue( -90.f );

    newdesc->setUserRef( usrref );
    return ds.addDesc( newdesc.ptr() );
}


bool uiReliefAttrib::getInput( Desc& desc )
{
    DescSet* ds = desc.descSet();
    if ( !ds ) return false;

    const DescID inpid = inpfld_->attribID();

    const Interval<float> zgate = gatefld_->getFInterval();
    const DescID enid = addEnergyAttrib( *ds, inpid, zgate, desc.userRef() );
    const DescID hilbid = addHilbertAttrib( *ds, enid );
    const DescID instid = addInstantaneousAttrib( *ds, enid, hilbid );

    return desc.setInput( 0, ds->getDesc(instid).ptr() );
}


bool uiReliefAttrib::getOutput( Desc& desc )
{
    fillOutput( desc, 0 );
    return true;
}
