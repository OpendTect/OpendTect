/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2010
-*/


#include "uisynthtorealscale.h"

#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "survinfo.h"
#include "position.h"
#include "seistrc.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "seisbufadapters.h"
#include "seiseventsnapper.h"
#include "seisprovider.h"
#include "seisseldata.h"
#include "seisselsetup.h"
#include "stratlayermodel.h"
#include "stratlevel.h"
#include "stratsynthdatamgr.h"
#include "statparallelcalc.h"
#include "picksettr.h"
#include "waveletmanager.h"

#include "uistratsynthseltools.h"
#include "uipicksetsel.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseparator.h"
#include "uislider.h"
#include "uiwaveletsel.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uihistogramdisplay.h"
#include "uiaxishandler.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitaskrunnerprovider.h"
#include "od_helpids.h"

#define mErrRetBool(s)\
{ uiMSG().error(s); return false; }\

#define mErrRet(s)\
{ uiMSG().error(s); return; }\

class uiSynthToRealScaleStatsDisp : public uiGroup
{ mODTextTranslationClass(uiSynthToRealScaleStatsDisp);
public:

uiSynthToRealScaleStatsDisp( uiParent* p, const char* nm, bool left )
    : uiGroup(p,nm)
    , usrval_(mUdf(float))
    , usrValChanged(this)
{
    uiFunctionDisplay::Setup su;
    su.annoty( false ).noyaxis( true ).noy2axis( true ).drawgridlines( false );
    dispfld_ = new uiHistogramDisplay( this, su );
    dispfld_->xAxis()->setCaption( uiString::empty() );
    dispfld_->setPrefWidth( 260 );
    dispfld_->setPrefHeight( GetGoldenMinor(260) );

    uiSlider::Setup slsu;
    slsu.withedit( true );
    valueslider_ = new uiSlider( this, slsu, "Value" );
    valueslider_->valueChanged.notify(
	    mCB(this,uiSynthToRealScaleStatsDisp,sliderChgCB) );
    valueslider_->attach( alignedBelow, dispfld_ );
    valueslider_->setStretch( 2, 1 );

    uiLabel* lbl = new uiLabel( this, toUiString(nm) );
    dispfld_->attach( centeredBelow, lbl );
    setHAlignObj( dispfld_ );
}

void setValue( float val )
{
    const uiAxisHandler* xaxis = dispfld_->xAxis();
    const StepInterval<float> xrg = xaxis->range();
    valueslider_->setScale( xrg.step * 0.001f, 0 );
    valueslider_->setInterval( xrg );
    valueslider_->setValue( val );
    setFromSlider();
}

void setFromSlider()
{
    usrval_ = valueslider_->getFValue();
    dispfld_->setMarkValue( usrval_, true );
}

void sliderChgCB( CallBacker* )
{
    setFromSlider();
    usrValChanged.trigger();
}

    float		usrval_;

    uiHistogramDisplay*	dispfld_;
    uiSlider*		valueslider_;
    Notifier<uiSynthToRealScaleStatsDisp>	usrValChanged;

};


