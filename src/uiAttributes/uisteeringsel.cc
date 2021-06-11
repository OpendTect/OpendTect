/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/


#include "uisteeringsel.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "ioobjctxt.h"
#include "ioobj.h"
#include "iopar.h"
#include "paramsetget.h"
#include "survinfo.h"
#include "seiscbvs.h"
#include "seisioobjinfo.h"
#include "uiattribfactory.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uistrings.h"



IOPar& uiSteeringSel::inpselhist = *new IOPar( "Steering selection history" );

uiSteeringSel::uiSteeringSel( uiParent* p, const DescSet* ads,
			      bool is2d, bool withconstdir, bool doinit )
    : uiGroup(p,"Steering selection")
    , descset_(ads ? ads : &DescSet::global(is2d))
    , typfld_(0)
    , inpfld_(0)
    , dirfld_(0)
    , dipfld_(0)
    , notypechange_(false)
    , is2d_(is2d)
    , withconstdir_(withconstdir)
    , steertypeSelected_(this)
{
    if ( !doinit ) return;

    if ( !uiAF().hasSteering() )
    {
	nosteerlbl_ = new uiLabel( this, tr("<Steering unavailable>") );
	setHAlignObj( nosteerlbl_ );
	return;
    }

    createFields();
}


uiSteeringSel::~uiSteeringSel()
{
}


void uiSteeringSel::createFields()
{
    uiStringSet steertyps;
    steertyps.add( uiStrings::sNone() ).add( uiStrings::sCentral() )
				       .add( uiStrings::sFull() );
    if ( withconstdir_ )
	steertyps.add( tr("Constant direction") );
    typfld_ = new uiGenInput( this, uiStrings::sSteering(),
                              StringListInpSpec(steertyps) );
    typfld_->valuechanged.notify( mCB(this,uiSteeringSel,typeSel));

    inpfld_ = new uiSteerAttrSel( this, *descset_ );
    inpfld_->getHistory( inpselhist );
    inpfld_->attach( alignedBelow, typfld_ );

    dirfld_ = new uiGenInput( this,tr("Azimuth (Inline-based)"),FloatInpSpec());
    dirfld_->attach( alignedBelow, typfld_ );
    uiString dipstr = tr("Apparent dip")
		      .withUnit( SI().zIsTime() ? "us/m" : "deg" );
    dipfld_ = new uiGenInput( this, dipstr, FloatInpSpec() );
    dipfld_->attach( alignedBelow, dirfld_ );

    setHAlignObj( inpfld_ );
    preFinalise().notify( mCB(this,uiSteeringSel,doFinalise) );
}


void uiSteeringSel::doFinalise(CallBacker*)
{
    typeSel(0);
}


void uiSteeringSel::typeSel( CallBacker* )
{
    if ( !inpfld_ )
	return;

    int typ = typfld_->getIntValue();
    inpfld_->display( typ > 0 && typ < 3 );
    dirfld_->display( typ == 3 && !is2d_ );
    dipfld_->display( typ == 3 );

    steertypeSelected_.trigger();
}


bool uiSteeringSel::willSteer() const
{
    return typfld_ && typfld_->getIntValue() > 0;
}


void uiSteeringSel::setDesc( const Desc* ad )
{
    if ( !typfld_ || !inpfld_ )
	return;

    if ( !ad || !ad->descSet() )
	{ typfld_->setValue( 0 ); return; }

    setDescSet( ad->descSet() );

    int type = 0;
    BufferString attribname = ad->attribName();
    if ( attribname == "ConstantSteering" )
    {
	type = 3;
	const Desc& desc = *ad;
	mIfGetFloat( "dip", dip, dipfld_->setValue( dip ) );
	mIfGetFloat( "azi", azi, dirfld_->setValue( azi ) );
    }
    else if ( attribname == "CentralSteering" )
    {
	type = 1;
	inpfld_->setDesc( ad->getInput(0) );
    }
    else if ( attribname == "FullSteering" )
    {
	type = 2;
	inpfld_->setDesc( ad->getInput(0) );
    }

    if ( !notypechange_ )
	typfld_->setValue( type );
    typeSel( 0 );
}


void uiSteeringSel::setDescSet( const DescSet* ads )
{
    if ( !inpfld_ || ads == descset_ )
	return;

    descset_ = ads ? ads : &DescSet::global( descset_->is2D() );
    inpfld_->setDescSet( descset_ );
    if ( !notypechange_ )
    {
	typfld_->setValue( 0 );
	typeSel( 0 );
    }
}


