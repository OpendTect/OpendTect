/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratsynthdisp.cc,v 1.7 2010-12-10 14:32:33 cvsbert Exp $";

#include "uistratsynthdisp.h"
#include "uiseiswvltsel.h"
#include "uicombobox.h"
#include "uiflatviewer.h"
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


uiStratSynthDisp::uiStratSynthDisp( uiParent* p, const Strat::LayerModel& lm )
    : uiGroup(p,"LayerModel synthetics display")
    , wvlt_(0)
    , lm_(lm)
    , dispeach_(1)
{
    const CallBack redrawcb( mCB(this,uiStratSynthDisp,wvltChg) );
    uiGroup* topgrp = new uiGroup( this, "Top group" );
    wvltfld_ = new uiSeisWaveletSel( topgrp );
    wvltfld_->newSelection.notify( redrawcb );

    vwr_ = new uiFlatViewer( this );
    vwr_->setInitialSize( uiSize(500,250) ); //TODO get hor sz from laymod disp
    vwr_->setStretch( 2, 2 );
    vwr_->attach( ensureBelow, topgrp );
    FlatView::Appearance& app = vwr_->appearance();
    app.setGeoDefaults( true );
    app.setDarkBG( false );
    app.annot_.title_.setEmpty();
    app.annot_.x1_.showAll( true );
    app.annot_.x2_.showAll( true );
    app.ddpars_.show( true, false );
}


uiStratSynthDisp::~uiStratSynthDisp()
{
    delete wvlt_;
    deepErase( aimdls_ );
}


void uiStratSynthDisp::setDispEach( int nr )
{
    dispeach_ = nr;
    modelChanged();
}


void uiStratSynthDisp::setDispMrkrs( const TypeSet<float>& zvals, Color col )
{
    //TODO clear previous markers
    if ( aimdls_.isEmpty() || zvals.isEmpty() ) return;

    for ( int imdl=0; imdl<aimdls_.size(); imdl++ )
    {
	float tval = zvals[imdl];
	if ( !mIsUdf(tval) )
	    tval = aimdls_[imdl]->convertTo( tval, AIModel::TWT );
	//TODO plot on flat viewer
    }
}


void uiStratSynthDisp::wvltChg( CallBacker* )
{
    modelChanged();
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
    vwr_->clearAllPacks(); vwr_->setNoViewDone();
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
    for ( int iseq=0; iseq<lm_.size(); iseq+=dispeach_ )
    {
	const Strat::LayerSequence& seq = lm_.sequence( iseq );
	AIModel* aimod = seq.getAIModel( velidx, denidx, isvel, isden );
	aimdls_ += aimod;
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
    const Coord crd0( SI().maxCoord(false) );
    const float dx = SI().crlDistance();

    for ( int imdl=0; imdl<nraimdls; imdl++ )
    {
	synthgen.generate( *aimdls_[imdl] );
	SeisTrc* newtrc = new SeisTrc( synthgen.result() );
	const int trcnr = imdl*dispeach_ + 1;
	newtrc->info().nr = trcnr;
	newtrc->info().coord = crd0;
	newtrc->info().coord.x += trcnr * dx;
	newtrc->info().binid = SI().transform( newtrc->info().coord );
	tbuf->add( newtrc );
    }

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
}