uiSynthToRealScale::uiSynthToRealScale( uiParent* p, const DataMgr& dmgr,
		    const DBKey& wid, bool use2d, const LevelID& lvlid )
    : uiDialog(p,Setup(tr("Scale synthetics"),
		       tr("Determine scaling for '%1'").arg( wid.name() ),
			mODHelpKey(mSynthToRealScaleHelpID) ))
    , seisev_(*new SeisEvent)
    , use2dseis_(use2d)
    , datamgr_(dmgr.getProdMgr())
    , inpwvltid_(wid)
    , seisfld_(0)
    , horizon_(0)
    , horiter_(0)
    , seldata_(0)
{
    datamgr_->getNames( synthnms_, DataMgr::OnlyZO );

#define mNoDealRet(cond,msg) \
    if ( cond ) \
	{ new uiLabel( this, msg ); return; }
    mNoDealRet( Strat::LVLS().isEmpty(), tr("No Stratigraphic Levels defined") )
    mNoDealRet( datamgr_->layerModel().isEmpty(), tr("Generate models first") )
    mNoDealRet( synthnms_.isEmpty(), tr("No Zero Offset Synthetics") )
    mNoDealRet( inpwvltid_.isInvalid(), uiStrings::phrCreate(
							tr("a Wavelet first")) )

    uiSeisSel::Setup uisssu( use2dseis_, false );
    seisfld_ = new uiSeisSel( this, uiSeisSel::ioContext(uisssu.geom_,true),
			      uisssu );

    uiLabeledComboBox* synthlcb = 0;
    if ( synthnms_.size() > 1 )
    {
	synthlcb = new uiLabeledComboBox( this, tr("Synthetics") );
	synthfld_ = synthlcb->box();
	synthfld_->addItems( synthnms_ );
	synthfld_->setCurrentItem( 0 );
	synthlcb->attach( alignedBelow, seisfld_ );
    }

    lvlhorfld_ = new uiStratLevelHorSel( this, lvlid );
    lvlhorfld_->set2D( use2dseis_ );
    if ( synthlcb )
	lvlhorfld_->attach( alignedBelow, synthlcb );
    else
	lvlhorfld_->attach( alignedBelow, seisfld_ );

    Seis::SelSetup sssu( uisssu.geom_ );
    sssu.onlyrange( false ).multiline( true ).withoutz( true );
    seissubselfld_ = uiSeisSubSel::get( this, sssu );
    seissubselfld_->attach( alignedBelow, lvlhorfld_ );

    uiStratSeisEvent::Setup ssesu( true );
    ssesu.sellevel( false ).levelid( lvlid );
    evfld_ = new uiStratSeisEvent( this, ssesu );
    evfld_->attach( alignedBelow, seissubselfld_ );

    uiPushButton* gobut = new uiPushButton( this, tr("Extract amplitudes"),
				mCB(this,uiSynthToRealScale,goPushCB), true );
    gobut->attach( alignedBelow, evfld_ );

    uiSeparator* sep = new uiSeparator( this, "separator" );
    sep->attach( stretchedBelow, gobut );

    valislbl_ = new uiLabel( this, tr("       [Amplitude values]       ") );
    valislbl_->setAlignment( OD::Alignment::HCenter );
    valislbl_->attach( centeredBelow, sep );

    uiGroup* statsgrp = new uiGroup( this, "Stats displays" );

    synthstatsfld_ = new uiSynthToRealScaleStatsDisp( statsgrp, "Synthetics",
						      true );
    realstatsfld_ = new uiSynthToRealScaleStatsDisp( statsgrp, "Real Seismics",
						     false );
    realstatsfld_->attach( rightOf, synthstatsfld_ );
    statsgrp->attach( centeredBelow, valislbl_ );
    statsgrp->setHAlignObj( realstatsfld_ );

    finalscalefld_ = new uiGenInput( this, uiString::empty(),
				     FloatInpSpec() );
    finalscalefld_->attach( centeredBelow, statsgrp );
    new uiLabel( this, tr("Scaling factor"), finalscalefld_ );

    uiWaveletIOObjSel::Setup wvsu( tr("Save scaled Wavelet as") );
    wvltfld_ = new uiWaveletIOObjSel( this, wvsu, false );
    wvltfld_->attach( alignedBelow, finalscalefld_ );

    postFinalise().notify( mCB(this,uiSynthToRealScale,initWin) );
}


uiSynthToRealScale::~uiSynthToRealScale()
{
    detachAllNotifiers();

    delete horiter_;
    if ( horizon_ )
	horizon_->unRef();
    delete seldata_;
    delete &seisev_;
}


void uiSynthToRealScale::initWin( CallBacker* )
{
    updSynthStats();

    const CallBack valchgcb( mCB(this,uiSynthToRealScale,statsUsrValChgCB) );
    synthstatsfld_->usrValChanged.notify( valchgcb );
    realstatsfld_->usrValChanged.notify( valchgcb );
    mAttachCB( lvlhorfld_->levelSel, uiSynthToRealScale::levelSelCB );
    mAttachCB( evfld_->anyChange, uiSynthToRealScale::evChgCB );
    mAttachCB( seisfld_->selectionDone, uiSynthToRealScale::seisSelCB );
    if ( synthfld_ )
	mAttachCB( synthfld_->selectionChanged, uiSynthToRealScale::synthSelCB);
}


