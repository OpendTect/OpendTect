/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratsynthcrossplot.cc,v 1.29 2011-07-01 12:12:52 cvsbruno Exp $";

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
#include "stratlevel.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratlayseqattrib.h"
#include "stratlayseqattribcalc.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribprocessor.h"
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


uiStratSynthCrossplot::PackSynthData::~PackSynthData()
{
    const DataPack::FullID dpid = sd_.packid_;
    DataPackMgr::ID packmgrid = DataPackMgr::getID( dpid );
    DPM(packmgrid).release( pack_.id() );
}


uiStratSynthCrossplot::uiStratSynthCrossplot( uiParent* p,
			const Strat::LayerModel& lm,
			const ObjectSet<const SyntheticData>& synths)
    : uiDialog(p,Setup("Layer model/synthetics cross-plotting",
			mNoDlgTitle,mTODOHelpID))
    , lm_(lm)
{
    if ( lm.isEmpty() )
	{ errmsg_ = "Model is empty"; return;}

    TypeSet<DataPack::FullID> fids, psfids; 
    for ( int idx=0; idx<synths.size(); idx++ )
    {
	const SyntheticData& sd = *synths[idx];
	const DataPack::FullID dpid = sd.packid_;

	DataPackMgr::ID packmgrid = DataPackMgr::getID( dpid );
	DataPack* dp = DPM(packmgrid).obtain( DataPack::getID(dpid) );
	if ( !dp ) continue;

	if ( sd.isps_ )
	{
	    mDynamicCastGet(PreStack::GatherSetDataPack*,psdp,dp)
	    if ( !psdp ) continue; 
	    pssynthdatas_ += new PackSynthData( *psdp, sd );
	    psfids += dpid; 
	}
	else
	{
	    mDynamicCastGet(SeisTrcBufDataPack*,tbdp,dp)
	    if ( !tbdp ) continue;
	    synthdatas_ += new PackSynthData( *tbdp, sd );
	    fids += dpid; 
	}
    }
    if ( fids.isEmpty() && psfids.isEmpty() )
	{ errmsg_ = "Missing or invalid datapacks"; return;}

    uiAttribDescSetBuild::Setup bsu( !SI().has3D() );
    bsu.showdepthonlyattrs(false).showusingtrcpos(true).showps( psfids.size() );
    seisattrfld_ = new uiAttribDescSetBuild( this, bsu );
    seisattrfld_->setDataPackInp( fids, false );
    seisattrfld_->setDataPackInp( psfids, true );

    uiSeparator* sep = new uiSeparator( this, "sep1", true );
    sep->attach( stretchedBelow, seisattrfld_ );

    layseqattrfld_ = new uiStratLaySeqAttribSetBuild( this, lm_ );
    layseqattrfld_->attach( alignedWith, seisattrfld_ );
    layseqattrfld_->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "sep2" );
    sep->attach( stretchedBelow, layseqattrfld_ );

    evfld_ = new uiStratSeisEvent( this, uiStratSeisEvent::Setup(true) );
    evfld_->attach( alignedWith, layseqattrfld_ );
    evfld_->attach( ensureBelow, sep );
}


uiStratSynthCrossplot::~uiStratSynthCrossplot()
{
    deepErase( synthdatas_ ); deepErase( pssynthdatas_ );
}


