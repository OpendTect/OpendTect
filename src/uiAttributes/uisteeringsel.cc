/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: uisteeringsel.cc,v 1.5 2005-08-08 15:09:12 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uisteeringsel.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "survinfo.h"
#include "seistrctr.h"
#include "uigeninput.h"
#include "uimsg.h"


using namespace Attrib;

IOPar& uiSteeringSel::inpselhist = *new IOPar( "Steering selection history" );

uiSteeringSel::uiSteeringSel( uiParent* p, const Attrib::DescSet* ads )
    : uiGroup(p,"Steering selection")
    , ctio_(*mMkCtxtIOObj(SeisTrc))
    , descset_(ads)
    , typfld_(0)
    , inpfld_(0)
    , dirfld_(0)
    , dipfld_(0)
    , is2d_(false)
{
    static const char* steertyps[] = { "None", "Central", "Full",
				       "Constant direction", 0 };
//  if ( AF().forms(true).size() < 1 )
//  {
//	nosteerlbl = new uiLabel( this, "<Steering unavailable>" );
//	setHAlignObj( nosteerlbl );
//	return;
//  }

    typfld_ = new uiGenInput( this, "Steering", StringListInpSpec(steertyps) );
    typfld_->valuechanged.notify( mCB(this,uiSteeringSel,typeSel));

    ctio_.ctxt.forread = true;
    inpfld_ = new uiSteerCubeSel( this, ctio_, ads );
    inpfld_->getHistory( inpselhist );
    inpfld_->attach( alignedBelow, typfld_ );

    dirfld_ = new uiGenInput( this, "Azimuth", FloatInpSpec() );
    dirfld_->attach( alignedBelow, typfld_ );
    BufferString dipstr( "Apparent dip " );
    dipstr += SI().zIsTime() ? "(us/m)" : "(degrees)";
    dipfld_ = new uiGenInput( this, dipstr, FloatInpSpec() );
    dipfld_->attach( alignedBelow, dirfld_ );

    setHAlignObj( inpfld_ );
    mainObject()->finaliseStart.notify( mCB(this,uiSteeringSel,doFinalise) );
}


uiSteeringSel::~uiSteeringSel()
{
    delete ctio_.ioobj; delete &ctio_;
}


void uiSteeringSel::doFinalise(CallBacker*)
{
    typeSel(0);
}


void uiSteeringSel::typeSel( CallBacker* )
{
    if ( !typfld_ ) return; 

    int typ = typfld_->getIntValue();
    inpfld_->display( typ > 0 && typ < 3 );
    dirfld_->display( typ == 3 && !is2d_ );
    dipfld_->display( typ == 3 );
}


bool uiSteeringSel::willSteer() const
{
    return typfld_ && typfld_->getIntValue() > 0;
}


void uiSteeringSel::setDesc( const Attrib::Desc* ad )
{
    if ( !typfld_ || !ad )
    {
	typfld_->setValue( 0 );
	return;
    }

    setDescSet( ad->descSet() );

    int type = 0;
    BufferString attribname = ad->attribName();
    if ( attribname == "ConstantSteering" )
    {
	type = 3;
	const Attrib::Desc& desc = *ad;
	float dip, azi;
	mGetFloat( dip, "dip" );
	mGetFloat( azi, "azi" );
	dirfld_->setValue( azi );
	dipfld_->setValue( dip );
    }
    else
    {
	type = attribname == "CentralSteering" ? 1 : 2;
	inpfld_->setDesc( ad->getInput(0) );
	inpfld_->updateHistory( inpselhist );
    }

    typfld_->setValue( type );

    typeSel( 0 );
}


void uiSteeringSel::setDescSet( const DescSet* ads )
{
    descset_ = ads;
    inpfld_->setDescSet( ads );
}


