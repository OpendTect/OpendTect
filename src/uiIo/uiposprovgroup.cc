/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiposprovgroupstd.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uipossubsel.h"
#include "uiselsurvranges.h"
#include "uimsg.h"

#include "file.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "oddirs.h"
#include "picksettr.h"
#include "polyposprovider.h"
#include "survinfo.h"
#include "trckeyzsampling.h"

mImplFactory2Param(uiPosProvGroup,uiParent*,const uiPosProvGroup::Setup&,
		   uiPosProvGroup::factory);

// uiPosProvGroup::Setup

uiPosProvGroup::Setup::Setup( bool is_2d, bool with_step, bool with_z )
    : uiPosFiltGroup::Setup(is_2d)
    , withstep_(with_step)
    , withz_(with_z)
    , tkzs_(!is_2d)
    , withrandom_(false)
{
    if ( is_2d )
	tkzs_.set2DDef();
}


uiPosProvGroup::Setup::~Setup()
{
}


uiPosProvGroup::Setup& uiPosProvGroup::Setup::cs( const TrcKeyZSampling& tkzs )
{
    tkzs_ = tkzs;
    return *this;
}



// uiPosProvGroup

uiPosProvGroup::uiPosProvGroup( uiParent* p, const uiPosProvGroup::Setup& su )
    : uiPosFiltGroup(p,su)
    , posProvGroupChg(nullptr)
{
}


uiPosProvGroup::~uiPosProvGroup()
{}


// uiRangePosProvGroup
uiRangePosProvGroup::uiRangePosProvGroup( uiParent* p,
					  const uiPosProvGroup::Setup& su )
    : uiPosProvGroup(p,su)
    , hrgfld_(nullptr)
    , zrgfld_(nullptr)
    , nrrgfld_(nullptr)
    , samplingfld_(nullptr)
    , nrsamplesfld_(nullptr)
    , setup_(su)
{
    const CallBack cb( mCB(this,uiRangePosProvGroup,rangeChgCB) );
    uiObject* attobj = nullptr;
    if ( su.is2d_ )
    {
	nrrgfld_ =
	    new uiSelNrRange( this, uiSelNrRange::Gen, su.withstep_ );
	nrrgfld_->rangeChanged.notify( cb );
	nrrgfld_->setRange( su.tkzs_.hsamp_.crlRange() );
	attobj = nrrgfld_->attachObj();
    }
    else
    {
	hrgfld_ = new uiSelHRange( this, su.withstep_, &su.tkzs_.hsamp_ );
	hrgfld_->inlfld_->rangeChanged.notify( cb );
	hrgfld_->crlfld_->rangeChanged.notify( cb );
	attobj = hrgfld_->attachObj();
    }

    if ( setup_.withz_ )
    {
	zrgfld_ = new uiSelZRange( this, su.withstep_,
				   su.zdomkey_.buf(), su.zunitstr_.buf() );
	zrgfld_->setRangeLimits( su.tkzs_.zsamp_ );
	zrgfld_->rangeChanged.notify( cb );
	if ( attobj )
	    zrgfld_->attach( alignedBelow, attobj );
	attobj = zrgfld_->attachObj();
    }

    if ( su.withrandom_ && !su.is2d_ )
    {
	samplingfld_ = new uiGenInput( this, tr("Sampling Mode"),
			    BoolInpSpec(true,tr("Random"),tr("Regular")) );
	mAttachCB( samplingfld_->valueChanged,
		   uiRangePosProvGroup::samplingCB );
	if ( attobj )
	    samplingfld_->attach( alignedBelow, attobj );

	nrsamplesfld_ = new uiGenInput( this, tr("Number of samples"),
					IntInpSpec(4000) );
	nrsamplesfld_->attach( rightOf, samplingfld_ );
	attobj = samplingfld_->attachObj();
    }

    if ( attobj ) setHAlignObj( attobj );

    mAttachCB( postFinalize(), uiRangePosProvGroup::initGrp );
}


uiRangePosProvGroup::~uiRangePosProvGroup()
{
    detachAllNotifiers();
}


void uiRangePosProvGroup::initGrp( CallBacker* )
{
    samplingCB( nullptr );
}


bool uiRangePosProvGroup::hasRandomSampling() const
{
    return samplingfld_ ? samplingfld_->getBoolValue() : false;
}