Attrib::DescID uiSteeringSel::descID()
{
    if ( !typfld_ )
	return DescID();
    const int type = typfld_->getIntValue();
    if ( !willSteer() )
	return DescID();

    DescSet& descset4ed = *const_cast<DescSet*>(descset_);

    if ( type==3 )
    {
	const FixedString attribnm = "ConstantSteering";
	for ( int idx=0; idx<descset_->size(); idx++ )
	{
	    const DescID descid = descset_->getID( idx );
	    const Desc& desc = *descset_->getDesc( descid );
	    if ( desc.attribName() != attribnm )
		continue;

	    float dip, azi;
	    mGetFloatFromDesc( desc, dip, "dip" );
	    mGetFloatFromDesc( desc, azi, "azi" );
	    if ( mIsEqual(dip,dipfld_->getFValue(),mDefEps) &&
		 mIsEqual(azi,dirfld_->getFValue(),mDefEps) )
		return descid;
	}

	Desc* desc = Attrib::PF().createDescCopy( attribnm );
	if ( !desc )
	    return DescID();
	desc->getValParam("dip")->setValue( dipfld_->getFValue() );
	desc->getValParam("azi")->setValue( dirfld_->getFValue() );

	BufferString userref = attribnm;
	desc->setUserRef( userref );
	desc->setIsHidden(true);
	return descset4ed.addDesc( desc );
    }

    const DescID inldipid = inpfld_->inlDipID();
    const DescID crldipid = inpfld_->crlDipID();
    if ( !inldipid.isValid() || !crldipid.isValid() )
    {
	uiMSG().error(tr("Selected Steering input is not valid"));
	return DescID();
    }

    BufferString attribnm( type==1 ? "CentralSteering" : "FullSteering" );
    for ( int idx=0; idx<descset_->size(); idx++ )
    {
	const DescID descid = descset_->getID( idx );
	const Desc* desc = descset_->getDesc( descid );
	if ( attribnm != desc->attribName() ) continue;

	if ( desc->getInput(0) && desc->getInput(0)->id() == inldipid &&
	     desc->getInput(1) && desc->getInput(1)->id() == crldipid )
	    return descid;
    }

    Desc* desc = Attrib::PF().createDescCopy( attribnm );
    if ( !desc )
	return DescID();

    desc->setInput( 0, descset4ed.getDesc(inldipid) );
    desc->setInput( 1, descset4ed.getDesc(crldipid) );
    desc->setIsHidden( true );
    desc->setIsSteering( true );

    const Attrib::ValParam* param = descset_->getDesc(inldipid)->getValParam(
					Attrib::StorageProvider::keyStr() );
    BufferString userref = attribnm; userref += " ";
    userref += param->getStringValue();
    desc->setUserRef( userref );
    BufferString defstr;
    desc->getDefStr(defstr);
    desc->parseDefStr(defstr);

    return descset4ed.addDesc( desc );
}


void uiSteeringSel::setType( int nr, bool nochg )
{
    if ( !typfld_ ) return;
    typfld_->setValue( nr );
    typfld_->setSensitive( !nochg );
    notypechange_ = nochg;
    typeSel(0);
}


void uiSteeringSel::clearInpField()
{
    inpfld_->setEmpty();
}


const char* uiSteeringSel::text() const
{
    return inpfld_->getInput();
}


uiSteerAttrSel::uiSteerAttrSel( uiParent* p, const Attrib::DescSet& ads,
				const uiString& txt )
    : uiSteerCubeSel(p,ads.is2D(),true,txt)
    , attrdata_( ads.is2D() )
{
    attrdata_.setAttrSet( ads );
}


uiSteerAttrSel::uiSteerAttrSel( uiParent* p, bool is2d,
				const uiString& txt )
    : uiSteerCubeSel(p,is2d,true,txt)
    , attrdata_( is2d )
{
    attrdata_.setAttrSet( Attrib::DescSet::global(is2d) );
}


Attrib::DescID uiSteerAttrSel::getDipID( int dipnr ) const
{
    const Attrib::DescSet& ads = attrdata_.attrSet();
    const IOObj* inpioobj = ioobj( true );
    if ( !inpioobj )
	return Attrib::DescID();

    const BufferString storkey( inpioobj->key().toString() );
    for ( int idx=0; idx<ads.size(); idx++ )
    {
	const Attrib::DescID descid = ads.getID( idx );
	const Attrib::Desc* desc = ads.getDesc( descid );
	if ( !desc->isStored() || desc->selectedOutput()!=dipnr )
	    continue;

	const Attrib::Param* keypar = desc->getParam(
					Attrib::StorageProvider::keyStr() );
	BufferString res; keypar->getCompositeValue( res );
	if ( res == storkey )
	    return descid;
    }

    Attrib::Desc* desc = Attrib::PF().createDescCopy(
				Attrib::StorageProvider::attribName() );
    desc->setIsHidden( true );
    desc->setIsSteering( true );
    desc->setNrOutputs( Seis::Dip, 2 );
    desc->selectOutput( dipnr );
    Attrib::ValParam* keypar = desc->getValParam(
					Attrib::StorageProvider::keyStr() );
    keypar->setValue( storkey );

    BufferString userref = inpioobj->name();
    userref += dipnr==0 ? "_inline_dip" : "_crline_dip";
    desc->setUserRef( userref );
    desc->updateParams();

    return const_cast<Attrib::DescSet&>(ads).addDesc( desc );
}


void uiSteerAttrSel::setDescSet( const Attrib::DescSet* ads )
{
    const bool is2d = attrdata_.is2D();
    attrdata_.setAttrSet( ads ? *ads : Attrib::DescSet::global(is2d) );
}


void uiSteerAttrSel::setDesc( const Attrib::Desc* desc )
{
    if ( !desc || desc->selectedOutput() || !desc->descSet()
      || !desc->isStored() || desc->dataType() != Seis::Dip )
	return;

    setDescSet( desc->descSet() );

    const Attrib::ValParam* keypar
		= desc->getValParam( Attrib::StorageProvider::keyStr() );
    const StringPair storkey( keypar->getStringValue() );
    workctio_.setObj( DBKey(storkey.first()).getIOObj() );
    updateInput();
}


void uiSteerAttrSel::fillSelSpec( Attrib::SelSpec& as, bool inldip )
{
    as.set( 0, inldip ? inlDipID() : crlDipID(), false, "" );
    as.setRefFromID( attrdata_.attrSet() );
    as.set2D( is2D() );
}
