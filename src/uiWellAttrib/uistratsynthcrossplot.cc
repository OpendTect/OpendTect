/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratsynthcrossplot.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribparam.h"
#include "commondefs.h"
#include "datacoldef.h"
#include "od_helpids.h"
#include "posvecdataset.h"
#include "prestackattrib.h"
#include "prestackgather.h"
#include "seistrc.h"
#include "stratlayer.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratlayseqattrib.h"
#include "stratlayseqattribcalc.h"
#include "stratsynth.h"
#include "survinfo.h"
#include "syntheticdataimpl.h"
#include "timedepthmodel.h"

#include "uiattribsetbuild.h"
#include "uidatapointset.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistratlayseqattrsetbuild.h"
#include "uistratseisevent.h"
#include "uitaskrunner.h"

/*Adhoc arrangement for od6.0, later versions should use the issynth parameter
  in uiAttribDescSetBuild::Setup*/

static const char* unwantedattribnms[] =
{
    "Horizon",
    "Log",
    "Curvature",
    "Curvature Gradient",
    "Dip steered median filter",
    "Perpendicular dip extractor",
    "Dip",
    "Dip Angle",
    "FaultDip",
    "FingerPrint",
    "Fracture Attributes",
    "HorizonCube Data",
    "HorizonCube Density",
    "HorizonCube Dip",
    "HorizonCube Layer",
    "HorizonCube Thickness",
    "Systems Tract",
    "EW2DErrorGrid",
    "EW3DModelBuilder",
    "EWDeterministicInversion",
    "EWStochasticInversion",
    "EWUtilities",
    "Fingerprint",
    "Fracture Attributes",
    "Texture",
    "Texture - Directional",
    nullptr
};


class uiStratSynthAttribSetBuild : public uiAttribDescSetBuild
{
public:
uiStratSynthAttribSetBuild( uiParent* p, uiAttribDescSetBuild::Setup& setup )
    : uiAttribDescSetBuild(p,setup)
{
    BufferStringSet removeattrnms( unwantedattribnms );
    for ( int idx=removeattrnms.size()-1; idx>=0; idx-- )
    {
	const char* attrnm = removeattrnms.get(idx).buf();
	if ( avfld_->isPresent(attrnm) )
	    avfld_->removeItem( attrnm );
    }
}


};


uiStratSynthCrossplot::uiStratSynthCrossplot( uiParent* p,
					      const StratSynth::DataMgr& dm )
    : uiDialog(p,Setup(tr("Layer model/synthetics cross-plotting"),
			mNoDlgTitle, mODHelpKey(mStratSynthCrossplotHelpID) ))
    , synthmgr_(dm.getProdMgr())
    , lm_(dm.layerModel())
{
    const bool emptymodel = synthmgr_->layerModel().isEmpty();
    TypeSet<SynthID> ids;
    if ( !emptymodel )
	synthmgr_->getIDs( ids );
    if ( emptymodel || ids.isEmpty() )
    {
	new uiLabel( this, tr("No input.\n"
			"Please generate geology/geophysics first") );
	return;
    }

	// It's a pity we cannot do work without generating all datasets because
	// we need the generated DataPack IDs for the Attribute Set editor ...

    TypeSet<DataPack::FullID> dpids, psdpids;
    uiTaskRunner trprov( this );
    for ( const auto& id : ids )
    {
	if ( !synthmgr_->ensureGenerated(id,&trprov) )
	    continue;

	ConstRefMan<SyntheticData> sd = synthmgr_->getDataSet( id );
	if ( sd && sd->nrPositions() > 1 )
	    (sd->isPS() ? psdpids : dpids).add( sd->fullID() );
    }

    if ( dpids.isEmpty() && psdpids.isEmpty() )
    {
	new uiLabel( this, tr("Missing or invalid 'datapacks'."
		    "\nMost likely, no synthetics are available.") );
	return;
    }

    uiAttribDescSetBuild::Setup bsu( true );
    bsu.showdepthonlyattrs(false)
       .showusingtrcpos(true)
       .showps( !psdpids.isEmpty() )
       .showhidden(false)
       .showsteering(false);
    seisattrfld_ = new uiStratSynthAttribSetBuild( this, bsu );
    seisattrfld_->setDataPackInp( dpids, false );
    seisattrfld_->setDataPackInp( psdpids, true );

    auto* sep = new uiSeparator( this, "sep1" );
    sep->attach( stretchedBelow, seisattrfld_ );

    layseqattrfld_ = new uiStratLaySeqAttribSetBuild( this, lm_ );
    layseqattrfld_->attach( alignedWith, seisattrfld_ );
    layseqattrfld_->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "sep2" );
    sep->attach( stretchedBelow, layseqattrfld_ );

    evfld_ = new uiStratSeisEvent( this,
			uiStratSeisEvent::Setup(true).allowlayerbased(true) );
    evfld_->attach( alignedWith, layseqattrfld_ );
    evfld_->attach( ensureBelow, sep );
}


