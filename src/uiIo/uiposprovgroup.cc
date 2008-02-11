/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiposprovgroup.cc,v 1.5 2008-02-11 17:23:05 cvsbert Exp $";

#include "uiposprovgroupstd.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uiioobjsel.h"
#include "iopar.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "survinfo.h"
#include "keystrs.h"
#include "cubesampling.h"
#include "picksettr.h"

mImplFactory2Param(uiPosProvGroup,uiParent*,const uiPosProvider::Setup&,
		   uiPosProvGroup::factory);


static uiGenInput* mkStdZFld( uiParent* p, bool wstep, bool useworksi )
{
    StepInterval<float> zrg = SI().zRange( useworksi );
    zrg.scale( SI().zFactor() );
    uiGenInput* fld = new uiGenInput( p, "Z Range",
	    			      FloatInpIntervalSpec(wstep) );
    fld->setValue( zrg );
    return fld;
}


uiPosProvGroup::uiPosProvGroup( uiParent* p, const uiPosProvider::Setup& su )
    : uiGroup(p,su.seltxt_)
{
}


uiRangePosProvGroup::uiRangePosProvGroup( uiParent* p,
					  const uiPosProvider::Setup& su )
    : uiPosProvGroup(p,su)
    , inlfld_(0)
    , zfld_(0)
{
    mDynamicCastGet(const uiRangePosProvGroup::Setup*,rsu,&su)
    wstep_ = rsu ? rsu->withstep_ : true;
    wsi_ = rsu ? rsu->useworksi_ : true;
    StepInterval<int> inlrg, crlrg;
    SI().sampling(wsi_).hrg.get( inlrg, crlrg );

    if ( !su.is2d_ )
    {
	inlfld_ = new uiGenInput( this, "In-line range",
				  IntInpIntervalSpec(wstep_) );
	inlfld_->setValue( inlrg );
    }
    const char* fldtxt = su.is2d_ ? "Trace number range" : "X-line range";
    crlfld_ = new uiGenInput( this, fldtxt, IntInpIntervalSpec(wstep_) );
    crlfld_->setValue( crlrg );
    if ( inlfld_ )
	crlfld_->attach( alignedBelow, inlfld_ );

    if ( su.withz_ )
    {
	zfld_ = mkStdZFld( this, wstep_, wsi_ );
	zfld_->attach( alignedBelow, crlfld_ );
    }

    setHAlignObj( crlfld_ );
}


void uiRangePosProvGroup::usePar( const IOPar& iop )
{
    CubeSampling cs; getCubeSampling( cs );
    cs.usePar( iop );

    if ( inlfld_ )
    {
	inlfld_->setValue( cs.hrg.start.inl, 0 );
	inlfld_->setValue( cs.hrg.stop.inl, 1 );
	if ( wstep_ ) inlfld_->setValue( cs.hrg.step.inl, 2 );
    }

    crlfld_->setValue( cs.hrg.start.crl, 0 );
    crlfld_->setValue( cs.hrg.stop.crl, 1 );
    if ( wstep_ ) crlfld_->setValue( cs.hrg.step.crl, 2 );

    if ( zfld_ )
    {
	StepInterval<float> zrg( cs.zrg );
	zrg.scale( SI().zFactor() );
	zfld_->setValue( zrg.start, 0 );
	zfld_->setValue( zrg.stop, 1 );
	if ( wstep_ ) zfld_->setValue( zrg.step, 2 );
    }
}


bool uiRangePosProvGroup::fillPar( IOPar& iop ) const
{
    CubeSampling cs; getCubeSampling( cs );
    cs.fillPar( iop );
    return true;
}


void uiRangePosProvGroup::getCubeSampling( CubeSampling& cs ) const
{
    cs = SI().sampling( wsi_ );

    if ( !inlfld_ )
    {
	cs.hrg.start.crl = cs.hrg.step.crl = 1;
	cs.hrg.stop.crl = mUdf(int);
    }
    else
    {
	cs.hrg.start.inl = inlfld_->getIntValue( 0 );
	cs.hrg.stop.inl = inlfld_->getIntValue( 1 );
	if ( wstep_ ) cs.hrg.step.inl = inlfld_->getIntValue( 2 );
    }

    cs.hrg.start.crl = crlfld_->getIntValue( 0 );
    cs.hrg.stop.crl = crlfld_->getIntValue( 1 );
    if ( wstep_ ) cs.hrg.step.crl = crlfld_->getIntValue( 2 );

    if ( zfld_ )
    {
	const float fac = 1. / SI().zFactor();
	cs.zrg.start = zfld_->getfValue( 0 ) * fac;
	cs.zrg.stop = zfld_->getfValue( 1 ) * fac;
	if ( wstep_ )
	    cs.zrg.step = zfld_->getfValue( 2 ) * fac;
	else
	    cs.zrg.step = SI().zRange(wsi_).step;
    }
}


void uiRangePosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Range );
}


uiPolyPosProvGroup::uiPolyPosProvGroup( uiParent* p,
					const uiPosProvider::Setup& su )
    : uiPosProvGroup(p,su)
    , ctio_(*mMkCtxtIOObj(PickSet))
    , zfld_(0)
{
    ctio_.ctxt.parconstraints.set( sKey::Type, sKey::Polygon );
    ctio_.ctxt.allowcnstrsabsent = false;
    ctio_.fillIfOnlyOne( IOObjContext::Loc );
    polyfld_ = new uiIOObjSel( this, ctio_, sKey::Polygon );

    if ( su.withz_ )
    {
	zfld_ = mkStdZFld( this, true, true );
	zfld_->attach( alignedBelow, polyfld_ );
    }

    setHAlignObj( polyfld_ );
}


uiPolyPosProvGroup::~uiPolyPosProvGroup()
{
    delete ctio_.ioobj; delete &ctio_;
}


void uiPolyPosProvGroup::usePar( const IOPar& iop )
{
    polyfld_->usePar( iop );
    if ( zfld_ )
    {
	StepInterval<float> zrg( SI().zRange(true) );
	iop.get( sKey::ZRange, zrg );
	zrg.scale( SI().zFactor() );
	zfld_->setValue( zrg );
    }
}


bool uiPolyPosProvGroup::fillPar( IOPar& iop ) const
{
    if ( !polyfld_->fillPar(iop) )
	return false;

    const float fac = 1. / SI().zFactor();
    StepInterval<float> zrg; zfld_->getFStepInterval();
    zrg.scale( 1. / SI().zFactor() );
    iop.set( sKey::ZRange, zrg );
    return true;
}


void uiPolyPosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Polygon );
}


uiTablePosProvGroup::uiTablePosProvGroup( uiParent* p,
					const uiPosProvider::Setup& su )
    : uiPosProvGroup(p,su)
{
    BufferString lbl( su.is2d_ ? "2D" : "3D" );
    lbl += " Table group ";
    lbl += su.withz_ ? "+" : "-"; lbl += "Z";
    new uiLabel( this, lbl );
}


void uiTablePosProvGroup::usePar( const IOPar& )
{
}


bool uiTablePosProvGroup::fillPar( IOPar& ) const
{
    return true;
}


void uiTablePosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Table );
}