#define mSetFldValue( fld, val ) \
	 if ( !mIsUdf(val) ) \
	    fld->setValue( (float)val ); \


void uiSynthToRealScale::setScaleFld()
{
    const float synthval = synthstatsfld_->usrval_;
    const float realval = realstatsfld_->usrval_;
    if ( mIsUdf(synthval) || mIsUdf(realval) || synthval == 0 )
	finalscalefld_->setValue( mUdf(float) );
    else
	finalscalefld_->setValue( realval / synthval );

    mSetFldValue( synthstatsfld_, synthval );
    mSetFldValue( realstatsfld_, realval );
}


void uiSynthToRealScale::statsUsrValChgCB( CallBacker* )
{
    setScaleFld();
}


void uiSynthToRealScale::levelSelCB( CallBacker* )
{
    evfld_->setLevel( lvlhorfld_->levelID() );
    updSynthStats();
}


void uiSynthToRealScale::seisSelCB( CallBacker* )
{
    seissubselfld_->setInput( seisfld_->key(true) );
}


void uiSynthToRealScale::synthSelCB( CallBacker* )
{
    updSynthStats();
}


void uiSynthToRealScale::evChgCB( CallBacker* )
{
    updSynthStats();
}


void uiSynthToRealScale::subselChgCB( CallBacker* )
{
    IOPar iop;
    seissubselfld_->fillPar( iop );
    delete seldata_;
    seldata_ = Seis::SelData::get( iop );
}


bool uiSynthToRealScale::getEvent()
{
    if ( !evfld_->getFromScreen() )
	return false;
    seisev_ = evfld_->event();
    const bool isrms = evfld_->getFullExtrWin().nrSteps() > 0;
    valislbl_->setText( (isrms ? tr("Amplitude RMS values")
			       : tr("Amplitude values")).optional() );
    return true;
}


bool uiSynthToRealScale::getHorData( TaskRunnerProvider& trprov )
{
    if ( horizon_ )
	{ horizon_->unRef(); horizon_ = 0; }

    const DBKey horid = lvlhorfld_->horID();
    if ( !horid.isValid() )
	return false;
    EM::Object* emobj = EM::MGR().loadIfNotFullyLoaded( horid, trprov );
    mDynamicCastGet(EM::Horizon*,hor,emobj);
    if ( !hor )
	return false;

    horizon_ = hor;
    horizon_->ref();
    horiter_ = horizon_->createIterator();
    return true;
}


float uiSynthToRealScale::getTrcValue( const SeisTrc& trc, float reftm ) const
{
    const int lastsamp = trc.size() - 1;
    const Interval<float> trg( trc.startPos(), trc.samplePos(lastsamp) );
    const StepInterval<float> extrwin( evfld_->getFullExtrWin() );
    const int nrtms = extrwin.nrSteps() + 1;
    const bool calcrms = nrtms > 1;
    float sumsq = 0;

    if ( evfld_->snapToEvent() )
    {
	const float snappedreftm = SeisEventSnapper::findNearestEvent(
		trc, reftm, extrwin, evfld_->event().evType() );
	if ( !mIsUdf(snappedreftm) )
	    reftm = snappedreftm;
    }

    for ( int itm=0; itm<nrtms; itm++ )
    {
	float extrtm = reftm + extrwin.atIndex( itm );
	float val;
	if ( extrtm <= trg.start )
	    val = trc.get( 0, 0 );
	if ( extrtm >= trg.stop )
	    val = trc.get( lastsamp, 0 );
	else
	    val = trc.getValue( extrtm, 0 );
	if ( calcrms ) val *= val;
	sumsq += val;
    }
    return calcrms ? Math::Sqrt( sumsq / nrtms ) : sumsq;
}