uiStratSynthCrossplot::~uiStratSynthCrossplot()
{
    detachAllNotifiers();
    deepErase( extrgates_ );
}


#define mErrRet(s) { uiMSG().error(s); dps = nullptr; return dps; }
#define mpErrRet(s) { pErrMsg(s); dps = nullptr; return dps; }

RefMan<DataPointSet> uiStratSynthCrossplot::getData(
					const Attrib::DescSet& seisattrs,
					const Strat::LaySeqAttribSet& seqattrs,
					const Strat::Level& lvl,
					const ZGate& extrwin, float zstep,
					const Strat::Level* stoplvl )
{
    //If default Desc(s) present remove them
    for ( int idx=seisattrs.size()-1; idx>=0; idx-- )
    {
	const Attrib::Desc* tmpdesc = seisattrs.desc(idx);
	if ( tmpdesc && tmpdesc->isStored() && !tmpdesc->isStoredInMem() )
	    const_cast<Attrib::DescSet*>(&seisattrs)->removeDesc(tmpdesc->id());
    }

    bool hasintegrated = false;
    for ( const auto* seqattr : seqattrs )
    {
	if ( !seqattr->islocal_ )
	{
	    hasintegrated = true;
	    break;
	}
    }

    RefMan<DataPointSet> dps =
		seisattrs.createDataPointSet(Attrib::DescSetup(),false);
    if ( !dps )
	{ uiMSG().error(seisattrs.errMsg()); return nullptr; }

    PosVecDataSet& pvds = dps->dataSet();
    const UnitOfMeasure* depunit = PropertyRef::thickness().unit();
    if ( dps->nrCols() )
    {
	pvds.insert( dps->nrFixedCols(),
		     new DataColDef( sKey::Depth(), nullptr, depunit ) );
	pvds.insert( dps->nrFixedCols()+1,
		     new DataColDef(Strat::LayModAttribCalc::sKeyModelIdx()) );
    }
    else
    {
	pvds.add( new DataColDef( sKey::Depth(), nullptr, depunit ) );
	pvds.add( new DataColDef(Strat::LayModAttribCalc::sKeyModelIdx()) );
    }

    int iattr = 0;
    for ( const auto* seqattr : seqattrs )
    {
	const UnitOfMeasure* uom = seqattr->prop_.unit();
	pvds.add( new DataColDef(seqattr->name(),toString(iattr++),uom) );
    }

    TypeSet<SynthID> ids;
    synthmgr_->getIDs( ids, StratSynth::DataMgr::NoSubSel, true );
    if ( ids.isEmpty() )
	mpErrRet("No valid synthetic data found");

    ConstRefMan<SyntheticData> sd = synthmgr_->getDataSet( ids.first() );
    const int nrmdls = sd->nrPositions();
    const Strat::SeisEvent& ssev = evfld_->event();
    for ( int imod=0; imod<nrmdls; imod++ )
    {
	const TimeDepthModel* tdmodel = sd->getTDModel( imod );
	if ( !tdmodel )
	    mpErrRet( "DataPack does not have a TD model" )

	SeisTrc& trc = const_cast<SeisTrc&>( *sd->getTrace(imod) );
	float dpth = lm_.sequence(imod).depthPositionOf( lvl );
	trc.info().pick = tdmodel->getTime( dpth );
	const float twt = ssev.snappedTime( trc );
	dpth = tdmodel->getDepth( twt );

	Interval<float> timerg;
	if ( !extrwin.isUdf() )
	{
	    timerg.setFrom( extrwin );
	    timerg.shift( twt );
	}
	else
	    timerg.start = twt;

	float maxdepth = mUdf(float); float maxtwt = mUdf(float);
	if ( stoplvl )
	{
	    maxdepth = lm_.sequence(imod).depthPositionOf( *stoplvl );
	    maxtwt = tdmodel->getTime( maxdepth );
	}

	if ( evfld_->layerBased() )
	{
	    ZGate depthrg( dpth, mUdf(float) );
	    depthrg.stop = extrwin.isUdf() ? maxdepth
			 : tdmodel->getDepth( timerg.stop );
	    fillPosFromLayerSampling( *dps, hasintegrated, *tdmodel,
				      trc.info(), depthrg, imod );
	}
	else
	{
	    if ( stoplvl && extrwin.isUdf() )
		timerg.stop = twt + zstep *
			mCast(float, mNINT32((maxtwt-twt)/zstep) );

	    fillPosFromZSampling( *dps, *tdmodel, trc.info(),
				  zstep, maxtwt, timerg );
	}
    }

    dps->dataChanged();

    if ( dps->isEmpty() )
	mErrRet( tr("No positions for data extraction") )

    if ( !seisattrs.isEmpty() && !extractSeisAttribs(*dps,seisattrs) )
	mErrRet( tr("Could not extract any seismic attribute") )

    if ( !seqattrs.isEmpty() && !extractLayerAttribs(*dps,seqattrs,stoplvl) )
	mErrRet( tr("Could not extract any layer attribute") );

    if ( !extractModelNr(*dps) )
	uiMSG().warning( tr("Could not extract the model numbers") );

    return dps;
}


