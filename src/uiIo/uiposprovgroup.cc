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
    , posProvGroupChg(this)
{
}


uiRangePosProvGroup::uiRangePosProvGroup( uiParent* p,
					  const uiPosProvGroup::Setup& su )
    : uiPosProvGroup(p,su)
    , geomChanged(this)
    , setup_(su)
{
    uiObject* attobj = nullptr;
    const Survey::FullSubSel& fss = su.fss_;
    if ( su.is2d_ )
    {
	const LineSubSelSet lsss( fss.lineSubSelSet() );
	linergsfld_ = new uiSelSublineSet( this, su.withstep_, su.withz_,&lsss);
	mAttachCB( linergsfld_->rangeChanged, uiRangePosProvGroup::rangeChgCB );
	mAttachCB( linergsfld_->geomChanged, uiRangePosProvGroup::lineChgCB );
	attobj = linergsfld_->attachObj();
    }
    else
    {
	const CubeSubSel css( fss.cubeSubSel() );
	volrgfld_ = new uiSelSubvol( this, su.withstep_, setup_.withz_,
				     su.zdomkey_, &css );
	mAttachCB( volrgfld_->rangeChanged, uiRangePosProvGroup::rangeChgCB );
	attobj = volrgfld_->attachObj();
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


void uiRangePosProvGroup::initGrp( CallBacker* cb )
{
    if ( volrgfld_ )
	geomChanged.trigger( Pos::GeomID::get3D() );
    samplingCB( cb );
}


bool uiRangePosProvGroup::hasRandomSampling() const
{
    return samplingfld_ ? samplingfld_->getBoolValue() : false;
}


void uiRangePosProvGroup::usePar( const IOPar& iop )
{
    Survey::FullSubSel fss; getSubSel( fss );
    fss.usePar( iop );
    if ( volrgfld_ && fss.is3D() )
	volrgfld_->setSampling( fss.cubeSubSel() );
    else if ( linergsfld_ && !fss.is3D() )
	linergsfld_->setSampling( fss.lineSubSelSet() );

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
    Survey::FullSubSel fss; getSubSel( fss );
    fss.fillPar( iop );

    const bool dorandom = samplingfld_ && samplingfld_->getBoolValue();
    iop.setYN( sKey::Random(), dorandom );
    if ( dorandom )
	iop.set( sKey::NrValues(), nrsamplesfld_->getIntValue() );

    return true;
}


void uiRangePosProvGroup::getSummary( uiString& txt ) const
{
    txt.appendPhrase( setup_.withz_ ? tr("Sub-volume") : tr("Sub-area"),
				    uiString::Space, uiString::OnSameLine );
}


static void getExtrDefZSubSel( Pos::ZSubSel& zss )
{
    ZSampling zsamp = zss.zRange();
    int nrsamps = zsamp.nrSteps() + 1;
    if ( nrsamps > 2000 ) zsamp.step *= 1000;
    else if ( nrsamps > 200 ) zsamp.step *= 100;
    else if ( nrsamps > 20 ) zsamp.step *= 10;
    else if ( nrsamps > 10 ) zsamp.step *= 5;
    nrsamps = zsamp.nrSteps() + 1;
    zss.setOutputZRange( zsamp );
}


static void getExtrDefHorSubSel( int nrzsamps, Survey::HorSubSel& hss )
{
    CubeHorSubSel* chss = hss.asCubeHorSubSel();
    if ( chss )
    {
	const int nrextr = mCast( int, hss.totalSize() * nrzsamps );
	int blocks = nrextr / 50000;
	float fstepfac = (float) ( Math::Sqrt( (double)blocks ) );
	int stepfac = mNINT32(fstepfac);
	auto inlrg = chss->inlRange();
	auto crlrg = chss->crlRange();
	inlrg.step *= stepfac;
	crlrg.step *= stepfac;
	chss->setInlRange( inlrg );
	chss->setCrlRange( crlrg );
    }
    else
    {
	LineHorSubSel* lhss = hss.asLineHorSubSel();
	if ( !lhss )
	    return;
	auto trcrg = lhss->trcNrRange();
	trcrg.step = 10;
	lhss->setTrcNrRange( trcrg );
    }
}


void uiRangePosProvGroup::setExtractionDefaults()
{
    Survey::FullSubSel fss; getSubSel( fss );
    for ( int idx=0; idx<fss.nrGeomIDs(); idx++ )
    {
	Pos::ZSubSel& zss = fss.zSubSel(idx);
	getExtrDefZSubSel( zss );
	const int nrzsamps = zss.zRange().nrSteps()+1;
	Survey::HorSubSel& hss = fss.horSubSel(idx);
	getExtrDefHorSubSel( nrzsamps, hss );
    }

    if ( volrgfld_ && fss.is3D() )
	volrgfld_->setSampling( fss.cubeSubSel() );
    else if ( linergsfld_ && fss.is2D() )
	linergsfld_->setSampling( fss.lineSubSelSet() );
}


void uiRangePosProvGroup::getSubSel( Survey::FullSubSel& fss ) const
{
    if ( setup_.is2d_ )
    {
	fss.setEmpty();
	fss.set( linergsfld_->getSampling() );
    }
    else
    {
	fss = CubeSubSel( OD::UsrWork );
	BinID hsampStep( fss.cubeHorSubSel().inlStep(),
			 fss.cubeHorSubSel().crlStep() );
	Pos::Z_Type zsampStep = fss.zRange().step;
	auto css = volrgfld_->getSampling();
	if ( hasRandomSampling() )
	{
	    CubeHorSubSel& chss = css.cubeHorSubSel();
	    auto inlrg = chss.inlRange();
	    inlrg.step = hsampStep.inl();
	    auto crlrg = chss.crlRange();
	    crlrg.step = hsampStep.crl();
	    chss = CubeHorSubSel( inlrg, crlrg );
	    if ( volrgfld_->hasZ() )
	    {
		ZSampling zrg( volrgfld_->getZRange() );
		zrg.step = zsampStep;
		css.setZRange( zrg );
	    }
	}
	fss.set( css );
    }
}


void uiRangePosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Range(),
							uiStrings::sRange() );
}


void uiRangePosProvGroup::rangeChgCB( CallBacker* )
{
    posProvGroupChg.trigger();
}


void uiRangePosProvGroup::lineChgCB( CallBacker* cb )
{
    mCBCapsuleUnpack(Pos::GeomID,gid,cb);
    geomChanged.trigger( gid );
}


void uiRangePosProvGroup::samplingCB( CallBacker* )
{
    if ( !samplingfld_ )
	return;

    const bool showstep = !samplingfld_->getBoolValue();
    if ( volrgfld_ )
	volrgfld_->displayStep( showstep );
    if ( linergsfld_ )
	linergsfld_->displayStep( showstep );
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
