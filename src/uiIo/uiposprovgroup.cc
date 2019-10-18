/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/


#include "uiposprovgroupstd.h"

#include "uifilesel.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uipicksetsel.h"
#include "uipossubsel.h"
#include "uiselsurvranges.h"
#include "uimsg.h"
#include "horsubsel.h"
#include "ioobjctxt.h"
#include "posvecdatasettr.h"
#include "trckeyzsampling.h"
#include "uistrings.h"

#include "file.h"
#include "ioobj.h"
#include "ioobjctxt.h"
#include "iopar.h"
#include "keystrs.h"
#include "oddirs.h"
#include "polyposprovider.h"
#include "survinfo.h"
#include "trckeyzsampling.h"

mImplClassFactory(uiPosProvGroup,factory);


uiPosProvGroup::uiPosProvGroup( uiParent* p, const uiPosProvGroup::Setup& su )
    : uiPosFiltGroup(p,su)
    , posProvGroupChg(0)
{
}


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
    uiObject* attobj = nullptr;
    if ( su.is2d_ )
    {
	nrrgfld_ =
	    new uiSelNrRange( this, uiSelNrRange::Gen, su.withstep_ );
	nrrgfld_->setRange( su.tkzs_.hsamp_.crlRange() );
	attobj = nrrgfld_->attachObj();
    }
    else
    {
	hrgfld_ = new uiSelHRange( this, su.tkzs_.hsamp_, su.withstep_ );
	attobj = hrgfld_->attachObj();
    }

    if ( setup_.withz_ )
    {
	zrgfld_ = new uiSelZRange( this, su.tkzs_.zsamp_, su.withstep_,
				   uiString(), su.zdomkey_ );
	if ( attobj )
	    zrgfld_->attach( alignedBelow, attobj );
	attobj = zrgfld_->attachObj();
    }

    if ( !su.is2d_ && su.withrandom_ )
    {
	samplingfld_ = new uiGenInput( this, tr("Sampling Mode"),
			BoolInpSpec(true,tr("Random"),tr("Regular")) );
	mAttachCB( samplingfld_->valuechanged,
		   uiRangePosProvGroup::samplingCB );
	if ( attobj )
	    samplingfld_->attach( alignedBelow, attobj );

	nrsamplesfld_ = new uiGenInput( this, uiStrings::sNrSamples(),
					IntInpSpec(4000) );
	nrsamplesfld_->attach( rightOf, samplingfld_ );
	attobj = samplingfld_->attachObj();
    }

    if ( attobj ) setHAlignObj( attobj );

    mAttachCB( postFinalise(), uiRangePosProvGroup::initGrp );
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

    if ( samplingfld_ ) {
	iop.setYN( sKey::Random(), samplingfld_->getBoolValue() );
	if ( samplingfld_->getBoolValue() )
	    iop.set( sKey::NrValues(), nrsamplesfld_->getIntValue() );
    }

    TrcKeyZSampling cs; getTrcKeyZSampling( cs );

    if ( setup_.is2d_ )
    {
	iop.set( IOPar::compKey(sKey::TrcRange(),0), cs.hsamp_.crlRange() );
	if ( setup_.withz_ )
	    iop.set( IOPar::compKey(sKey::ZRange(),0), cs.zsamp_ );

	return true;
    }

    cs.fillPar( iop );
    return true;
}


void uiRangePosProvGroup::getSummary( uiString& txt ) const
{
    TrcKeyZSampling cs; getTrcKeyZSampling( cs );
    txt.appendPhrase( setup_.withz_ ? tr("Sub-volume") : tr("Sub-area"),
				    uiString::Space, uiString::OnSameLine );
}