void uiStratSynthCrossplot::fillPosFromZSampling( DataPointSet& dps,
					       const TimeDepthModel& d2tmodel,
					       const SeisTrcInfo& trcinfo,
					       float step, float maxtwt,
					       const ZGate& extrwin )
{
    if ( mIsUdf(step) )
	uiMSG().error( tr("No valid step provided for data extraction"));

    const float halfstep = step / 2.f;
    const int depthidx = dps.indexOf( sKey::Depth() );
    const int nrcols = dps.nrCols();
    const ZSampling win( extrwin.start, extrwin.stop, step );

    auto* extrgate = new TypeSet<ZGate>;
    for ( int iextr=0; iextr<win.nrSteps()+1; iextr++ )
    {
	const float twt = win.atIndex( iextr );
	if ( !mIsUdf(maxtwt) && twt > maxtwt )
	    break;

	float dah = d2tmodel.getDepth( twt );
	if ( mIsUdf(dah) )
	    continue;

	if ( SI().depthsInFeet() )
	    dah *= mToFeetFactorF;

	DataPointSet::DataRow dr;
	dr.data_.setSize( nrcols, mUdf(float) );
	dr.pos_.trckey_ = trcinfo.trcKey();
	dr.pos_.z_ = twt;
	dr.data_[depthidx] = dah;
	dps.addRow( dr );

	Interval<float> timerg( twt - halfstep, twt + halfstep );
	Interval<float> depthrg;
	depthrg.start = d2tmodel.getDepth( timerg.start );
	depthrg.stop = d2tmodel.getDepth( timerg.stop );
	*extrgate += depthrg;
    }

    extrgates_ += extrgate;
}


void uiStratSynthCrossplot::fillPosFromLayerSampling( DataPointSet& dps,
						bool hasintegrated,
						const TimeDepthModel& d2tmodel,
						const SeisTrcInfo& trcinfo,
						const ZGate& extrwin, int iseq )
{
    Strat::LayerSequence subseq;
    const Strat::LayerSequence& seqin = lm_.sequence( iseq );
    seqin.getSequencePart( extrwin, true, subseq );
    if ( subseq.isEmpty() )
    {
	if ( hasintegrated && !seqin.isEmpty() )
	{
	    auto* newlay = new Strat::Layer( *seqin.layers().first() );
	    newlay->setThickness( 0.f );
	    subseq.layers().add( newlay );
	    subseq.setStartDepth( seqin.startDepth() );
	}
	else
	    return;
    }

    const int depthidx = dps.indexOf( sKey::Depth() );
    const int nrcols = dps.nrCols();
    float dah = subseq.startDepth();
    for ( int ilay=0; ilay<subseq.size(); ilay++ )
    {
	const float laythickness = subseq.layers()[ilay]->thickness();
	DataPointSet::DataRow dr;
	dr.data_.setSize( nrcols, mUdf(float) );
	dr.pos_.trckey_ = trcinfo.trcKey();
	float depth = dah + laythickness/2.f;
	dr.pos_.z_ = d2tmodel.getTime( depth );
	if ( SI().depthsInFeet() )
	    depth *= mToFeetFactorF;

	dr.data_[depthidx] = depth;
	dps.addRow( dr );
	dah += laythickness;
    }
}


