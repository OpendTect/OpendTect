/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisteeringsel.cc,v 1.56 2012/01/11 08:20:25 cvshelene Exp $";


#include "uisteeringsel.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "paramsetget.h"
#include "survinfo.h"
#include "keystrs.h"
#include "seisselection.h"
#include "seiscbvs.h"
#include "seisioobjinfo.h"
#include "uiattribfactory.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"


using namespace Attrib;

IOPar& uiSteeringSel::inpselhist = *new IOPar( "Steering selection history" );

uiSteeringSel::uiSteeringSel( uiParent* p, const Attrib::DescSet* ads, 
			      bool is2d, bool withconstdir, bool doinit )
    : uiGroup(p,"Steering selection")
    , ctio_( *uiSteerCubeSel::mkCtxtIOObj(is2d,true) )
    , descset_(ads)
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

    const char* res = uiAF().attrNameOf( "Curvature" );
    if ( !res )
    {
	nosteerlbl_ = new uiLabel( this, "<Steering unavailable>" );
	setHAlignObj( nosteerlbl_ );
	return;
    }

    createFields();
}


uiSteeringSel::~uiSteeringSel()
{
    delete ctio_.ioobj; delete &ctio_;
}


void uiSteeringSel::createFields()
{
    BufferStringSet steertyps;
    steertyps.add( "None" ).add( "Central" ).add( "Full" );
    if ( withconstdir_ ) steertyps.add ( "Constant direction" );
    typfld_ = new uiGenInput( this, "Steering", StringListInpSpec(steertyps) );
    typfld_->valuechanged.notify( mCB(this,uiSteeringSel,typeSel));

    ctio_.ctxt.forread = true;
    inpfld_ = new uiSteerCubeSel( this, ctio_, descset_, is2d_ );
    inpfld_->getHistory( inpselhist );
    inpfld_->attach( alignedBelow, typfld_ );

    dirfld_ = new uiGenInput( this, "Azimuth", FloatInpSpec() );
    dirfld_->attach( alignedBelow, typfld_ );
    BufferString dipstr( "Apparent dip " );
    dipstr += SI().zIsTime() ? "(us/m)" : "(degrees)";
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
    if ( !inpfld_ ) return; 

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


void uiSteeringSel::setDesc( const Attrib::Desc* ad )
{
    if ( !typfld_ || !inpfld_ )
	return;
    
    if ( !ad )
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
	mIfGetFloat( "dip", dip, dipfld_->setValue( dip ) );
	mIfGetFloat( "azi", azi, dirfld_->setValue( azi ) );
    }
    else if ( attribname == "CentralSteering" )
    {
	type = 1;
	inpfld_->setDesc( ad->getInput(0) );
	inpfld_->updateHistory( inpselhist );
    }
    else if ( attribname == "FullSteering" )
    {
	type = 2;
	inpfld_->setDesc( ad->getInput(0) );
	inpfld_->updateHistory( inpselhist );
    }

    if ( !notypechange_ )
	typfld_->setValue( type );
    typeSel( 0 );
}


void uiSteeringSel::setDescSet( const DescSet* ads )
{
    if ( !inpfld_ ) return;

    descset_ = ads;
    inpfld_->setDescSet( ads );
    if ( !notypechange_ )
    {
	typfld_->setValue( 0 );
	typeSel( 0 );
    }
}