static void getExtrDefTrcKeyZSampling( TrcKeyZSampling& cs )
{
    int nrsamps = cs.zsamp_.nrSteps() + 1;
    if ( nrsamps > 2000 ) cs.zsamp_.step *= 1000;
    else if ( nrsamps > 200 ) cs.zsamp_.step *= 100;
    else if ( nrsamps > 20 ) cs.zsamp_.step *= 10;
    else if ( nrsamps > 10 ) cs.zsamp_.step *= 5;
    nrsamps = cs.zsamp_.nrSteps() + 1;

    const int nrextr = mCast( int, cs.hsamp_.totalNr() * nrsamps );
    int blocks = nrextr / 50000;
    float fstepfac = (float) ( Math::Sqrt( (double)blocks ) );
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
    cs = TrcKeyZSampling( OD::UsrWork );
    if ( hrgfld_ )
	cs.hsamp_ = TrcKeySampling( CubeHorSubSel(hrgfld_->getSampling()) );
    else if ( nrrgfld_ )
    {
	auto gid = setup_.tkzs_.hsamp_.getGeomID();
	if ( !gid.isValid() )
	    { pErrMsg("GeomID required"); gid = Pos::GeomID::getDefault2D(); }
	LineHorSubSel lhss( gid );
	lhss.setTrcNrRange( nrrgfld_->getRange() );
	cs.hsamp_ = TrcKeySampling( lhss );
    }

    if ( zrgfld_ )
	cs.zsamp_ = zrgfld_->getRange();
}


void uiRangePosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Range(),
							uiStrings::sRange() );
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


uiPolyPosProvGroup::uiPolyPosProvGroup( uiParent* p,
					const uiPosProvGroup::Setup& su )
    : uiPosProvGroup(p,su)
    , zrgfld_(0)
    , stepfld_(0)
{
    polyfld_ = new uiPickSetIOObjSel( this, true,
				      uiPickSetIOObjSel::PolygonOnly );

    uiGroup* attachobj = polyfld_;
    if ( su.withstep_ )
    {
	stepfld_ = new uiSelSteps( this, false );
	stepfld_->attach( alignedBelow, polyfld_ );
	attachobj = stepfld_;
    }

    if ( su.withz_ )
    {
	zrgfld_ = new uiSelZRange( this, true, false, uiString(), su.zdomkey_ );
	zrgfld_->attach( alignedBelow, attachobj );
	attachobj = zrgfld_;
    }

    inoutfld_ = new uiGenInput( this, tr("Use Positions"),
			BoolInpSpec(true,tr("Inside"),tr("Outside")) );
    inoutfld_->valuechanged.notify( mCB(this,uiPolyPosProvGroup,inoutCB) );

    bboxfld_ = new uiPosSubSel( this,
			uiPosSubSel::Setup(false,su.withz_).withstep(false) );

    inoutfld_->attach( alignedBelow, attachobj );
    bboxfld_->attach( alignedBelow, inoutfld_ );

    inoutCB( nullptr );

    setHAlignObj( polyfld_ );
}


uiPolyPosProvGroup::~uiPolyPosProvGroup()
{}


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
	BinID stps( SI().inlStep(), SI().crlStep() );
	iop.get( mGetPolyKey(sKey::StepInl()), stps.inl() );
	iop.get( mGetPolyKey(sKey::StepCrl()), stps.crl() );
	stepfld_->setSteps( stps );
    }
    if ( zrgfld_ )
    {
	StepInterval<float> zrg( SI().zRange(OD::UsrWork) );
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
    if ( !polyfld_->commitInput() || !polyfld_->fillPar(iop,sKey::Polygon()) )
	mErrRet(uiStrings::phrSelect(uiStrings::sPolygon()))

    BinID stps( SI().inlStep(), SI().crlStep() );
    if ( stepfld_ )
	stps = stepfld_->getSteps();
    iop.set( mGetPolyKey(sKey::StepInl()), stps.inl() );
    iop.set( mGetPolyKey(sKey::StepCrl()), stps.crl() );
    iop.set( mGetPolyKey(sKey::ZRange()),
	zrgfld_ ? zrgfld_->getRange() : SI().zRange(OD::UsrWork) );

    iop.setYN( mGetPolyKey(Pos::PolyProvider3D::sInside()),
	       inoutfld_->getBoolValue() );
    IOPar bbpar;
    bboxfld_->envelope().fillPar( bbpar );
    iop.mergeComp( bbpar, mGetPolyKey(Pos::PolyProvider3D::sBoundingBox()) );
    return true;
}


