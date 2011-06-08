/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratsynthcrossplot.cc,v 1.27 2011-06-08 14:19:09 cvsbruno Exp $";

#include "uistratsynthcrossplot.h"
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


uiStratSynthCrossplot::uiStratSynthCrossplot( uiParent* p,
				    const DataPack::FullID& dpid,
				    const Strat::LayerModel& lm,
				    const ObjectSet<const TimeDepthModel>& d2t,
				    const DataPack::FullID& psdpid )
    : uiDialog(p,Setup("Layer model/synthetics cross-plotting",
			mNoDlgTitle,mTODOHelpID))
    , packmgrid_(DataPackMgr::getID(dpid))
    , pspackmgrid_(DataPackMgr::getID(psdpid))
    , lm_(lm)
    , d2tmodels_(d2t)
    , emptylbl_(0)
    , tbpack_(0)
{
    if ( lm.isEmpty() )
	{ emptylbl_ = new uiLabel(this,"No input data"); return; }
    DataPack* dp = DPM(packmgrid_).obtain( DataPack::getID(dpid) );
    mDynamicCastGet(SeisTrcBufDataPack*,tbdp,dp)
    if ( !tbdp )
	{ emptylbl_ = new uiLabel(this,"Missing or invalid datapack"); return; }
    tbpack_ = tbdp;

    DataPack* psdp = DPM(pspackmgrid_).obtain( DataPack::getID(psdpid) );
    mDynamicCastGet(PreStack::GatherSetDataPack*,psdp_,psdp)

    uiAttribDescSetBuild::Setup bsu( !SI().has3D() );
    bsu.showdepthonlyattrs(false).showusingtrcpos(true).showps(psdp_);
    seisattrfld_ = new uiAttribDescSetBuild( this, bsu );
    TypeSet<DataPack::FullID> fids; fids += dpid; if ( psdp ) fids += psdpid;
    seisattrfld_->setDataPackInp( fids );

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
    if( tbpack_ )
	DPM(packmgrid_).release( tbpack_->id() );
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

    const int nrmdls = d2tmodels_.size();
    SeisTrcBuf& tbuf = tbpack_->trcBuf();
    if ( tbuf.size() != nrmdls )
	{ pErrMsg("DataPack nr of traces != nr of d2t models"); return 0; }

    TypeSet<float> lvltms;
    const Strat::SeisEvent& ssev = evfld_->event();
    for ( int imod=0; imod<nrmdls; imod++ )
    {
	SeisTrc& trc = *tbuf.get( imod );
	const float dpth = lm_.sequence(imod).depthOf( lvl );
	trc.info().pick = d2tmodels_[imod]->getTime( dpth );
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
	    dr.data_[depthcol] = d2tmodels_[itrc]->getDepth( dr.pos_.z_ );
	    dps->addRow( dr );
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
    while( true )
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
    if ( emptylbl_ )
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
