/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiposprovgroup.cc,v 1.6 2008-02-12 09:07:35 cvsbert Exp $";

#include "uiposprovgroupstd.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "cubesampling.h"
#include "picksettr.h"
#include "ctxtioobj.h"
#include "survinfo.h"
#include "keystrs.h"
#include "ioobj.h"
#include "iopar.h"
#include "oddirs.h"
#include "filegen.h"

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


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiPolyPosProvGroup::fillPar( IOPar& iop ) const
{
    if ( !polyfld_->fillPar(iop) )
	mErrRet("Please select the polygon")

    const float fac = 1. / SI().zFactor();
    StepInterval<float> zrg = zfld_->getFStepInterval();
    zrg.scale( 1. / SI().zFactor() );
    iop.set( sKey::ZRange, zrg );
    return true;
}


bool uiPolyPosProvGroup::getID( MultiID& ky ) const
{
    if ( !polyfld_->commitInput(false) || !ctio_.ioobj )
	return false;
    ky = ctio_.ioobj->key();
    return true;
}


void uiPolyPosProvGroup::getZRange( StepInterval<float>& zrg ) const
{
    zrg = zfld_ ? zfld_->getFStepInterval() : SI().zRange(true);
}


void uiPolyPosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Polygon );
}


uiTablePosProvGroup::uiTablePosProvGroup( uiParent* p,
					const uiPosProvider::Setup& su )
    : uiPosProvGroup(p,su)
    , ctio_(*mMkCtxtIOObj(PickSet))
{
    ctio_.fillIfOnlyOne( IOObjContext::Loc );
    const CallBack selcb( mCB(this,uiTablePosProvGroup,selChg) );

    selfld_ = new uiGenInput( this, "Data from",
	    		      BoolInpSpec(true,"Pick Set","Table file") );
    selfld_->valuechanged.notify( selcb );
    psfld_ = new uiIOObjSel( this, ctio_ );
    psfld_->attach( alignedBelow, selfld_ );
    tffld_ = new uiIOFileSelect( this, sKey::FileName, true,
	    			 GetDataDir(), true );
    tffld_->getHistory( uiIOFileSelect::ixtablehistory );
    tffld_->attach( alignedBelow, selfld_ );

    setHAlignObj( selfld_ );
    mainwin()->finaliseDone.notify( selcb );
}


void uiTablePosProvGroup::selChg( CallBacker* )
{
    const bool isps = selfld_->getBoolValue();
    psfld_->display( isps );
    tffld_->display( !isps );
}


void uiTablePosProvGroup::usePar( const IOPar& iop )
{
    const char* idres = iop.find( "ID" );
    const char* fnmres = iop.find( sKey::FileName );
    const bool isfnm = fnmres && *fnmres;
    selfld_->setValue( !isfnm );
    if ( idres ) psfld_->setInput( idres );
    if ( fnmres ) tffld_->setInput( fnmres );
}


bool uiTablePosProvGroup::fillPar( IOPar& iop ) const
{
    if ( selfld_->getBoolValue() )
    {
	if ( !psfld_->fillPar(iop) )
	    mErrRet("Please select the Pick Set")
	iop.removeWithKey( sKey::FileName );
    }
    else
    {
	const BufferString fnm = tffld_->getInput();
	if ( fnm.isEmpty() )
	    mErrRet("Please provide the table file name")
	else if ( File_isEmpty(fnm.buf()) )
	    mErrRet("Please select an existing/readable file")
	iop.set( sKey::FileName, fnm );
	iop.removeWithKey( "ID" );
    }
    return true;
}


bool uiTablePosProvGroup::getID( MultiID& ky ) const
{
    if ( !selfld_->getBoolValue() || !psfld_->commitInput(false) )
	return false;
    ky = ctio_.ioobj->key();
    return true;
}


bool uiTablePosProvGroup::getFileName( BufferString& fnm ) const
{
    if ( selfld_->getBoolValue() )
	return false;
    fnm = tffld_->getInput();
    return true;
}


void uiTablePosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Table );
}