void uiRangePosProvGroup::usePar( const IOPar& iop )
{
    TrcKeyZSampling cs; getTrcKeyZSampling( cs );
    cs.usePar( iop );

    if ( hrgfld_ )
	hrgfld_->setSampling( cs.hsamp_ );
    if ( zrgfld_ )
	zrgfld_->setRange( cs.zsamp_ );
    if ( nrrgfld_ )
    {
	StepInterval<int> trcrg = cs.hsamp_.crlRange();
	iop.get( IOPar::compKey(sKey::TrcRange(),0), trcrg );
	nrrgfld_->setRange( trcrg );
	if ( zrgfld_ )
	{
	    StepInterval<float> zrg = cs.zsamp_;
	    iop.get( IOPar::compKey(sKey::ZRange(),0), zrg );
	    zrgfld_->setRange( zrg );
	}
    }
    if	( samplingfld_ )
    {
	bool random = true;
	iop.getYN( sKey::Random(), random );
	samplingfld_->setValue( random );

	int nrsamples = 4000;
	if ( random )
	    iop.get( sKey::NrValues(), nrsamples );
	nrsamplesfld_->setValue( nrsamples );

	samplingCB( nullptr );
    }
}


bool uiRangePosProvGroup::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type(), sKey::Range() );

    if ( samplingfld_ )
    {
	iop.setYN( sKey::Random(), samplingfld_->getBoolValue() );
	if ( samplingfld_->getBoolValue() )
	    iop.set( sKey::NrValues(), nrsamplesfld_->getIntValue() );
    }

    TrcKeyZSampling cs; getTrcKeyZSampling( cs );

    if ( setup_.is2d_ )
    {
	iop.set( IOPar::compKey(sKey::GeomID(),0), cs.hsamp_.getGeomID() );
	iop.set( IOPar::compKey(sKey::TrcRange(),0), cs.hsamp_.crlRange() );
	if ( setup_.withz_ )
	    iop.set( IOPar::compKey(sKey::ZRange(),0), cs.zsamp_ );

	return true;
    }

    cs.fillPar( iop );
    return true;
}


void uiRangePosProvGroup::getSummary( BufferString& txt ) const
{
    TrcKeyZSampling cs; getTrcKeyZSampling( cs );
    txt += setup_.withz_ ? "Sub-volume" : "Sub-area";
}


static void getExtrDefTrcKeyZSampling( TrcKeyZSampling& cs )
{
    int nrsamps = cs.zsamp_.nrSteps() + 1;
    if ( nrsamps > 2000 ) cs.zsamp_.step *= 1000;
    else if ( nrsamps > 200 ) cs.zsamp_.step *= 100;
    else if ( nrsamps > 20 ) cs.zsamp_.step *= 10;
    else if ( nrsamps > 10 ) cs.zsamp_.step *= 5;
    nrsamps = cs.zsamp_.nrSteps() + 1;

    const int nrextr = sCast( int, cs.hsamp_.totalNr() * nrsamps );
    int blocks = nrextr / 50000;
    float fstepfac = float( Math::Sqrt( double(blocks) ) );
    int stepfac = mNINT32(fstepfac);
    cs.hsamp_.step_.inl() *= stepfac;
    cs.hsamp_.step_.crl() *= stepfac;
}


void uiRangePosProvGroup::setExtractionDefaults()
{
    TrcKeyZSampling cs( true ); getExtrDefTrcKeyZSampling( cs );
    if ( hrgfld_ )
	hrgfld_->setSampling( cs.hsamp_ );
    if ( nrrgfld_ )
    {
	StepInterval<int> rg( nrrgfld_->getRange() );
	rg.step = 10;
	nrrgfld_->setRange( rg );
    }
    zrgfld_->setRange( cs.zsamp_ );
}


void uiRangePosProvGroup::getTrcKeyZSampling( TrcKeyZSampling& cs ) const
{
    cs = SI().sampling( true );
    if ( hrgfld_ )
	cs.hsamp_ = hrgfld_->getSampling();
    if ( nrrgfld_ )
	cs.hsamp_.set( StepInterval<int>(0,mUdf(int),1), nrrgfld_->getRange() );
    if ( zrgfld_ )
	cs.zsamp_ = zrgfld_->getRange();
    if ( hasRandomSampling() ) {
	cs.hsamp_.step_ = SI().sampling(true).hsamp_.step_;
	cs.zsamp_.step = SI().sampling(true).zsamp_.step;
    }
}


void uiRangePosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Range() );
}


void uiRangePosProvGroup::rangeChgCB( CallBacker* )
{
    posProvGroupChg.trigger();
}