DescID uiSteeringSel::descID()
{
    if ( !typfld_ ) return DescID::undef();

    const int type = typfld_->getIntValue();
    if ( !willSteer() ) return DescID::undef();

    if ( type==3 )
    {
	const char* attribnm = "ConstantSteering";
	for ( int idx=0; idx<descset_->size(); idx++ )
	{
	    const DescID descid = descset_->getID( idx );
	    const Desc& desc = *descset_->getDesc( descid );
	    if ( strcmp(attribnm,desc.attribName()) )
		continue;

	    float dip, azi;
	    mGetFloatFromDesc( desc, dip, "dip" );
	    mGetFloatFromDesc( desc, azi, "azi" );
	    if ( mIsEqual(dip,dipfld_->getfValue(),mDefEps) &&
		 mIsEqual(azi,dirfld_->getfValue(),mDefEps) )
		return descid;
	}

	Desc* desc = PF().createDescCopy( attribnm );
	if ( !desc ) return DescID::undef();
	desc->getValParam("dip")->setValue( dipfld_->getfValue() );
	desc->getValParam("azi")->setValue( dirfld_->getfValue() );

	DescSet* ads = const_cast<DescSet*>(descset_);
	BufferString userref = attribnm;
	desc->setUserRef( userref );
	desc->setHidden(true);
	return ads->addDesc( desc );
    }

    inpfld_->processInput();
    const DescID inldipid = inpfld_->inlDipID();
    const DescID crldipid = inpfld_->crlDipID();
    if ( !inldipid.isValid() || !crldipid.isValid() )
    {
	uiMSG().error( "Selected Steering input is not valid" );
	return DescID::undef();
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

    Desc* desc = PF().createDescCopy( attribnm );
    if ( !desc ) return DescID::undef();

    DescSet* ads = const_cast<DescSet*>(descset_);
    desc->setInput( 0, ads->getDesc(inldipid) );
    desc->setInput( 1, ads->getDesc(crldipid) );
    desc->setHidden( true );
    desc->setSteering( true );

    const ValParam* param = 
	ads->getDesc(inldipid)->getValParam( StorageProvider::keyStr() );
    BufferString userref = attribnm; userref += " ";
    userref += param->getStringValue();
    desc->setUserRef( userref );
    BufferString defstr;
    desc->getDefStr(defstr);
    desc->parseDefStr(defstr);

    return ads->addDesc( desc );
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


static uiSeisSel::Setup mkSeisSelSetup( bool is2d, const char* txt )
{
    uiSeisSel::Setup sssu( is2d, false );
    sssu.selattr( is2d ).wantSteering().seltxt( txt );
    return sssu;
}


uiSteerCubeSel::uiSteerCubeSel( uiParent* p, CtxtIOObj& c,
				const DescSet* ads, bool is2d, const char* txt )
	: uiSeisSel( p, c, mkSeisSelSetup(is2d,txt) )
	, attrdata_( is2d )
{
    attrdata_.setAttrSet( ads );
    preFinalise().notify( mCB(this,uiSteerCubeSel,doFinalise) );
}


void uiSteerCubeSel::doFinalise( CallBacker* c )
{
    if ( workctio_.ioobj ) return;

    const MultiID& defid = SeisIOObjInfo::getDefault( sKey::Steering );
    workctio_.setObj( IOM().get(defid) );
    if ( workctio_.ioobj )
	updateInput();
}


const IOObjContext& uiSteerCubeSel::ioContext( bool is2d )
{
    static PtrMan<IOObjContext> ctxt = 0;
    if ( !ctxt )
    {
	ctxt = new IOObjContext( SeisTrcTranslatorGroup::ioContext() );
	ctxt->deftransl = CBVSSeisTrcTranslator::translKey();
	if ( !is2d )
	    ctxt->toselect.require_.set( sKey::Type, sKey::Steering );
    }

    return *ctxt;
}


CtxtIOObj* uiSteerCubeSel::mkCtxtIOObj( bool is2d, bool forread )
{
    CtxtIOObj* ret = mMkCtxtIOObj(SeisTrc);
    ret->ctxt = ioContext( is2d );
    ret->ctxt.forread = forread;
    if ( is2d )
	ret->ctxt.deftransl = "2D";
    else
	ret->ctxt.toselect.require_.set( sKey::Type, sKey::Steering );

    if ( forread && !is2d )
    {
	const char* defcubeid = SI().pars().find( "Default.Cube.Steering" );
	if ( defcubeid && *defcubeid )
	    ret->setObj( MultiID(defcubeid) );
			     
    }
    return ret;
}


DescID uiSteerCubeSel::getDipID( int dipnr ) const
{
    const DescSet& ads = attrdata_.attrSet();
    if ( !workctio_.ioobj ) 
	return DescID::undef();

    LineKey linekey( workctio_.ioobj->key() );
    if ( is2D() ) linekey.setAttrName( attrNm() );

    for ( int idx=0; idx<ads.size(); idx++ )
    {
	const DescID descid = ads.getID( idx );
	const Desc* desc = ads.getDesc( descid );
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
    ValParam* keypar = desc->getValParam( StorageProvider::keyStr() );
    keypar->setValue( linekey );

    BufferString userref = workctio_.ioobj->name();
    userref += dipnr==0 ? "_inline_dip" : "_crline_dip";
    desc->setUserRef( userref );
    desc->updateParams();

    return const_cast<DescSet&>(ads).addDesc( desc );
}


void uiSteerCubeSel::setDescSet( const DescSet* ads )
{
    attrdata_.setAttrSet( ads );
}


void uiSteerCubeSel::setDesc( const Desc* desc )
{
    if ( !desc || desc->selectedOutput() ) return;

    if ( !desc->isStored() || desc->dataType() != Seis::Dip ) return;

    setDescSet( desc->descSet() );

    const ValParam* keypar = desc->getValParam( StorageProvider::keyStr() );
    const LineKey lk( keypar->getStringValue() );
    const MultiID mid( lk.lineName() );
    PtrMan<IOObj> ioob = IOM().get( mid );
    workctio_.setObj( ioob ? ioob->clone() : 0 );
    updateInput();
    if ( is2D() )
	setAttrNm( lk.attrName() );
}


void uiSteerCubeSel::fillSelSpec( SelSpec& as, bool inldip )
{
    as.set( 0, inldip ? inlDipID() : crlDipID(), false, "" );
    as.setRefFromID( attrdata_.attrSet() );
    as.set2DFlag( is2D() );
}
