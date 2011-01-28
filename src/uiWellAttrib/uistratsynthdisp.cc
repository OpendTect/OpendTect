/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratsynthdisp.cc,v 1.17 2011-01-28 11:09:51 cvsbert Exp $";

#include "uistratsynthdisp.h"
#include "uiseiswvltsel.h"
#include "uicombobox.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "uimsg.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "wavelet.h"
#include "synthseis.h"
#include "aimodel.h"
#include "ptrman.h"
#include "ioman.h"
#include "survinfo.h"
#include "flatposdata.h"
#include "flatviewzoommgr.h"

static const int cMarkerSize = 6;


Notifier<uiStratSynthDisp>& uiStratSynthDisp::fieldsCreated()
{
    static Notifier<uiStratSynthDisp> FieldsCreated(0);
    return FieldsCreated;
}


uiStratSynthDisp::uiStratSynthDisp( uiParent* p, const Strat::LayerModel& lm )
    : uiGroup(p,"LayerModel synthetics display")
    , wvlt_(0)
    , lm_(lm)
    , wvltChanged(this)
    , zoomChanged(this)
    , longestaimdl_(0)
    , lasttool_(0)
{
    topgrp_ = new uiGroup( this, "Top group" );
    topgrp_->setFrame( true );
    topgrp_->setStretch( 2, 0 );
    wvltfld_ = new uiSeisWaveletSel( topgrp_ );
    wvltfld_->newSelection.notify( mCB(this,uiStratSynthDisp,wvltChg) );
    wvltfld_->setFrame( false );

    vwr_ = new uiFlatViewer( this );
    vwr_->setInitialSize( uiSize(500,250) ); //TODO get hor sz from laymod disp
    vwr_->setStretch( 2, 2 );
    vwr_->attach( ensureBelow, topgrp_ );
    FlatView::Appearance& app = vwr_->appearance();
    app.setGeoDefaults( true );
    app.setDarkBG( false );
    app.annot_.title_.setEmpty();
    app.annot_.x1_.showAll( true );
    app.annot_.x2_.showAll( true );
    app.annot_.x2_.name_ = "TWT (s)";
    app.ddpars_.show( true, false );
    app.ddpars_.wva_.symmidvalue_ = app.ddpars_.vd_.symmidvalue_ = 0;

    uiFlatViewStdControl::Setup fvsu( this );
    fvsu.withwva( true ).withthumbnail( false ).withcoltabed( false )
	.tba( (int)uiToolBar::Right );
    uiFlatViewStdControl* ctrl = new uiFlatViewStdControl( *vwr_, fvsu );
    ctrl->zoomChanged.notify( mCB(this,uiStratSynthDisp,zoomChg) );
}


uiStratSynthDisp::~uiStratSynthDisp()
{
    delete wvlt_;
    deepErase( aimdls_ );
}


void uiStratSynthDisp::addTool( const uiToolButtonSetup& bsu )
{
    uiToolButton* tb = new uiToolButton( topgrp_, bsu );
    if ( lasttool_ )
	tb->attach( leftOf, lasttool_ );
    else
	tb->attach( rightBorder );

    tb->attach( ensureRightOf, wvltfld_ );
    lasttool_ = tb;
}


void uiStratSynthDisp::setDispMrkrs( const TypeSet<float>& zvals, Color col )
{
    FlatView::Annotation& ann = vwr_->appearance().annot_;
    deepErase( ann.auxdata_ );

    if ( !aimdls_.isEmpty() && !zvals.isEmpty() )
    {
	FlatView::Annotation::AuxData* auxd =
			new FlatView::Annotation::AuxData("Level markers");
	auxd->linestyle_.type_ = LineStyle::None;
	for ( int imdl=0; imdl<aimdls_.size(); imdl++ )
	{
	    float tval = zvals[ imdl>=zvals.size() ? zvals.size()-1 :imdl ];
	    if ( !mIsUdf(tval) )
	    {
		tval = aimdls_[imdl]->convertTo( tval, AIModel::TWT );

		auxd->markerstyles_ += MarkerStyle2D( MarkerStyle2D::Target,
						      cMarkerSize, col );
		auxd->poly_ += FlatView::Point( imdl+1, tval );
	    }
	}
	if ( auxd->isEmpty() )
	    delete auxd;
	else
	    ann.auxdata_ += auxd;
    }

    vwr_->handleChange( FlatView::Viewer::Annot, true );
}


void uiStratSynthDisp::wvltChg( CallBacker* )
{
    modelChanged();
    wvltChanged.trigger();
}


void uiStratSynthDisp::zoomChg( CallBacker* )
{
    zoomChanged.trigger();
}