void uiRangePosProvGroup::samplingCB( CallBacker* )
{
    if ( !samplingfld_ )
	return;

    bool showstep = !samplingfld_->getBoolValue();
    if ( hrgfld_ )
	hrgfld_->displayStep( showstep );
    if ( zrgfld_ )
	zrgfld_->displayStep( showstep );
    if ( nrrgfld_ )
	nrrgfld_->displayStep( showstep );
    if ( nrsamplesfld_ )
	nrsamplesfld_->display( !showstep );
    posProvGroupChg.trigger();
}



// uiPolyPosProvGroup
uiPolyPosProvGroup::uiPolyPosProvGroup( uiParent* p,
					const uiPosProvGroup::Setup& su )
    : uiPosProvGroup(p,su)
    , stepfld_(nullptr)
    , zrgfld_(nullptr)
{
    IOObjContext ctxt = mIOObjContext( PickSet );
    PickSetTranslator::fillConstraints( ctxt, true );
    polyfld_ = new uiIOObjSel( this, ctxt, uiStrings::sPolygon() );

    uiGroup* attachobj = polyfld_;
    if ( su.withstep_ )
    {
	stepfld_ = new uiSelSteps( this, false );
	stepfld_->attach( alignedBelow, polyfld_ );
	attachobj = stepfld_;
    }

    if ( su.withz_ )
    {
	zrgfld_ = new uiSelZRange( this, true,
				   su.zdomkey_.buf(), su.zunitstr_.buf() );
	zrgfld_->attach( alignedBelow, attachobj );
	attachobj = zrgfld_;
    }

    inoutfld_ = new uiGenInput( this, tr("Use Positions"),
			BoolInpSpec(true,tr("Inside"),tr("Outside")) );
    inoutfld_->valueChanged.notify( mCB(this,uiPolyPosProvGroup,inoutCB) );

    bboxfld_ = new uiPosSubSel( this,
			uiPosSubSel::Setup(false,su.withz_) );

    inoutfld_->attach( alignedBelow, attachobj );
    bboxfld_->attach( alignedBelow, inoutfld_);

    inoutCB( nullptr );

    setHAlignObj( polyfld_ );
}


uiPolyPosProvGroup::~uiPolyPosProvGroup()
{
}


void uiPolyPosProvGroup::inoutCB( CallBacker* )
{
    bboxfld_->display( !inoutfld_->getBoolValue() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }
#define mGetPolyKey(k) IOPar::compKey(sKey::Polygon(),k)


void uiPolyPosProvGroup::usePar( const IOPar& iop )
{
    polyfld_->usePar( iop, sKey::Polygon() );
    if ( stepfld_ )
    {
	BinID stps( SI().sampling(true).hsamp_.step_ );
	iop.get( mGetPolyKey(sKey::StepInl()), stps.inl() );
	iop.get( mGetPolyKey(sKey::StepCrl()), stps.crl() );
	stepfld_->setSteps( stps );
    }
    if ( zrgfld_ )
    {
	StepInterval<float> zrg( SI().zRange(true) );
	iop.get( mGetPolyKey(sKey::ZRange()), zrg );
	zrgfld_->setRange( zrg );
    }

    bool inside = true;
    iop.getYN( mGetPolyKey(Pos::PolyProvider3D::sInside()), inside );
    inoutfld_->setValue( inside );
    inoutCB( nullptr );

    TrcKeyZSampling tkzs;
    PtrMan<IOPar> bbpar =
	iop.subselect( mGetPolyKey(Pos::PolyProvider3D::sBoundingBox()) );
    if ( bbpar )
	tkzs.usePar( *bbpar );
    bboxfld_->setInput( tkzs );
}


bool uiPolyPosProvGroup::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type(), sKey::Polygon() );
    if ( !polyfld_->ioobj() || !polyfld_->fillPar(iop,sKey::Polygon()) )
	mErrRet(uiStrings::phrSelect(uiStrings::sPolygon()))

    const BinID stps(
	stepfld_ ? stepfld_->getSteps() : SI().sampling(true).hsamp_.step_ );
    iop.set( mGetPolyKey(sKey::StepInl()), stps.inl() );
    iop.set( mGetPolyKey(sKey::StepCrl()), stps.crl() );
    iop.set( mGetPolyKey(sKey::ZRange()),
	zrgfld_ ? zrgfld_->getRange() : SI().zRange(true) );

    iop.setYN( mGetPolyKey(Pos::PolyProvider3D::sInside()),
        inoutfld_->getBoolValue() );
    IOPar bbpar;
    bboxfld_->envelope().fillPar( bbpar );
    iop.mergeComp( bbpar, mGetPolyKey(Pos::PolyProvider3D::sBoundingBox()) );
    return true;
}


