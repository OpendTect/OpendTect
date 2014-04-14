/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistratsynthcrossplot.h"
#include "uistratsynthdisp.h"
#include "uistratlayseqattrsetbuild.h"
#include "uistratseisevent.h"
#include "uiattribsetbuild.h"
#include "uidatapointset.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "syntheticdata.h"
#include "stratlevel.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratlayseqattrib.h"
#include "stratlayseqattribcalc.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribprocessor.h"
#include "commondefs.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "datacoldef.h"
#include "prestackgather.h"
#include "seisbuf.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "survinfo.h"
#include "velocitycalc.h"
#include "valseriesevent.h"



uiStratSynthCrossplot::uiStratSynthCrossplot( uiParent* p,
			const Strat::LayerModel& lm,
			const ObjectSet<SyntheticData>& synths )
    : uiDialog(p,Setup("Layer model/synthetics cross-plotting",
			mNoDlgTitle,"110.3.0"))
    , lm_(lm)
    , synthdatas_(synths)
{
    if ( lm.isEmpty() )
	{ errmsg_ = "Input model is empty."
	    "\nYou need to generate layer models."; return; }

    TypeSet<DataPack::FullID> fids, psfids;
    for ( int idx=0; idx<synths.size(); idx++ )
    {
	const SyntheticData& sd = *synths[idx];
	if ( sd.isPS() )
	    psfids += sd.datapackid_;
	else
	    fids += sd.datapackid_;
    }
    if ( fids.isEmpty() && psfids.isEmpty() )
	{ errmsg_ = "Missing or invalid 'datapacks'."
	    "\nMost likely, no synthetics are available."; return; }

    uiAttribDescSetBuild::Setup bsu( true );
    bsu.showdepthonlyattrs(false).showusingtrcpos(true).showps( psfids.size() );
    seisattrfld_ = new uiAttribDescSetBuild( this, bsu );
    seisattrfld_->setDataPackInp( fids, false );
    seisattrfld_->setDataPackInp( psfids, true );

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


#define mErrRet(s) { uiMSG().error(s); delete dps; dps = 0; return dps; }
#define mpErrRet(s) { pErrMsg(s); delete dps; dps = 0; return dps; }

DataPointSet* uiStratSynthCrossplot::getData( const Attrib::DescSet& seisattrs,
					const Strat::LaySeqAttribSet& seqattrs,
					const Strat::Level& lvl,
					const Interval<float>& extrwin,
					float zstep,
					const Strat::Level* stoplvl )
{
    //If default Desc(s) present remove them
    for ( int idx=seisattrs.size()-1; idx>=0; idx-- )
    {
	const Attrib::Desc* tmpdesc = seisattrs.desc(idx);
	if ( tmpdesc && tmpdesc->isStored() && !tmpdesc->isStoredInMem() )
	    const_cast<Attrib::DescSet*>(&seisattrs)->removeDesc(tmpdesc->id());
    }

    DataPointSet* dps = seisattrs.createDataPointSet(Attrib::DescSetup(),false);
    if ( !dps )
	{ uiMSG().error(seisattrs.errMsg()); return 0; }

    if ( dps->nrCols() )
    {
	dps->dataSet().insert(dps->nrFixedCols(),new DataColDef(sKey::Depth()));
	dps->dataSet().insert( dps->nrFixedCols()+1,
		    new DataColDef(Strat::LayModAttribCalc::sKeyModelIdx()) );
    }
    else
    {
	dps->dataSet().add( new DataColDef(sKey::Depth()) );
	dps->dataSet().add(
		    new DataColDef(Strat::LayModAttribCalc::sKeyModelIdx()) );
    }

    for ( int iattr=0; iattr<seqattrs.size(); iattr++ )
	dps->dataSet().add(
		new DataColDef(seqattrs.attr(iattr).name(),toString(iattr)) );

    for ( int isynth=0; isynth<synthdatas_.size(); isynth++ )
    {
	const SyntheticData& sd = *synthdatas_[isynth];
	const ObjectSet<const TimeDepthModel>& d2tmodels = sd.d2tmodels_;
	const int nrmdls = d2tmodels.size();

	mDynamicCastGet(const SeisTrcBufDataPack*,tbpack,&sd.getPack());
	if ( !tbpack ) continue;

	const SeisTrcBuf& tbuf = tbpack->trcBuf();
	if ( tbuf.size() != nrmdls )
	    mpErrRet( "DataPack nr of traces != nr of d2t models" )

	if ( isynth > 0 )
	    continue;

	const Strat::SeisEvent& ssev = evfld_->event();
	for ( int imod=0; imod<nrmdls; imod++ )
	{
	    SeisTrc& trc = const_cast<SeisTrc&>( *tbuf.get( imod ) );
	    if ( !d2tmodels[imod] )
		mpErrRet( "DataPack does not have a TD model" )

	    float dpth = lm_.sequence(imod).depthPositionOf( lvl );
	    trc.info().pick = d2tmodels[imod]->getTime( dpth );
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

	    if ( evfld_->doAllLayers() )
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
    }

    dps->dataChanged();

    if ( dps->isEmpty() )
	mErrRet( "No positions for data extraction" )

    if ( !seisattrs.isEmpty() && !extractSeisAttribs(*dps,seisattrs) )
	mErrRet( "Could not extract any seismic attribute" )

    if ( !seqattrs.isEmpty() && !extractLayerAttribs(*dps,seqattrs,stoplvl) )
	mErrRet( "Could not extract any layer attribute" );

    if ( !extractModelNr(*dps) )
	uiMSG().warning( "Could not extract the model numbers" );

    return dps;
}


void uiStratSynthCrossplot::fillPosFromZSampling( DataPointSet& dps,
					       const TimeDepthModel& d2tmodel,
					       const SeisTrcInfo& trcinfo,
					       float step, float maxtwt,
					       const Interval<float>& extrwin )
{
    if ( mIsUdf(step) )
	uiMSG().error( "No valid step provided for data extraction");

    const float halfstep = step / 2.f;
    const int trcnr = trcinfo.nr;
    const Coord trcpos = trcinfo.coord;
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
	dr.pos_.nr_ = trcnr;
	dr.pos_.set( trcpos );
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
						const TimeDepthModel& d2tmodel,
						const SeisTrcInfo& trcinfo,
						const Interval<float>& extrwin,
						int iseq )
{
    Strat::LayerSequence subseq;
    lm_.sequence( iseq ).getSequencePart( extrwin, true, subseq );

    if ( subseq.isEmpty() )
	return;

    const int trcnr = trcinfo.nr;
    const Coord trcpos = trcinfo.coord;
    const int depthidx = dps.indexOf( sKey::Depth() );
    const int nrcols = dps.nrCols();
    float dah = subseq.startDepth();
    for ( int ilay=0; ilay<subseq.size(); ilay++ )
    {
	const float laythickness = subseq.layers()[ilay]->thickness();
	DataPointSet::DataRow dr;
	dr.data_.setSize( nrcols, mUdf(float) );
	dr.pos_.nr_ = trcnr;
	dr.pos_.set( trcpos );
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


bool uiStratSynthCrossplot::extractSeisAttribs( DataPointSet& dps,
						const Attrib::DescSet& attrs )
{
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
    uiTaskRunner tr( this );
    return TaskRunner::execute( &tr, lmac );
}


bool uiStratSynthCrossplot::launchCrossPlot( const DataPointSet& dps,
					     const Strat::Level& lvl,
					     const Strat::Level* stoplvl,
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

    BufferString wintitl( "Attributes " );
    BufferString timegate( "[", toString(winms.start,0), "-" );
    timegate.add( toString(winms.stop,0) ).add( "]" ).add( "ms" );

    if ( !multiz )
    {
	if ( !shiftedsingez )
	    wintitl.add( "at " );
	else
	{
	    wintitl.add( fabs( winms.start ) ).add( "ms " )
		   .add( winms.start < 0 ? "above " : "below " );
	}
    }
    else if ( !layerbased && !extrwin.isUdf() )
    {
	wintitl.add( "in a time gate " ).add( timegate ).add( " relative to " );
    }
    else
    {
	wintitl.add( "between " );
    }

    wintitl.add( lvl.name() );
    if ( stoplvl )
    {
	wintitl.add( !extrwin.isUdf() && !layerbased ? " and down to "
						     : " and " );
	wintitl.add( stoplvl->name() );
    }

    if ( multiz && !layerbased )
    {
	wintitl.add( " each " ).add( winms.step ).add( "ms" );
    }
    else if ( layerbased && !extrwin.isUdf() )
    {
	wintitl.add( " within " ).add( timegate );
    }

    if ( layerbased )
	wintitl.add( " - layer-based extraction" );

    uiDataPointSet::Setup su( wintitl, false );
    uiDataPointSet* uidps = new uiDataPointSet( this, dps, su, 0 );
    uidps->showXY( false );
    seisattrfld_->descSet().fillPar( uidps->storePars() );
    uidps->show();
    return false;
}


Attrib::EngineMan* uiStratSynthCrossplot::createEngineMan(
					    const Attrib::DescSet& attrs ) const
{
    Attrib::EngineMan* aem = new Attrib::EngineMan;

    TypeSet<Attrib::SelSpec> attribspecs;
    attrs.fillInSelSpecs( Attrib::DescSetup().hidden(false), attribspecs );

    aem->setAttribSet( &attrs );
    aem->setAttribSpecs( attribspecs );
    return aem;
}


void uiStratSynthCrossplot::setRefLevel( const char* lvlnm )
{
    evfld_->setLevel( lvlnm );
}


bool uiStratSynthCrossplot::handleUnsaved()
{
    return seisattrfld_->handleUnsaved() && layseqattrfld_->handleUnsaved();
}


bool uiStratSynthCrossplot::rejectOK( CallBacker* )
{
    return handleUnsaved();
}

#undef mErrRet
#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }

bool uiStratSynthCrossplot::acceptOK( CallBacker* )
{
    if ( !errmsg_.isEmpty() )
	return true;

    const Attrib::DescSet& seisattrs = seisattrfld_->descSet();
    const Strat::LaySeqAttribSet& seqattrs = layseqattrfld_->attribSet();
    if ( !evfld_->getFromScreen() )
	mErrRet(0)

    deepErase( extrgates_ );
    const Interval<float>& extrwin = evfld_->hasExtrWin()
				   ? evfld_->event().extrWin()
				   : Interval<float>::udf();
    const float zstep = evfld_->hasStep() ? evfld_->event().extrStep()
					  : mUdf(float);
    const Strat::Level& lvl = *evfld_->event().level();
    const Strat::Level* stoplvl = evfld_->event().downToLevel();
    DataPointSet* dps = getData( seisattrs, seqattrs, lvl, extrwin, zstep,
				 stoplvl );
    const bool res = dps ? launchCrossPlot( *dps, lvl, stoplvl, extrwin, zstep )
			 : false;

    if ( res && !handleUnsaved() )
	return false;

    return res;
}