const uiWorldRect& uiStratSynthDisp::curView( bool indpth ) const
{
    static uiWorldRect wr; wr = vwr_->curView();
    if ( indpth && !aimdls_.isEmpty() )
    {
	int mdlidx = longestaimdl_;
	if ( mdlidx >= aimdls_.size() )
	    mdlidx = aimdls_.size()-1;

	const AIModel& aimdl = *aimdls_[mdlidx];
	wr.setTop( aimdl.convertTo(wr.top(),AIModel::Depth) );
	wr.setBottom( aimdl.convertTo(wr.bottom(),AIModel::Depth) );
    }
    return wr;
}


DataPack::FullID uiStratSynthDisp::packID() const
{
    const FlatDataPack* dp = vwr_->pack( true );
    if ( !dp ) dp = vwr_->pack( false );
    if ( !dp ) return DataPack::cNoID();
    return DataPack::FullID( DataPackMgr::FlatID(), dp->id() );
}


int uiStratSynthDisp::getVelIdx( bool& isvel ) const
{
    //TODO this requires a lot of work. Can be auto-detected form property
    // StdType but sometimes user has many velocity providers:
    // - Many versions (different measurements, sources, etc)
    // - Sonic vs velocity
    isvel = true; return 1; // This is what the simple generator generates
}


int uiStratSynthDisp::getDenIdx( bool& isden ) const
{
    //TODO support:
    // - density itself
    // - den = ai / vel
    isden = true; return 2; // This is what the simple generator generates
}


#define mErrRet(s) { if ( s ) uiMSG().error(s); return; }

void uiStratSynthDisp::modelChanged()
{
    MouseCursorChanger mcs( MouseCursor::Busy );
    vwr_->clearAllPacks(); vwr_->setNoViewDone();
    vwr_->control()->zoomMgr().toStart();
    deepErase( vwr_->appearance().annot_.auxdata_ );

    deepErase( aimdls_ );
    delete wvlt_;
    wvlt_ = wvltfld_->getWavelet();
    if ( !wvlt_ )
    {
	const char* nm = wvltfld_->getName();
	if ( nm && *nm )
	    mErrRet("Cannot read chosen wavelet")
	else
	    mErrRet(0)
    }

    bool isvel; const int velidx = getVelIdx( isvel );
    bool isden; const int denidx = getDenIdx( isden );
    int maxsz = 0; SamplingData<float> sd;
    longestaimdl_ = 0; int maxaimdlsz = 0;
    for ( int iseq=0; iseq<lm_.size(); iseq++ )
    {
	const Strat::LayerSequence& seq = lm_.sequence( iseq );
	AIModel* aimod = seq.getAIModel( velidx, denidx, isvel, isden );
	aimdls_ += aimod;
	if ( aimod->nrTimes() > maxaimdlsz )
	    { maxaimdlsz = aimod->nrTimes(); longestaimdl_ = iseq; }
	int sz;
	sd = Seis::SynthGenerator::getDefOutSampling( *aimod, *wvlt_, sz );
	if ( sz > maxsz ) maxsz = sz;
    }
    const int nraimdls = aimdls_.size();
    if ( nraimdls < 1 || maxsz < 2 )
	mErrRet(0)

    Seis::SynthGenerator synthgen( *wvlt_ );
    synthgen.setOutSampling( sd, maxsz );
    SeisTrcBuf* tbuf = new SeisTrcBuf( true );
    const int crlstep = SI().crlStep();
    const BinID bid0( SI().inlRange(false).stop + SI().inlStep(),
	    	      SI().crlRange(false).stop + crlstep );

    for ( int imdl=0; imdl<nraimdls; imdl++ )
    {
	synthgen.generate( *aimdls_[imdl] );
	SeisTrc* newtrc = new SeisTrc( synthgen.result() );
	const int trcnr = imdl + 1;
	newtrc->info().nr = trcnr;
	newtrc->info().binid = BinID( bid0.inl, bid0.crl + imdl * crlstep );
	newtrc->info().coord = SI().transform( newtrc->info().binid );
	tbuf->add( newtrc );
    }

    deepErase( vwr_->appearance().annot_.auxdata_ );
    SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( tbuf, Seis::Line,
	    			SeisTrcInfo::TrcNr, "Seismic" );
    dp->setName( "Model synthetics" );
    dp->posData().setRange( true, StepInterval<double>(1,nraimdls,1) );
    const SeisTrc& trc0 = *tbuf->get(0);
    StepInterval<double> zrg( trc0.info().sampling.start,
			      trc0.info().sampling.atIndex(trc0.size()-1),
			      trc0.info().sampling.step );
    dp->posData().setRange( false, zrg );
    DPM(DataPackMgr::FlatID()).add( dp );
    vwr_->setPack( true, dp->id(), false );
    vwr_->setPack( false, dp->id(), false );
}
