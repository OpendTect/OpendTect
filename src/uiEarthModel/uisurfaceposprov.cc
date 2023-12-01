/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisurfaceposprov.h"
#include "emsurfaceposprov.h"

#include "ctxtioobj.h"
#include "emsurfacetr.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"

#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiiosurface.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiselsurvranges.h"
#include "uispinbox.h"
#include "uistrings.h"


uiSurfacePosProvGroup::uiSurfacePosProvGroup( uiParent* p,
					const uiPosProvGroup::Setup& su )
    : uiPosProvGroup(p,su)
    , zfac_(sCast(float,SI().zDomain().userFactor()))
    , surf1fld_(nullptr)
    , surf2fld_(nullptr)
    , zstepfld_(nullptr)
    , extrazfld_(nullptr)
    , samplingfld_(nullptr)
    , nrsamplesfld_(nullptr)
{
    if ( su.is2d_ )
    {
	new uiLabel( this, tr("Not implemented for 2D") );
	return;
    }

    surf1fld_ = new uiHorizon3DSel( this, true, uiStrings::sHorizon() );

    const CallBack selcb( mCB(this,uiSurfacePosProvGroup,selChg) );
    issingfld_ = new uiGenInput( this, uiStrings::sSelect(),
			BoolInpSpec(true,tr("On Horizon"),
				    tr("To a 2nd Horizon")) );
    issingfld_->attach( alignedBelow, surf1fld_ );
    issingfld_->valueChanged.notify( selcb );

    surf2fld_ = new uiHorizon3DSel( this, true, uiStrings::sBottomHor() );
    surf2fld_->attach( alignedBelow, issingfld_ );

    uiString txt;
    if ( su.withstep_ )
    {
	txt = tr("Z step %1").arg(SI().getUiZUnitString());
	zstepfld_ = new uiLabeledSpinBox( this, txt, 0, "Z step" );
	zstepfld_->attach( alignedBelow, surf2fld_ );
	float zstep = SI().zRange(true).step * 10;
	int v = (int)((zstep * zfac_) + .5);
	zstepfld_->box()->setValue( v );
	zstepfld_->box()->setInterval( StepInterval<int>(1,999999,1) );
    }

    if ( su.withz_ )
    {
	extrazfld_ = new uiSelZRange( this, false, true, "Extra Z" );
	if ( zstepfld_ )
	    extrazfld_->attach( alignedBelow, zstepfld_ );
	else
	    extrazfld_->attach( alignedBelow, surf2fld_ );
    }

    if ( su.withrandom_ )
    {
	samplingfld_ = new uiGenInput( this, tr("Sampling Mode"),
				BoolInpSpec(true,tr("Random"),tr("Regular")) );
	mAttachCB( samplingfld_->valueChanged,
		   uiSurfacePosProvGroup::samplingCB );
	if ( extrazfld_ )
	    samplingfld_->attach( alignedBelow, extrazfld_ );
	else if ( zstepfld_ )
	    samplingfld_->attach( alignedBelow, zstepfld_ );
	else
	    samplingfld_->attach( alignedBelow, surf2fld_ );

	nrsamplesfld_ = new uiGenInput( this, tr("Number of samples"),
					IntInpSpec(4000) );
	nrsamplesfld_->attach( rightOf, samplingfld_ );
    }

    setHAlignObj( surf1fld_ );
    postFinalize().notify( selcb );
}


uiSurfacePosProvGroup::~uiSurfacePosProvGroup()
{
    detachAllNotifiers();
}


bool uiSurfacePosProvGroup::hasRandomSampling() const
{
    return samplingfld_ ? samplingfld_->getBoolValue() : false;
}


void uiSurfacePosProvGroup::selChg( CallBacker* )
{
    const bool isbtwn = !issingfld_->getBoolValue();
    surf2fld_->display( isbtwn );
    samplingCB( nullptr );
}