uiSynthToRealScale::SynthID uiSynthToRealScale::synthID() const
{
    const auto selidx = synthfld_ ? synthfld_->currentItem() : 0;
    return datamgr_->find( synthnms_.get(selidx) );
}


const SeisTrcBuf& uiSynthToRealScale::synthTrcBuf( SynthID synthid ) const
{
    datamgr_->ensureGenerated( synthid, uiTaskRunnerProvider(this) );
    const auto* sds = datamgr_->getDataSet( synthid );
    static SeisTrcBuf emptytb( false );
    if ( !sds )
	return emptytb;
    mDynamicCastGet( const SeisTrcBufDataPack*, tbufdp, &sds->dataPack() )
    if ( !tbufdp )
	{ pErrMsg("Huh"); return emptytb; }
    return tbufdp->trcBuf();
}


void uiSynthToRealScale::updSynthStats()
{
    const auto synthid = synthID();
    if ( !getEvent() || !synthid.isValid() )
	return;

    TypeSet<float> vals;
    const auto& synthtbuf = synthTrcBuf( synthid );
    const auto& lvl = datamgr_->levels().get( lvlhorfld_->levelID() );
    const auto& t2dmdls = datamgr_->d2TModels();
    const auto sz = synthtbuf.size();
    if ( lvl.size() != sz || t2dmdls.size() != sz )
	{ pErrMsg("Huh"); }
    for ( int iseq=0; iseq<sz; iseq++ )
    {
	const float zref = lvl.zvals_.get( iseq );
	if ( !mIsUdf(zref) )
	{
	    const auto& t2d = *t2dmdls.get( iseq );
	    const float tref = t2d.getTime( zref );
	    vals += getTrcValue( *synthtbuf.get(iseq), tref );
	}
    }

    uiHistogramDisplay& histfld = *synthstatsfld_->dispfld_;
    histfld.setData( vals.arr(), vals.size() );
    histfld.putN();
    mSetFldValue( synthstatsfld_, histfld.getStatCalc().average() );
}


class uiSynthToRealScaleRealStatCollector : public Executor
{ mODTextTranslationClass(uiSynthToRealScaleRealStatCollector);
public:
uiSynthToRealScaleRealStatCollector(
	uiSynthToRealScale& d, Seis::Provider& prov )
    : Executor( "Collect Amplitudes" )
    , dlg_(d)
    , prov_(prov)
    , msg_(uiStrings::sCollectingData())
    , nrdone_(0)
    , totalnr_(-1)
    , seldata_(d.seldata_)
{
    if ( seldata_ )
	totalnr_ = seldata_->expectedNrTraces();
    else
	totalnr_ = dlg_.horiter_->approximateSize();
}

uiString message() const	{ return msg_; }
uiString nrDoneText() const	{ return tr("Traces handled"); }
od_int64 nrDone() const		{ return nrdone_; }
od_int64 totalNr() const	{ return totalnr_; }

bool getNextPos3D()
{
    while ( true )
    {
	const EM::PosID posid = dlg_.horiter_->next();
	if ( posid.isInvalid() )
	    return false;
	const Coord3 crd = dlg_.horizon_->getPos( posid );
	if ( setBinID(crd.getXY()) )
	{
	    z_ = (float)crd.z_;
	    break;
	}
    }

    return true;
}

bool setBinID( const Coord& crd )
{
    bid_ = SI().transform( crd );
    return seldata_ ? seldata_->isOK(bid_) : true;
}


int getTrc3D()
{
    while ( true )
    {
	if ( !getNextPos3D() )
	    return Finished();

	if ( !prov_.isPresent(TrcKey(bid_)) )
	    continue;

	const uiRetVal uirv = prov_.getAt( TrcKey(bid_), trc_ );
	if ( !uirv.isOK() )
	    { msg_ = uirv; return ErrorOccurred(); }

	break;
    }
    return MoreToDo();
}

int getTrc2D()
{
    const auto curgeomid = prov_.as2D()->curGeomID();
    const TrcKey reqtk( curgeomid, bid_.crl() );
    const uiRetVal uirv = prov_.getAt( reqtk, trc_ );
    if ( !uirv.isOK() )
	{ msg_ = uirv; return ErrorOccurred(); }

    if ( !setBinID(trc_.info().coord_) )
	return MoreToDo();

    mDynamicCastGet(const EM::Horizon2D*,hor2d,dlg_.horizon_)
    if ( !hor2d )
	return ErrorOccurred();
    TrcKey tk( curgeomid, trc_.info().trcNr() );
    EM::PosID pid = hor2d->geometry().getPosID( tk );
    const Coord3 crd = dlg_.horizon_->getPos( pid );
    if ( mIsUdf(crd.z_) )
	return MoreToDo();

    z_ = (float)crd.z_;
    return MoreToDo();
}

int nextStep()
{
    const int res = dlg_.use2dseis_ ? getTrc2D() : getTrc3D();
    if ( res <= 0 )
	return res;
    nrdone_++;
    if ( res > 1 )
	return MoreToDo();

    const float val = dlg_.getTrcValue( trc_, z_ );
    if ( !mIsUdf(val) )
	vals_ += val;
    return MoreToDo();
}

