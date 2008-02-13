/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiposprovgroup.cc,v 1.8 2008-02-13 13:28:48 cvsbert Exp $";

#include "uiposprovgroupstd.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiselsurvranges.h"
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


uiPosProvGroup::uiPosProvGroup( uiParent* p, const uiPosProvider::Setup& su )
    : uiGroup(p,su.seltxt_)
{
}


uiRangePosProvGroup::uiRangePosProvGroup( uiParent* p,
					  const uiPosProvider::Setup& su,
					  bool wstep )
    : uiPosProvGroup(p,su)
    , hrgfld_(0)
    , nrrgfld_(0)
    , zrgfld_(0)
{
    uiObject* attobj = 0;
    if ( su.is2d_ )
    {
	nrrgfld_ = new uiSelNrRange( this, uiSelNrRange::Gen, wstep );
	attobj = nrrgfld_->attachObj();
    }
    else
    {
	hrgfld_ = new uiSelHRange( this, wstep );
	attobj = hrgfld_->attachObj();
    }
    if ( su.withz_ )
    {
	zrgfld_ = new uiSelZRange( this, wstep );
	zrgfld_->attach( alignedBelow, attobj );
    }

    setHAlignObj( attobj );
}


void uiRangePosProvGroup::usePar( const IOPar& iop )
{
    CubeSampling cs; getCubeSampling( cs );
    cs.usePar( iop );

    if ( hrgfld_ )
	hrgfld_->setSampling( cs.hrg );
    if ( nrrgfld_ )
	nrrgfld_->setRange( cs.hrg.crlRange() );
    if ( zrgfld_ )
	zrgfld_->setRange( cs.zrg );
}


bool uiRangePosProvGroup::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type, sKey::Range );
    CubeSampling cs; getCubeSampling( cs );
    cs.fillPar( iop );
    return true;
}


void uiRangePosProvGroup::getCubeSampling( CubeSampling& cs ) const
{
    cs = SI().sampling( false );
    if ( hrgfld_ )
	cs.hrg = hrgfld_->getSampling();
    if ( nrrgfld_ )
	cs.hrg.set( StepInterval<int>(1,mUdf(int),1), nrrgfld_->getRange() );
    if ( zrgfld_ )
	cs.zrg = zrgfld_->getRange();
}


void uiRangePosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Range );
}


uiPolyPosProvGroup::uiPolyPosProvGroup( uiParent* p,
					const uiPosProvider::Setup& su )
    : uiPosProvGroup(p,su)
    , ctio_(*mMkCtxtIOObj(PickSet))
    , zrgfld_(0)
{
    ctio_.ctxt.parconstraints.set( sKey::Type, sKey::Polygon );
    ctio_.ctxt.allowcnstrsabsent = false;
    ctio_.fillIfOnlyOne( IOObjContext::Loc );
    polyfld_ = new uiIOObjSel( this, ctio_, sKey::Polygon );

    if ( su.withz_ )
    {
	zrgfld_ = new uiSelZRange( this, true );
	zrgfld_->attach( alignedBelow, polyfld_ );
    }

    setHAlignObj( polyfld_ );
}


uiPolyPosProvGroup::~uiPolyPosProvGroup()
{
    delete ctio_.ioobj; delete &ctio_;
}


#define mErrRet(s) { uiMSG().error(s); return false; }
#define mGetPolyKey(k) IOPar::compKey(sKey::Polygon,k)


void uiPolyPosProvGroup::usePar( const IOPar& iop )
{
    polyfld_->usePar( iop, sKey::Polygon );
    if ( zrgfld_ )
    {
	StepInterval<float> zrg( SI().zRange(true) );
	iop.get( mGetPolyKey(sKey::ZRange), zrg );
	zrgfld_->setRange( zrg );
    }
}


bool uiPolyPosProvGroup::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type, sKey::Polygon );
    if ( !polyfld_->fillPar(iop,sKey::Polygon) )
	mErrRet("Please select the polygon")
    iop.set( mGetPolyKey(sKey::ZRange), zrgfld_->getRange() );
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
    zrg = zrgfld_ ? zrgfld_->getRange() : SI().zRange(true);
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

#define mGetTableKey(k) IOPar::compKey(sKey::Table,k)

void uiTablePosProvGroup::usePar( const IOPar& iop )
{
    const char* idres = iop.find( mGetTableKey("ID") );
    const char* fnmres = iop.find( mGetTableKey(sKey::FileName) );
    const bool isfnm = fnmres && *fnmres;
    selfld_->setValue( !isfnm );
    if ( idres ) psfld_->setInput( idres );
    if ( fnmres ) tffld_->setInput( fnmres );
}


bool uiTablePosProvGroup::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type, sKey::Table );
    if ( selfld_->getBoolValue() )
    {
	if ( !psfld_->fillPar(iop,sKey::Table) )
	    mErrRet("Please select the Pick Set")
	iop.removeWithKey( mGetTableKey(sKey::FileName) );
    }
    else
    {
	const BufferString fnm = tffld_->getInput();
	if ( fnm.isEmpty() )
	    mErrRet("Please provide the table file name")
	else if ( File_isEmpty(fnm.buf()) )
	    mErrRet("Please select an existing/readable file")
	iop.set( mGetTableKey(sKey::FileName), fnm );
	iop.removeWithKey( mGetTableKey("ID") );
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