bool uiStratSynthCrossplot::extractModelNr( DataPointSet& dps ) const
{
    const int modnridx =
	dps.indexOf( Strat::LayModAttribCalc::sKeyModelIdx() );
    if ( modnridx<0 ) return false;

    for ( int dpsrid=0; dpsrid<dps.size(); dpsrid++ )
    {
	float* dpsvals = dps.getValues( dpsrid );
	dpsvals[modnridx] = mCast(float,dps.trcNr(dpsrid));
    }

    return true;
}


void uiStratSynthCrossplot::preparePreStackDescs()
{
    TypeSet<SynthID> ids;
    synthmgr_->getIDs( ids );
    TypeSet<DataPack::FullID> dpids;
    for ( const auto& id : ids )
	dpids += synthmgr_->getDataSet(id)->fullID();

    auto* ds = const_cast<Attrib::DescSet*>( &seisattrfld_->descSet() );
    for ( int dscidx=0; dscidx<ds->size(); dscidx++ )
    {
	Attrib::Desc& desc = (*ds)[dscidx];
	if ( !desc.isPS() )
	    continue;

	//TODO: obtain prestack gathers.
	mDynamicCastGet(Attrib::EnumParam*,gathertypeparam,
			desc.getValParam(Attrib::PSAttrib::gathertypeStr()))
	if ( gathertypeparam->getIntValue()==(int)(Attrib::PSAttrib::Ang) )
	{
	    mDynamicCastGet(Attrib::BoolParam*,useangleparam,
			    desc.getValParam(Attrib::PSAttrib::useangleStr()))
	    useangleparam->setValue( true );
	    uiString errmsg;
	    Attrib::Provider* attrib = Attrib::Provider::create( desc, errmsg );
	    mDynamicCastGet(Attrib::PSAttrib*,psattrib,attrib);
	    if ( !psattrib )
		continue;

	    BufferString inputpsidstr( psattrib->psID() );
	    const char* dpbuf = inputpsidstr.buf();
	    dpbuf++;
	    inputpsidstr = dpbuf;
	    DataPack::FullID inputdpid = DataPack::FullID::getFromString(
								inputpsidstr );
	    const int inpdsidx = dpids.indexOf( inputdpid );
	    ConstRefMan<SyntheticData> sd =
				       synthmgr_->getDataSetByIdx( inpdsidx );
	    mDynamicCastGet(const PreStackSyntheticData*,pssd,sd.ptr());
	    if ( !pssd )
		continue;

	    //TODO: obtain angle gathers
	    mDynamicCastGet(Attrib::IntParam*,angledpparam,
			    desc.getValParam(Attrib::PSAttrib::angleDPIDStr()))
	    angledpparam->setValue( pssd->angleData().id().asInt() );
	}
    }
}


bool uiStratSynthCrossplot::extractSeisAttribs( DataPointSet& dps,
						const Attrib::DescSet& attrs )
{
    preparePreStackDescs();

    uiString errmsg;
    PtrMan<Attrib::EngineMan> aem = createEngineMan( attrs );
    PtrMan<Executor> exec = aem->getTableExtractor(dps,attrs,errmsg,2,false);
    if ( !exec )
    {
	uiMSG().error( errmsg );
	return false;
    }

    exec->setName( "Attributes from Traces" );
    uiTaskRunner dlg( this );
    TaskRunner::execute( &dlg, *exec );
    return true;
}


bool uiStratSynthCrossplot::extractLayerAttribs( DataPointSet& dps,
				     const Strat::LaySeqAttribSet& seqattrs,
				     const Strat::Level* stoplvl )
{
    Strat::LayModAttribCalc lmac( lm_, seqattrs, dps );
    lmac.setExtrGates( extrgates_, stoplvl );
    uiTaskRunner taskrunner( this );
    return TaskRunner::execute( &taskrunner, lmac );
}