DescID uiSteeringSel::descID()
{
    if ( !typfld_ ) return DescID::undef();

    const int type = typfld_->getIntValue();
    if ( type==0 ) return DescID::undef();

    if ( type==3 )
    {
	for ( int idx=0; idx<descset_->nrDescs(); idx++ )
	{
	    const DescID descid = descset_->getID( idx );
	    const Desc& desc = *descset_->getDesc( descid );
	    if ( strcmp("ConstantSteering",desc.attribName()) )
		continue;

	    float dip, azi;
	    mGetFloat( dip, "dip" );
	    mGetFloat( azi, "azi" );
	    if ( mIsEqual(dip,dipfld_->getValue(),mDefEps) &&
		 mIsEqual(dip,dipfld_->getValue(),mDefEps) )
		return descid;
	}

	return DescID::undef();
    }

    inpfld_->processInput();
    const DescID inldipid = inpfld_->inlDipID();
    const DescID crldipid = inpfld_->crlDipID();
    if ( inldipid < 0 && crldipid < 0 )
    {
	uiMSG().error( "Selected Steering input is not valid" );
	return DescID::undef();
    }

    BufferString attribnm( type==1 ? "CentralSteering" : "FullSteering" );
    for ( int idx=0; idx<descset_->nrDescs(); idx++ )
    {
	const DescID descid = descset_->getID( idx );
	const Desc* desc = descset_->getDesc( descid );
	if ( attribnm != desc->attribName() ) continue;

	if ( desc->getInput(0) && desc->getInput(0)->id() == inldipid &&
	     desc->getInput(1) && desc->getInput(1)->id() == crldipid )
	    return descid;
    }

    Desc* desc = PF().createDescCopy( attribnm );
    if ( !desc ) return DescID::undef();

    DescSet* ads = const_cast<DescSet*>(descset_);
    desc->setInput( 0, ads->getDesc(inldipid) );
    desc->setInput( 1, ads->getDesc(crldipid) );
    desc->setHidden( true );
    desc->setSteering( true );

    mDynamicCastGet(const ValParam*,param,
	ads->getDesc(inldipid)->getParam(StorageProvider::keyStr()))
    BufferString userref = attribnm; userref += " ";
    userref += param->getStringValue();
    desc->setUserRef( userref );

    return ads->addDesc( desc );
}


void uiSteeringSel::setType( int nr, bool fixed )
{
    if ( !typfld_ ) return;
    typfld_->setValue( nr );
    typfld_->setSensitive( !fixed );
    typeSel(0);
}


void uiSteeringSel::set2D( bool yn )
{
    if ( !inpfld_ ) return;

    is2d_ = yn;
    inpfld_->set2DPol( is2d_ ? Only2D : No2D );
    typeSel(0);
}




static const char* steer_seltxts[] = { "Steering Data", 0 };


uiSteerCubeSel::uiSteerCubeSel( uiParent* p, CtxtIOObj& c, 
				const DescSet* ads, const char* txt ) 
	: uiSeisSel(p,getCtio(c),SeisSelSetup().selattr(false),false,
			steer_seltxts)
	, attrdata( ads )
{
}


CtxtIOObj& uiSteerCubeSel::getCtio( CtxtIOObj& c )
{
    c.ctxt.parconstraints.set( sKey::Type, sKey::Steering );
    c.ctxt.includeconstraints = true;
    c.ctxt.allowcnstrsabsent = false;
    c.ctxt.forread = true;
    return c;
}


void uiSteerCubeSel::updateAttrSet2D()
{
    set2DPol( !attrdata.attrset || !attrdata.attrset->nrDescs() ? Both2DAnd3D
	    : (attrdata.attrset->is2D() ? Only2D : No2D) );
}


DescID uiSteerCubeSel::getDipID( int dipnr ) const
{
    const DescSet* ads = attrdata.attrset;
    if ( !ads )
    {
	pErrMsg( "No attribdescset set");
	return DescID::undef();
    }

    if ( !ctio.ioobj ) 
	return DescID::undef();

    LineKey linekey( ctio.ioobj->key() );
    if ( is2D() ) linekey.setAttrName( sKey::Steering );

    for ( int idx=0; idx<ads->nrDescs(); idx++ )
    {
	const DescID descid = ads->getID( idx );
	const Desc* desc = ads->getDesc( descid );
	if ( !desc->isStored() || desc->selectedOutput()!=dipnr )
	    continue;

	const Param* keypar = desc->getParam( StorageProvider::keyStr() );
	BufferString res; keypar->getCompositeValue( res );
	if ( res == linekey )
	    return descid;
    }

    Desc* desc = PF().createDescCopy( StorageProvider::attribName() );
    desc->setHidden( true );
    desc->selectOutput( dipnr );
    mDynamicCastGet(ValParam*,keypar,desc->getParam(StorageProvider::keyStr()))
    keypar->setValue( linekey );

    BufferString userref = ctio.ioobj->name();
    userref += dipnr==0 ? "_inline_dip" : "_crline_dip";
    desc->setUserRef( userref );
    desc->updateParams();

    return const_cast<DescSet*>(ads)->addDesc( desc );
}


void uiSteerCubeSel::setDescSet( const DescSet* ads )
{
    attrdata.attrset = ads;
    updateAttrSet2D();
}


void uiSteerCubeSel::setDesc( const Desc* desc )
{
    if ( !desc || desc->selectedOutput() ) return;

    if ( !desc->isStored() ) return;

    const ValParam* keypar = 
		(ValParam*)desc->getParam( StorageProvider::keyStr() );
    const MultiID mid( keypar->getStringValue() );
    PtrMan<IOObj> ioobj = IOM().get( mid );
    ctio.setObj( ioobj ? ioobj->clone() : 0 );
    updateInput();
}


void uiSteerCubeSel::fillSelSpec( SelSpec& as, bool inldip )
{
    as.set( 0, inldip ? inlDipID() : crlDipID(), false, "" );
    as.setRefFromID( *attrdata.attrset );
}
