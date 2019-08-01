/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/

#include "uistratsynthcrossplot.h"
#include "uistratsynthdisp.h"
#include "uistratsynthseltools.h"
#include "uistratlayseqattrsetbuild.h"
#include "uiattribsetbuild.h"
#include "uidatapointset.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "synthseisdataset.h"
#include "stratlevel.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratlayseqattrib.h"
#include "stratlayseqattribcalc.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribparam.h"
#include "attribengman.h"
#include "attribprocessor.h"
#include "commondefs.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "datacoldef.h"
#include "prestackattrib.h"
#include "paramsetget.h"
#include "prestackgather.h"
#include "prestacksynthdataset.h"
#include "seisbuf.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "stratsynthdatamgr.h"
#include "survinfo.h"
#include "velocitycalc.h"
#include "valseriesevent.h"
#include "od_helpids.h"



uiStratSynthCrossplot::uiStratSynthCrossplot( uiParent* p, const DataMgr& dm )
    : uiDialog(p,Setup(tr("Synthetics/Properties Cross-Plotting"),
			mNoDlgTitle, mODHelpKey(mStratSynthCrossplotHelpID) ))
    , synthmgr_(dm.getProdMgr())
    , lm_(dm.layerModel())
{
    if ( synthmgr_->layerModel().isEmpty() || synthmgr_->nrSynthetics() < 1 )
    {
	new uiLabel( this, tr("No input.\n"
		     "Please generate geology/geophysics first") );
	return;
    }

	// It's a pity we cannot do work without generating all datasets because
	// we need the generated DataPack IDs for the Attribute Set editor ...

    TypeSet<DataPack::FullID> dpids, psdpids;
    uiTaskRunnerProvider trprov( this );
    for ( int idx=0; idx<synthmgr_->nrSynthetics(); idx++ )
    {
	const auto synthid = synthmgr_->getIDByIdx( idx );
	synthmgr_->ensureGenerated( synthid, trprov );
	const SynthSeis::DataSet* sd = synthmgr_->getDataSetByIdx( idx );
	if ( sd && !sd->isEmpty() )
	{
	    const auto dpid = sd->dataPackID();
	    DPM(dpid).add( sd->dataPack() );
	    (sd->isPS() ? psdpids : dpids).add( dpid );
	}
    }

    uiAttribDescSetBuild::Setup bsu( true );
    bsu.showdepthonlyattrs(false)
	.showusingtrcpos(true)
	.showps( !psdpids.isEmpty() )
	.showhidden(false)
	.showsteering(false)
	.issynth(true);
    seisattrfld_ = new uiAttribDescSetBuild( this, bsu );
    seisattrfld_->setDataPackInp( dpids, false );
    seisattrfld_->setDataPackInp( psdpids, true );

    uiSeparator* sep = new uiSeparator( this, "sep1" );
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
    deepErase( extrgates_ );
}


#define mErrRet(s) { uiMSG().error(s); return 0; }
#define mpErrRet(s) { pErrMsg(s); return 0; }