#define mErrRet(s) { uiMSG().error(s); return false; }
#define mGetSurfKey(k) \
    IOPar::compKey(sKey::Surface(),Pos::EMSurfaceProvider3D::k##Key())


void uiSurfacePosProvGroup::usePar( const IOPar& iop )
{
    if ( !surf1fld_ )
	return;

    MultiID mids1;
    iop.get( mGetSurfKey(id1), mids1 );
    if( mids1.isUdf() )
	return;

    surf1fld_->setInput( mids1 );

    MultiID mids2;
    iop.get ( mGetSurfKey(id2), mids2 );
    const bool issing = mids2.isUdf();
    if ( !issing )
	surf2fld_->setInput( mids2 );

    issingfld_->setValue( issing );

    if ( zstepfld_ )
    {
	float zstep = zstepfld_->box()->getFValue() / zfac_;
	iop.get( mGetSurfKey(zstep), zstep );
	int v = (int)((zstep * zfac_) + .5);
	zstepfld_->box()->setValue( v );
    }

    if ( extrazfld_ )
    {
	StepInterval<float> ez = extrazfld_->getRange();
	iop.get( mGetSurfKey(extraZ), ez );
	extrazfld_->setRange( ez );
    }

    if	( samplingfld_ )
    {
	bool random = false;
	iop.getYN( sKey::Random(), random );
	samplingfld_->setValue( random );

	int nrsamples = 1000;
	if ( random )
	    iop.get( sKey::NrValues(), nrsamples );
	nrsamplesfld_->setValue( nrsamples );

	samplingCB( nullptr );
    }

    selChg( nullptr );
}


bool uiSurfacePosProvGroup::fillPar( IOPar& iop ) const
{
    const IOObj* ioobj1 = surf1fld_ ? surf1fld_->ioobj() : nullptr;
    if ( !ioobj1 )
	return false;

    iop.set( mGetSurfKey(id1), ioobj1->key() );

    Interval<float> ez( 0, 0 );
    if ( issingfld_->getBoolValue() )
	iop.removeWithKey( mGetSurfKey(id2) );
    else
    {
	const IOObj* ioobj2 = surf2fld_->ioobj();
	if ( !ioobj2 )
	    return false;

	if ( ioobj1->key() == ioobj2->key() )
	    mErrRet(tr("Please select two different horizons"))

	iop.set( mGetSurfKey(id2), ioobj2->key() );
    }

    const float zstep = zstepfld_ ? zstepfld_->box()->getFValue() / zfac_
				  : SI().zStep();
    iop.set( mGetSurfKey(zstep), zstep );

    if ( mIsUdf(ez.start) ) ez.start = 0;
    if ( mIsUdf(ez.stop) ) ez.stop = 0;

    if ( extrazfld_ ) assign( ez, extrazfld_->getRange() );
    iop.set( mGetSurfKey(extraZ), ez );
    iop.set( sKey::Type(), sKey::Surface() );

    if ( samplingfld_ )
    {
	iop.setYN( sKey::Random(), samplingfld_->getBoolValue() );
	if ( samplingfld_->getBoolValue() )
	    iop.set( sKey::NrValues(), nrsamplesfld_->getIntValue() );
    }

    return true;
}


void uiSurfacePosProvGroup::getSummary( BufferString& txt ) const
{
    if ( !surf1fld_ ) return;
    txt += issingfld_->getBoolValue() ? "On Horizon" : "Between Horizons";
}


void uiSurfacePosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Surface(),
					  uiStrings::sHorizon() );
}


void uiSurfacePosProvGroup::samplingCB( CallBacker* )
{
    if ( samplingfld_ )
    {
	bool showstep = !samplingfld_->getBoolValue();
	if ( zstepfld_ )
	    zstepfld_->display( showstep );
	if ( nrsamplesfld_ )
	    nrsamplesfld_->display( !showstep );
	posProvGroupChg.trigger();
    }
}
