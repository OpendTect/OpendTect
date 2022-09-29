/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisteeringsel.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetman.h"
#include "attribdescsetsholder.h"
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

namespace Attrib
{

static IOPar& uiSteerSelinpSelHist( bool is2d )
{
    static PtrMan<IOPar> ret;
    DescSetMan* dsman = eDSHolder().getDescSetMan( is2d );
    if ( !dsman && !ret )
	ret = new IOPar( "Fallback Steering selection history" );

    return dsman ? dsman->steerSelHistory() : *ret.ptr();
}

} // namespace Attrib


uiSteeringSel::uiSteeringSel( uiParent* p, const Attrib::DescSet* ads,
			      bool is2d, bool withconstdir, bool doinit )
    : uiGroup(p,"Steering selection")
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
    BufferStringSet steertyps;
    steertyps.add( "None" ).add( "Central" ).add( "Full" );
    if ( withconstdir_ ) steertyps.add ( "Constant direction" );
    typfld_ = new uiGenInput( this, uiStrings::sSteering(),
			      StringListInpSpec(steertyps) );
    typfld_->valuechanged.notify( mCB(this,uiSteeringSel,typeSel));

    inpfld_ = new uiSteerAttrSel( this, descset_, is2d_ );
    inpfld_->getHistory( uiSteerSelinpSelHist(is2d_) );
    inpfld_->attach( alignedBelow, typfld_ );

    dirfld_ = new uiGenInput( this, tr("Azimuth (Inline-based)"),
			      FloatInpSpec() );
    dirfld_->attach( alignedBelow, typfld_ );
    uiString dipstr = tr("Apparent dip %1").arg(SI().zIsTime()
				? tr("(us/m)") : tr("(degrees)"));
    dipfld_ = new uiGenInput( this, dipstr, FloatInpSpec() );
    dipfld_->attach( alignedBelow, dirfld_ );

    setHAlignObj( inpfld_ );
    preFinalize().notify( mCB(this,uiSteeringSel,doFinalize) );
}


void uiSteeringSel::doFinalize(CallBacker*)
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
	inpfld_->getHistory( uiSteerSelinpSelHist(is2d_) );
    }
    else if ( attribname == "FullSteering" )
    {
	type = 2;
	inpfld_->setDesc( ad->getInput(0) );
	inpfld_->getHistory( uiSteerSelinpSelHist(is2d_) );
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
	const StringView attribnm = "ConstantSteering";
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

	Desc* desc = PF().createDescCopy( attribnm );
	if ( !desc ) return DescID::undef();
	desc->getValParam("dip")->setValue( dipfld_->getFValue() );
	desc->getValParam("azi")->setValue( dirfld_->getFValue() );

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
	uiMSG().error(tr("Selected Steering input is not valid"));
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


uiSteerAttrSel::uiSteerAttrSel( uiParent* p, const DescSet* ads,
				bool is2d, const uiString& txt )
	: uiSteerCubeSel(p,is2d,true,txt)
	, attrdata_( is2d )
{
    attrdata_.setAttrSet( ads );
}


uiSteerAttrSel::~uiSteerAttrSel()
{}


DescID uiSteerAttrSel::getDipID( int dipnr ) const
{
    const DescSet& ads = attrdata_.attrSet();
    if ( !workctio_.ioobj_ )
	return DescID::undef();

    const MultiID key = workctio_.ioobj_->key();
    for ( int idx=0; idx<ads.size(); idx++ )
    {
	const DescID descid = ads.getID( idx );
	const Desc* desc = ads.getDesc( descid );
	if ( !desc->isStored() || desc->selectedOutput()!=dipnr )
	    continue;

	const BufferString keystr = desc->getStoredID( false );
	if ( keystr == key.toString() )
	    return descid;
    }

    Desc* desc = PF().createDescCopy( StorageProvider::attribName() );
    desc->setHidden( true );
    desc->selectOutput( dipnr );
    ValParam* keypar = desc->getValParam( StorageProvider::keyStr() );
    keypar->setValue( key );

    BufferString userref = workctio_.ioobj_->name();
    userref += dipnr==0 ? "_inline_dip" : "_crline_dip";
    desc->setUserRef( userref );
    desc->updateParams();

    return const_cast<DescSet&>(ads).addDesc( desc );
}


void uiSteerAttrSel::setDescSet( const DescSet* ads )
{
    attrdata_.setAttrSet( ads );
}


void uiSteerAttrSel::setDesc( const Desc* desc )
{
    if ( !desc || desc->selectedOutput() )
	return;

    if ( !desc->isStored() || desc->dataType() != Seis::Dip )
	return;

    setDescSet( desc->descSet() );

    const ValParam* keypar = desc->getValParam( StorageProvider::keyStr() );
    const StringPair keystr( keypar->getStringValue() );
    MultiID mid;
    mid.fromString( keystr.first() );
    PtrMan<IOObj> ioob = IOM().get( mid );
    workctio_.setObj( ioob ? ioob->clone() : 0 );
    updateInput();
}


void uiSteerAttrSel::fillSelSpec( SelSpec& as, bool inldip )
{
    as.set( 0, inldip ? inlDipID() : crlDipID(), false, "" );
    as.setRefFromID( attrdata_.attrSet() );
    as.set2DFlag( is2D() );
}