void uiStratSynthCrossplot::launchCrossPlot( const DataPointSet& dps,
					     const Strat::Level& lvl,
					     const Strat::Level* stoplvl,
					     const ZGate& extrwin, float zstep )
{
    ZSampling winms( extrwin.start, extrwin.stop, zstep );
    winms.scale( 1000.f );
    const bool layerbased = mIsUdf( zstep );
    const bool multiz = ( !extrwin.isUdf() && winms.nrSteps() > 1 )
			|| stoplvl || layerbased;
    const bool shiftedsingez = !extrwin.isUdf() && !layerbased &&
			       !mIsZero(extrwin.start,1e-6f);

    uiString wintitl = uiStrings::sAttribute(mPlural);
    uiString timegate = tr("[%1-%2]ms").arg(toUiString(winms.start,0))
				       .arg(toUiString(winms.stop,0));

    if ( !multiz )
    {
	if ( !shiftedsingez )
	    wintitl = tr( "%1 at" ).arg(wintitl);
	else
	{
	    wintitl = tr("%1 %2ms %3").arg(wintitl)
				      .arg(toUiString(fabs( winms.start )))
				      .arg(winms.start < 0 ?
				      uiStrings::sAbove().toLower() :
				      uiStrings::sBelow().toLower());
	}
    }
    else if ( !layerbased && !extrwin.isUdf() )
    {
	wintitl = tr("%1 in a time gate %2 relative to").arg(wintitl)
							.arg(timegate);
    }
    else
    {
	wintitl = tr("%1 between").arg(wintitl);
    }

    wintitl = toUiString("%1 %2").arg(wintitl).arg(lvl.name());
    if ( stoplvl )
    {
	wintitl = toUiString("%1 %2").arg(wintitl)
					.arg(!extrwin.isUdf() && !layerbased ?
					tr("and down to") : tr("and"));
	wintitl = toUiString("%1 %2").arg(wintitl)
					      .arg(stoplvl->name());
    }

    if ( multiz && !layerbased )
    {
	wintitl = tr( "%1 each %2ms").arg(wintitl).arg(winms.step);
    }
    else if ( layerbased && !extrwin.isUdf() )
    {
	wintitl = tr("%1 within %2").arg(wintitl).arg( timegate );
    }

    if ( layerbased )
	wintitl = tr("%1 - layer-based extraction").arg(wintitl);

    uiDataPointSet::Setup su( wintitl, false );
    auto* uidps = new uiDataPointSet( this, dps, su, 0 );
    uidps->showXY( false );
    auto* ds = const_cast<Attrib::DescSet*>( &seisattrfld_->descSet() );
    ds->removeUnused( false, true );


    seisattrfld_->descSet().fillPar( uidps->storePars() );
    uidps->show();
}


Attrib::EngineMan* uiStratSynthCrossplot::createEngineMan(
					    const Attrib::DescSet& attrs ) const
{
    auto* aem = new Attrib::EngineMan;

    TypeSet<Attrib::SelSpec> attribspecs;
    attrs.fillInSelSpecs( Attrib::DescSetup().hidden(false), attribspecs );

    aem->setAttribSet( &attrs );
    aem->setAttribSpecs( attribspecs );
    return aem;
}


void uiStratSynthCrossplot::setRefLevel( const Strat::LevelID& lvlid )
{
    if ( evfld_ )
	evfld_->setLevel( lvlid );
}


bool uiStratSynthCrossplot::handleUnsaved()
{
    return !seisattrfld_
	|| (seisattrfld_->handleUnsaved() && layseqattrfld_->handleUnsaved());
}


bool uiStratSynthCrossplot::rejectOK( CallBacker* )
{
    return handleUnsaved();
}

#undef mErrRet
#define mErrRet(s) { if ( !s.isEmpty() ) uiMSG().error(s); return false; }

bool uiStratSynthCrossplot::acceptOK( CallBacker* )
{
    if ( !seisattrfld_ )
	return true;

    const Attrib::DescSet& seisattrs = seisattrfld_->descSet();
    const Strat::LaySeqAttribSet& seqattrs = layseqattrfld_->attribSet();
    if ( !evfld_->getFromScreen() )
	mErrRet(uiString::empty())

    deepErase( extrgates_ );
    const Interval<float>& extrwin = evfld_->hasExtrWin()
				   ? evfld_->event().extrWin()
				   : Interval<float>::udf();
    const float zstep = evfld_->hasStep() ? evfld_->event().extrStep()
					  : mUdf(float);
    const Strat::Level lvl = Strat::LVLS().get( evfld_->event().levelID() );
    const Strat::Level downtolevel = Strat::LVLS().get(
					evfld_->event().downToLevelID() );
    const Strat::Level* stoplvl = downtolevel.id().isValid()
				? &downtolevel : nullptr;
    RefMan<DataPointSet> dps = getData( seisattrs, seqattrs, lvl, extrwin,
					zstep, stoplvl );
    if ( !dps )
	return false;

    dps->setName( "Layer model" );
    DPM(DataPackMgr::PointID()).add( dps );
    launchCrossPlot( *dps, lvl, stoplvl, extrwin, zstep );
    return false;
}