DataPointSet* uiStratSynthCrossplot::getData( const Attrib::DescSet& seisattrs,
					const Strat::LaySeqAttribSet& seqattrs,
					const Strat::Level& lvl,
					const StepInterval<float>& extrwin )
{
    DataPointSet* dps = seisattrs.createDataPointSet(Attrib::DescSetup());
    if ( !dps )
	{ uiMSG().error(seisattrs.errMsg()); return false; }
    dps->dataSet().add( new DataColDef(sKey::Depth) );
    const int depthcol = dps->nrCols() - 1;
    for ( int iattr=0; iattr<seqattrs.size(); iattr++ )
	dps->dataSet().add(
		new DataColDef(seqattrs.attr(iattr).name(),toString(iattr)) );

    for ( int idx=0; idx<synthdatas_.size(); idx++ )
    {
	const PackSynthData& psd = *synthdatas_[idx];
	const ObjectSet<const TimeDepthModel>& d2tmodels = psd.sd_.d2tmodels_;
	const int nrmdls = psd.sd_.d2tmodels_.size();

	SeisTrcBufDataPack& tbpack 
			    = dynamic_cast<SeisTrcBufDataPack&>( psd.pack_ );
	SeisTrcBuf& tbuf = tbpack.trcBuf();
	if ( tbuf.size() != nrmdls )
	    { pErrMsg("DataPack nr of traces != nr of d2t models"); return 0; }

	TypeSet<float> lvltms;
	const Strat::SeisEvent& ssev = evfld_->event();
	for ( int imod=0; imod<nrmdls; imod++ )
	{
	    SeisTrc& trc = *tbuf.get( imod );
	    const float dpth = lm_.sequence(imod).depthOf( lvl );
	    trc.info().pick = d2tmodels[imod]->getTime( dpth );
	    lvltms += ssev.snappedTime( trc );
	}

	const int nrextr = extrwin.nrSteps() + 1;
	for ( int iextr=0; iextr<nrextr; iextr++ )
	{
	    const float relz = extrwin.atIndex( iextr );
	    for ( int itrc=0; itrc<nrmdls; itrc++ )
	    {
		const SeisTrc& trc = *tbuf.get( itrc );
		DataPointSet::DataRow dr;
		dr.pos_.nr_ = trc.info().nr;
		dr.pos_.set( trc.info().coord );
		dr.pos_.z_ = lvltms[itrc] + relz;
		dr.data_.setSize( dps->nrCols(), mUdf(float) );
		dr.data_[depthcol] = d2tmodels[itrc]->getDepth( dr.pos_.z_ );
		dps->addRow( dr );
	    }
	}
    }

    for ( int idx=0; idx<pssynthdatas_.size(); idx++ )
    {
	const PackSynthData& psd = *pssynthdatas_[idx];
	PreStack::GatherSetDataPack& pspack 
		    = dynamic_cast<PreStack::GatherSetDataPack&>( psd.pack_ );

	TypeSet<float> lvltms;
	const Strat::SeisEvent& ssev = evfld_->event();
	const ObjectSet<const TimeDepthModel>& d2tmodels = psd.sd_.d2tmodels_;
	const int nrmdls = psd.sd_.d2tmodels_.size();

	const int nrextr = extrwin.nrSteps() + 1;
	for ( int iextr=0; iextr<nrextr; iextr++ )
	{
	    const float relz = extrwin.atIndex( iextr );
	    for ( int itrc=0; itrc<pspack.getGathers().size(); itrc++ )
	    {
		const PreStack::Gather& gather = *pspack.getGathers()[itrc];
		DataPointSet::DataRow dr;
		dr.pos_.nr_ = itrc+1;
		dr.pos_.set( gather.getCoord() );
		dr.pos_.z_ = lvltms[itrc] + relz;
		dr.data_.setSize( dps->nrCols(), mUdf(float) );
		dr.data_[depthcol] = d2tmodels[itrc]->getDepth( dr.pos_.z_ );
		dps->addRow( dr );
	    }
	}
    }

    dps->dataChanged();

    if ( dps->isEmpty() )
    {
	uiMSG().error( "No positions for data extraction" );
	delete dps; dps = 0;
    }
    else if ( !extractSeisAttribs(*dps,seisattrs)
	    || !extractLayerAttribs(*dps,seqattrs) )
	{ delete dps; dps = 0; }

    return dps;
}


bool uiStratSynthCrossplot::extractSeisAttribs( DataPointSet& dps,
						const Attrib::DescSet& attrs )
{
    BufferString errmsg;                                                        
    PtrMan<Attrib::EngineMan> aem = createEngineMan( attrs );

    PtrMan<Executor> exec = aem->getTableExtractor( dps, attrs, errmsg,0,false);
    if ( !exec )                                                                
    {                                                                           
	uiMSG().error( errmsg );                                                
	return false;
    }

    exec->setName( "Attributes from Traces" );                       
    uiTaskRunner dlg( this );                                                   
    dlg.execute(*exec);
    return true;
}


bool uiStratSynthCrossplot::extractLayerAttribs( DataPointSet& dps,
				     const Strat::LaySeqAttribSet& seqattrs )
{
    Strat::LayModAttribCalc lmac( lm_, seqattrs, dps );
    uiTaskRunner tr( this );
    return tr.execute( lmac );
}


bool uiStratSynthCrossplot::launchCrossPlot( const DataPointSet& dps,
					const Strat::Level& lvl,
					const StepInterval<float>& extrwin )
{
    BufferString wintitl( "Attributes at ", lvl.name() );
    wintitl.add( " [" ).add( extrwin.start )
	   .add( "-" ).add( extrwin.stop ).add( "]" );
    uiDataPointSet::Setup su( wintitl, false );
    uiDataPointSet* uidps = new uiDataPointSet( this, dps, su, 0 );
    seisattrfld_->descSet().fillPar( uidps->storePars() );
    uidps->setDeleteOnClose( true );
    uidps->show();
    return false;
}


Attrib::EngineMan* uiStratSynthCrossplot::createEngineMan(
					    const Attrib::DescSet& attrs ) const
{                                                                               
    Attrib::EngineMan* aem = new Attrib::EngineMan;

    //If default Desc(s) present remove it
    int idx = -1;
    while( true && idx < attrs.size()-1 )
    {
	idx++;
	const Attrib::Desc* tmpdesc = attrs.desc(idx);
	if ( tmpdesc && tmpdesc->isStored() && !tmpdesc->isStoredInMem() )
	    const_cast<Attrib::DescSet*>(&attrs)->removeDesc( tmpdesc->id() );
	else
	    break;
    }

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


#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }

bool uiStratSynthCrossplot::acceptOK( CallBacker* )
{
    if ( !errmsg_.isEmpty() )
	return true;

    const Attrib::DescSet& seisattrs = seisattrfld_->descSet();
    if ( seisattrs.isEmpty() )
	mErrRet("Please define a seismic attribute")
    const Strat::LaySeqAttribSet& seqattrs = layseqattrfld_->attribSet();
    if ( seqattrs.isEmpty() )
	mErrRet("Please define a layer attribute")
    if ( !evfld_->getFromScreen() )
	mErrRet(0)

    const StepInterval<float>& extrwin = evfld_->event().extrwin_;
    const Strat::Level& lvl = *evfld_->event().level_;
    DataPointSet* dps = getData( seisattrs, seqattrs, lvl, extrwin );
    return dps ? launchCrossPlot( *dps, lvl, extrwin ) : false;
}