void uiPolyPosProvGroup::getSummary( BufferString& txt ) const
{
    txt.set( inoutfld_->getBoolValue() ? "Inside " : "Outside " )
	.add( "polygon" );
    const IOObj* ioobj = polyfld_->ioobj( true );
    if ( ioobj )
	txt.add( " '" ).add( ioobj->name() ).add( "." );
}


void uiPolyPosProvGroup::setExtractionDefaults()
{
    TrcKeyZSampling cs( true ); getExtrDefTrcKeyZSampling( cs );
    if ( stepfld_ ) stepfld_->setSteps( cs.hsamp_.step_ );
    if ( zrgfld_ ) zrgfld_->setRange( cs.zsamp_ );
}


bool uiPolyPosProvGroup::getID( MultiID& ky ) const
{
    ky = polyfld_->key();
    return !ky.isUdf();
}


void uiPolyPosProvGroup::getZRange( StepInterval<float>& zrg ) const
{
    zrg = zrgfld_ ? zrgfld_->getRange() : SI().zRange(true);
}


void uiPolyPosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Polygon() );
}



// uiTablePosProvGroup
uiTablePosProvGroup::uiTablePosProvGroup( uiParent* p,
		const uiPosProvGroup::Setup& su, bool onlypointset )
    : uiPosProvGroup(p,su)
{
    const CallBack selcb( mCB(this,uiTablePosProvGroup,selChg) );

    selfld_ = new uiGenInput(this, tr("Data from"),
		    BoolInpSpec(true,uiStrings::sPointSet(),
			uiStrings::phrJoinStrings(uiStrings::sTable(),
						  uiStrings::sFile())));
    selfld_->valueChanged.notify( selcb );

    IOObjContext ctxt = mIOObjContext( PickSet );
    PickSetTranslator::fillConstraints( ctxt, false );
    psfld_ = new uiIOObjSel( this, ctxt, uiStrings::sPointSet() );
    psfld_->attach( alignedBelow, selfld_ );
    tffld_ = new uiIOFileSelect( this, toUiString(sKey::FileName()), true,
				 GetDataDir(), true );
    tffld_->getHistory( uiIOFileSelect::ixtablehistory() );
    tffld_->attach( alignedBelow, selfld_ );

    selfld_->display( !onlypointset );
    tffld_->display( !onlypointset );

    setHAlignObj( selfld_ );
    postFinalize().notify( selcb );
}


uiTablePosProvGroup::~uiTablePosProvGroup()
{
}


void uiTablePosProvGroup::selChg( CallBacker* )
{
    const bool isps = selfld_->getBoolValue();
    psfld_->display( isps );
    tffld_->display( !isps );
}


#define mGetTableKey(k) IOPar::compKey(sKey::Table(),k)

void uiTablePosProvGroup::usePar( const IOPar& iop )
{
    const BufferString fnmres = iop.find( mGetTableKey(sKey::FileName()) );
    const bool isfnm = !fnmres.isEmpty();
    selfld_->setValue( !isfnm );
    MultiID tableid;
    iop.get( mGetTableKey("ID"), tableid );
    if ( !tableid.isUdf())
	psfld_->setInput( tableid );

    if ( isfnm )
	tffld_->setInput( fnmres );
}


bool uiTablePosProvGroup::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type(), sKey::Table() );
    if ( selfld_->getBoolValue() )
    {
	if ( !psfld_->fillPar(iop,sKey::Table()) )
	    mErrRet(uiStrings::phrSelect(uiStrings::sPointSet()))
	iop.removeWithKey( mGetTableKey(sKey::FileName()) );
    }
    else
    {
	const BufferString fnm = tffld_->getInput();
	if ( fnm.isEmpty() )
	    mErrRet(tr("Provide the table file name"))
	else if ( File::isEmpty(fnm.buf()) )
	    mErrRet(tr("Select an existing/readable file"))
	iop.set( mGetTableKey(sKey::FileName()), fnm );
	iop.removeWithKey( mGetTableKey("ID") );
    }
    return true;
}


void uiTablePosProvGroup::getSummary( BufferString& txt ) const
{
    txt += "In table";
}


bool uiTablePosProvGroup::getID( MultiID& ky ) const
{
    if ( !selfld_->getBoolValue() )
	return false;

    ky = psfld_->key();
    return !ky.isUdf();
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
    uiPosProvGroup::factory().addCreator( create, sKey::Table(),
					  uiStrings::sPointSet() );
}