DataPointSet* uiStratSynthCrossplot::getData( const DescSet& seisattrs,
					const LaySeqAttribSet& seqattrs,
					const Level& lvl,
					const Interval<float>& extrwin,
					float zstep,
					const Level* stoplvl )
{
    //If default Desc(s) present remove them
    for ( int idx=seisattrs.size()-1; idx>=0; idx-- )
    {
	const Attrib::Desc* tmpdesc = seisattrs.desc(idx);
	if ( tmpdesc && tmpdesc->isStored() && !tmpdesc->isStoredInMem() )
	    const_cast<DescSet*>(&seisattrs)->removeDesc(tmpdesc->id());
    }

    RefMan<DataPointSet> dps
	= seisattrs.createDataPointSet(Attrib::DescSetup(),false);
    dps->bivSet().setGeomSystem( OD::SynthGeom );

#   define mDPSIns(idx,dcdnm) \
	dps->dataSet().insert( idx, new DataColDef(dcdnm) )
#   define mDPSAdd(dcdnm) \
	dps->dataSet().add( new DataColDef(dcdnm) );

    if ( dps->nrCols() > 0 )
    {
	mDPSIns( dps->nrFixedCols(), sKey::Depth() );
	mDPSIns( dps->nrFixedCols()+1, Strat::LayModAttribCalc::sKeyModelIdx());
    }
    else
    {
	mDPSAdd( sKey::Depth() );
	mDPSAdd( Strat::LayModAttribCalc::sKeyModelIdx() );
    }

#   undef mDPSAdd
#   define mDPSAdd(dcdnm,dcdref) \
	dps->dataSet().add( new DataColDef(dcdnm,dcdref) );
    for ( int iattr=0; iattr<seqattrs.size(); iattr++ )
	mDPSAdd( seqattrs.attr(iattr).name(), toString(iattr) );

	    // create DPS rows using first poststack dataset
    for ( int isynth=0; isynth<synthmgr_->nrSynthetics(); isynth++ )
    {
	const auto& sd = *synthmgr_->getDataSetByIdx( isynth );
	if ( sd.isPS() )
	    continue;

	const auto& d2tmodels = sd.d2TModels();
	const auto nrmdls = d2tmodels.size();
	mDynamicCastGet(const SeisTrcBufDataPack*,tbpack,&sd.dataPack());
	const SeisTrcBuf& tbuf = tbpack->trcBuf();
	if ( tbuf.size() != nrmdls )
	    mpErrRet( "DataPack nr of traces != nr of d2t models" )

	const Strat::SeisEvent& ssev = evfld_->event();
	for ( int imod=0; imod<nrmdls; imod++ )
	{
	    SeisTrc& trc = const_cast<SeisTrc&>( *tbuf.get( imod ) );
	    if ( !d2tmodels[imod] )
		mpErrRet( "DataSet has a missing TD model" )

	    float dpth = lm_.sequence(imod).depthPositionOf( lvl );
	    trc.info().pick_ = d2tmodels[imod]->getTime( dpth );
	    const float twt = ssev.snappedTime( trc );
	    dpth = d2tmodels[imod]->getDepth( twt );

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
		maxtwt = d2tmodels[imod]->getTime( maxdepth );
	    }

	    if ( evfld_->layerBased() )
	    {
		Interval<float> depthrg( dpth, mUdf(float) );
		depthrg.stop = extrwin.isUdf() ? maxdepth
			     : d2tmodels[imod]->getDepth( timerg.stop );
		fillPosFromLayerSampling( *dps, *d2tmodels[imod],
					  trc.info(), depthrg, imod );
	    }
	    else
	    {
		if ( stoplvl && extrwin.isUdf() )
		    timerg.stop = twt + zstep *
			    mCast(float, mNINT32((maxtwt-twt)/zstep) );

		fillPosFromZSampling( *dps, *d2tmodels[imod], trc.info(),
				      zstep, maxtwt, timerg );
	    }
	}

	break; // rows created
    }

    dps->dataChanged();

    if ( dps->isEmpty() )
	mErrRet( tr("No positions for data extraction") )

    if ( !seisattrs.isEmpty() && !extractSeisAttribs(*dps,seisattrs) )
	mErrRet( uiStrings::phrCannotExtract(tr("any seismic attribute")) )

    if ( !seqattrs.isEmpty() && !extractLayerAttribs(*dps,seqattrs,stoplvl) )
	mErrRet( uiStrings::phrCannotExtract(tr("any layer attribute")) );

    if ( !extractModelNr(*dps) )
	uiMSG().warning( uiStrings::phrCannotExtract(tr("the model numbers")) );

    return dps.release();
}