    uiSynthToRealScale&	dlg_;
    Seis::Provider&	prov_;
    const Seis::SelData* seldata_;
    SeisTrc		trc_;
    uiString		msg_;
    od_int64		nrdone_;
    od_int64		totalnr_;
    BinID		bid_;
    float		z_;
    TypeSet<float>	vals_;

};


void uiSynthToRealScale::updRealStats()
{
    if ( !getEvent() )
	return;
    const auto* seisioobj = seisfld_->ioobj();
    if ( !seisioobj )
	return;
    uiTaskRunnerProvider trprov( this );
    if ( !getHorData(trprov) )
	return;

    if ( use2dseis_ )
    {
	const auto horid = lvlhorfld_->horID();
	if ( !horid.isValid() )
	    return;

	EM::IOObjInfo eminfo( horid );
	SeisIOObjInfo seisinfo( seisioobj );
	BufferStringSet seislnms, horlnms;
	seisinfo.getLineNames( seislnms );
	eminfo.getLineNames( horlnms );
	bool selectionvalid = false;
	for ( int lidx=0; lidx<seislnms.size(); lidx++ )
	{
	    if ( horlnms.isPresent(seislnms.get(lidx)) )
	    {
		selectionvalid = true;
		break;
	    }
	}

	if ( !selectionvalid )
	    mErrRet(tr("No common line names found in horizon & seismic data"))
    }

    uiRetVal uirv;
    PtrMan<Seis::Provider> prov = Seis::Provider::create( seisioobj->key(),
							  &uirv );
    if ( !prov )
	mErrRet( uirv );

    uiSynthToRealScaleRealStatCollector coll( *this, *prov );
    if ( !trprov.execute(coll) )
	return;

    uiHistogramDisplay& histfld = *realstatsfld_->dispfld_;
    histfld.setData( coll.vals_.arr(), coll.vals_.size() );
    histfld.putN();
    mSetFldValue( realstatsfld_, histfld.getStatCalc().average() );
    setScaleFld();
}


bool uiSynthToRealScale::acceptOK()
{
    if ( !evfld_->getFromScreen() )
	return false;

    const float scalefac = finalscalefld_->getFValue();
    if ( mIsUdf(scalefac) )
	mErrRetBool(uiStrings::phrEnter(tr("the scale factor")))

    uiRetVal retval;
    ConstRefMan<Wavelet> inpwvlt = WaveletMGR().fetch( inpwvltid_, retval );
    if ( retval.isError() )
	mErrRetBool( retval )

    RefMan<Wavelet> wvlt = inpwvlt->clone();
    wvlt->transform( 0, scalefac );
    if ( !wvltfld_->store(*wvlt) )
	return false;

    outwvltid_ = wvltfld_->key( true );
    const auto horid = lvlhorfld_->horID();
    const auto seisid = seisfld_->key( true );
    WaveletMGR().setScalingInfo( outwvltid_, &inpwvltid_, &horid, &seisid,
				 lvlhorfld_->levelID().name() );
    return true;
}