void uiPolyPosProvGroup::getSummary( uiString& txt ) const
{
    txt =  tr("Within polygon");
    const IOObj* ioobj = polyfld_->ioobj( true );
    if ( ioobj )
	txt.appendPhrase( toUiString("'%1'").arg(toUiString(ioobj->name())),
				uiString::Space, uiString::OnSameLine );
}


void uiPolyPosProvGroup::setExtractionDefaults()
{
    TrcKeyZSampling cs( true ); getExtrDefTrcKeyZSampling( cs );
    if ( stepfld_ ) stepfld_->setSteps( cs.hsamp_.step_ );
    if ( zrgfld_ ) zrgfld_->setRange( cs.zsamp_ );
}


bool uiPolyPosProvGroup::getID( DBKey& ky ) const
{
    const IOObj* ioobj = polyfld_->ioobj( true );
    if ( !ioobj )
	return false;
    ky = ioobj->key();
    return true;
}


void uiPolyPosProvGroup::getZRange( StepInterval<float>& zrg ) const
{
    zrg = zrgfld_ ? zrgfld_->getRange() : SI().zRange(OD::UsrWork);
}


void uiPolyPosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Polygon(),
						    uiStrings::sPolygon() );
}


uiTablePosProvGroup::uiTablePosProvGroup( uiParent* p,
					const uiPosProvGroup::Setup& su )
    : uiPosProvGroup(p,su)
{
    typfld_ = new uiGenInput( this, tr("Get locations from"),
			      BoolInpSpec( true, uiStrings::sPick(mPlural),
						 uiStrings::sCrossPlot() ) );
    typfld_->valuechanged.notify( mCB(this,uiTablePosProvGroup,typSelCB) );

    psfld_ = new uiPickSetIOObjSel( this, true );
    psfld_->attach( alignedBelow, typfld_ );

    pvdsfld_ = new uiIOObjSel( this, mIOObjContext(PosVecDataSet),
			       uiStrings::sCrossPlotData() );
    pvdsfld_->attach( alignedBelow, typfld_ );

    setHAlignObj( typfld_ );
    mAttachCB( postFinalise(), uiTablePosProvGroup::initGrpCB );
}


#define mGetTableKey(k) IOPar::compKey(sKey::Table(),k)

void uiTablePosProvGroup::usePar( const IOPar& iop )
{
    const char* idres = iop.find( mGetTableKey("ID") );
    const DBKey dbky( idres );
    PtrMan<IOObj> ioobj = getIOObj( dbky );
    if ( !ioobj )
	return;

    const bool isps = ioobj->translator() != mTranslGroupName( PosVecDataSet );
    typfld_->setValue( isps );
    if ( isps )
	psfld_->setInput( dbky );
    else
	pvdsfld_->setInput( dbky );

    typSelCB( 0 );
}


void uiTablePosProvGroup::initGrpCB( CallBacker* )
{
    typSelCB(0);
}

void uiTablePosProvGroup::typSelCB( CallBacker* )
{
    const bool isps = typfld_->getBoolValue();
    psfld_->display( isps );
    pvdsfld_->display( !isps );
}


bool uiTablePosProvGroup::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type(), sKey::Table() );
    if ( typfld_->getBoolValue() )
    {
	if ( !psfld_->fillPar(iop,sKey::Table()) )
	    mErrRet(uiStrings::phrSelect(uiStrings::sPointSet()))
    }
    else
    {
	if ( !pvdsfld_->fillPar(iop,sKey::Table()) )
	    mErrRet(uiStrings::phrSelect(uiStrings::sCrossPlotData()))
    }

    iop.removeWithKey( mGetTableKey(sKey::FileName()) );
    return true;
}


void uiTablePosProvGroup::getSummary( uiString& txt ) const
{
    txt.appendPhrase( tr("In table"), uiString::Space, uiString::OnSameLine );
}


bool uiTablePosProvGroup::getID( DBKey& ky ) const
{
    const bool isps = typfld_->getBoolValue();
    const IOObj* ioobj = isps ? psfld_->ioobj(true) : pvdsfld_->ioobj(true);
    ky = ioobj ? ioobj->key() : DBKey();
    return ioobj;
}


void uiTablePosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Table(),
					  uiStrings::sPointSet() );
}