void uiStratSynthCrossplot::fillPosFromZSampling( DataPointSet& dps,
					       const TimeDepthModel& d2tmodel,
					       const SeisTrcInfo& trcinfo,
					       float step, float maxtwt,
					       const Interval<float>& extrwin )
{
    if ( mIsUdf(step) )
	uiMSG().error( tr("No valid step provided for data extraction"));

    const float halfstep = step / 2.f;
    const int trcnr = trcinfo.trcNr();
    const Coord trcpos = trcinfo.coord_;
    const int depthidx = dps.indexOf( sKey::Depth() );
    const int nrcols = dps.nrCols();
    const StepInterval<float> win( extrwin.start, extrwin.stop, step );

    TypeSet<Interval<float> >* extrgate =  new TypeSet<Interval<float> >;
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
	dr.pos_.set( Bin2D(Pos::GeomID(),trcnr), trcpos );
	dr.pos_.setZ( twt );
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
						const TimeDepthModel& d2tmodel,
						const SeisTrcInfo& trcinfo,
						const Interval<float>& extrwin,
						int iseq )
{
    Strat::LayerSequence subseq;
    lm_.sequence( iseq ).getSequencePart( extrwin, true, subseq );
    if ( subseq.isEmpty() )
	return;

    const int trcnr = trcinfo.trcNr();
    const Coord trcpos = trcinfo.coord_;
    const int depthidx = dps.indexOf( sKey::Depth() );
    const int nrcols = dps.nrCols();
    float dah = subseq.startDepth();
    for ( int ilay=0; ilay<subseq.size(); ilay++ )
    {
	const float laythickness = subseq.layers()[ilay]->thickness();
	DataPointSet::DataRow dr;
	dr.data_.setSize( nrcols, mUdf(float) );
	dr.pos_.set( Bin2D(Pos::GeomID(),trcnr), trcpos );
	float depth = dah + laythickness/2.f;
	dr.pos_.setZ( d2tmodel.getTime(depth) );
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
    TypeSet<DataPack::FullID> dpids;
    for ( int idx=0; idx<synthmgr_->nrSynthetics(); idx++ )
	dpids += synthmgr_->getDataSetByIdx(idx)->dataPackID();

    DescSet* ds = const_cast<DescSet*>( &seisattrfld_->descSet() );
    for ( int dscidx=0; dscidx<ds->size(); dscidx++ )
    {
	Attrib::Desc& desc = ds->get( dscidx );
	if ( !desc.isPS() )
	    continue;

	mDynamicCastGet(Attrib::EnumParam*,gathertypeparam,
			desc.getValParam(Attrib::PSAttrib::gathertypeStr()))
	if ( gathertypeparam->getIntValue()==1 )
	{
	    mDynamicCastGet(Attrib::BoolParam*,useangleparam,
			    desc.getValParam(Attrib::PSAttrib::useangleStr()))
	    useangleparam->setValue( true );
	    uiRetVal uirv;
	    Attrib::Provider* attrib = Attrib::Provider::create( desc, uirv );
	    mDynamicCastGet(Attrib::PSAttrib*,psattrib,attrib);
	    if ( !psattrib )
		continue;

	    DBKey inppsky = psattrib->psID();
	    DataPack::FullID inputdpid
			    = DataPack::FullID::getFromDBKey( inppsky );
	    const int inpdsidx = dpids.indexOf( inputdpid );
	    mDynamicCastGet(const SynthSeis::PreStackDataSet*,pssd,
			    synthmgr_->getDataSetByIdx(inpdsidx) )
	    if ( !pssd )
		continue;

	    mDynamicCastGet(Attrib::IntParam*,angledpparam,
			    desc.getValParam(Attrib::PSAttrib::angleDPIDStr()))
	    angledpparam->setValue( pssd->angleData().id().getI() );
	}
    }
}


bool uiStratSynthCrossplot::extractSeisAttribs( DataPointSet& dps,
						const DescSet& attrs )
{
    preparePreStackDescs();

    uiRetVal uirv;
    PtrMan<Attrib::EngineMan> aem = createEngineMan( attrs );
    PtrMan<Executor> exec = aem->getTableExtractor(dps,attrs,uirv,2,false);
    if ( !exec )
	{ uiMSG().error( uirv ); return false; }

    exec->setName( "Attributes from Traces" );
    uiTaskRunner dlg( this );
    TaskRunner::execute( &dlg, *exec );
    return true;
}


bool uiStratSynthCrossplot::extractLayerAttribs( DataPointSet& dps,
				     const LaySeqAttribSet& seqattrs,
				     const Level* stoplvl )
{
    Strat::LayModAttribCalc lmac( lm_, seqattrs, dps );
    lmac.setExtrGates( extrgates_, stoplvl );
    uiTaskRunner taskrunner( this );
    return TaskRunner::execute( &taskrunner, lmac );
}


void uiStratSynthCrossplot::launchCrossPlot( const DataPointSet& dps,
					     const Level& lvl,
					     const Level* stoplvl,
					     const Interval<float>& extrwin,
					     float zstep )
{
    StepInterval<float> winms( extrwin.start, extrwin.stop, zstep );
    winms.scale( 1000.f );
    const bool layerbased = mIsUdf( zstep );
    const bool multiz = ( !extrwin.isUdf() && winms.nrSteps() > 1 )
			|| stoplvl || layerbased;
    const bool shiftedsingez = !extrwin.isUdf() && !layerbased &&
			       !mIsZero(extrwin.start,1e-6f);

    uiString wintitl = uiStrings::sAttribute(mPlural);
    uiString timegate = toUiString("[%1-%2]ms").arg(toUiString(winms.start,0))
				       .arg(toUiString(winms.stop,0));

    if ( !multiz )
    {
	if ( !shiftedsingez )
	    wintitl = toUiString( "%1 @ " ).arg(wintitl);
	else
	{
	    wintitl = toUiString("%1 %2ms %3").arg(wintitl)
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
			    tr("and down to") : uiStrings::sAnd().toLower() );
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
    uiDataPointSet* uidps = new uiDataPointSet( this, dps, su, 0 );
    uidps->showXY( false );
    DescSet& ds = *const_cast<DescSet*>( &seisattrfld_->descSet() );
    ds.removeUnused( false, true );

    seisattrfld_->descSet().fillPar( uidps->storePars() );
    uidps->show();
}


Attrib::EngineMan* uiStratSynthCrossplot::createEngineMan(
					    const DescSet& attrs ) const
{
    Attrib::EngineMan* aem = new Attrib::EngineMan;

    Attrib::SelSpecList attribspecs;
    attrs.fillInSelSpecs( Attrib::DescSetup().hidden(false), attribspecs );

    aem->setAttribSet( &attrs );
    aem->setAttribSpecs( attribspecs );
    return aem;
}


void uiStratSynthCrossplot::setRefLevel( const Strat::Level::ID& lvlid )
{
    if ( evfld_ )
	evfld_->setLevel( lvlid );
}


bool uiStratSynthCrossplot::handleUnsaved()
{
    return !seisattrfld_
	|| (seisattrfld_->handleUnsaved() && layseqattrfld_->handleUnsaved());
}


bool uiStratSynthCrossplot::rejectOK()
{
    return handleUnsaved();
}

#undef mErrRet
#define mErrRet(s) { if ( !s.isEmpty() ) uiMSG().error(s); return false; }

bool uiStratSynthCrossplot::acceptOK()
{
    if ( !seisattrfld_ )
	return true;

    const DescSet& seisattrs = seisattrfld_->descSet();
    const LaySeqAttribSet& seqattrs = layseqattrfld_->attribSet();
    if ( !evfld_->getFromScreen() )
	mErrRet(uiString::empty())

    deepErase( extrgates_ );
    const Interval<float>& extrwin = evfld_->hasExtrWin()
				   ? evfld_->event().extrWin()
				   : Interval<float>::udf();
    const float zstep = evfld_->hasStep() ? evfld_->event().extrStep()
					  : mUdf(float);
    const Level lvl = Strat::LVLS().get( evfld_->event().levelID() );
    const Level downtolevel = Strat::LVLS().get(
					evfld_->event().downToLevelID() );
    const Level* stoplvl = downtolevel.id().isValid() ? &downtolevel : 0;
    RefMan<DataPointSet> dps =
		getData( seisattrs, seqattrs, lvl, extrwin, zstep, stoplvl );
    if ( !dps )
	return false;

    DPM(DataPackMgr::PointID()).add( dps );
    launchCrossPlot( *dps, lvl, stoplvl, extrwin, zstep );
    return false;
}
