/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratsynthdisp.cc,v 1.4 2010-12-07 16:16:02 cvsbert Exp $";

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


uiStratSynthDisp::uiStratSynthDisp( uiParent* p, const Strat::LayerModel& lm )
    : uiGroup(p,"LayerModel synthetics display")
    , wvlt_(0)
    , lm_(lm)
    , dispeach_(1)
{
    const CallBack redrawcb( mCB(this,uiStratSynthDisp,reDraw) );
    uiGroup* topgrp = new uiGroup( this, "Top group" );
    wvltfld_ = new uiSeisWaveletSel( topgrp );
    wvltfld_->newSelection.notify( redrawcb );

    vwr_ = new uiFlatViewer( this );
    vwr_->setInitialSize( uiSize(500,250) ); //TODO get hor sz from laymod disp
    vwr_->attach( ensureBelow, topgrp );
    FlatView::Appearance& app = vwr_->appearance();
    app.setGeoDefaults( true );
    app.setDarkBG( false );
    app.annot_.x1_.showAll( false );
    app.annot_.title_.setEmpty();
    app.ddpars_.show( true, false );
    // app.ddpars_.wva_.overlap_ = 1;
}


uiStratSynthDisp::~uiStratSynthDisp()
{
    emptyPacks();
    delete wvlt_;
    deepErase( aimdls_ );
}


void uiStratSynthDisp::setDispEach( int nr )
{
    dispeach_ = nr;
    modelChanged();
}


void uiStratSynthDisp::emptyPacks()
{
    vwr_->setPack( true, DataPack::cNoID(), false );
    vwr_->setPack( false, DataPack::cNoID(), false );
}


#define mErrRet(s) { if ( s ) uiMSG().error(s); emptyPacks(); return; }

void uiStratSynthDisp::modelChanged()
{
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

    const int velidx = 1; const int denidx = 2; //TODO
    int maxsz = 0; SamplingData<float> sd;
    for ( int iseq=0; iseq<lm_.size(); iseq+=dispeach_ )
    {
	const Strat::LayerSequence& seq = lm_.sequence( iseq );
	AIModel* aimod = seq.getAIModel( velidx, denidx );
	aimdls_ += aimod;
	int sz;
	sd = Seis::SynthGenerator::getDefOutSampling( *aimod, *wvlt_, sz );
	if ( sz > maxsz ) maxsz = sz;
    }
    if ( maxsz < 2 )
	mErrRet(0)

    Seis::SynthGenerator synthgen( *wvlt_ );
    synthgen.setOutSampling( sd, maxsz );
    SeisTrcBuf* tbuf = new SeisTrcBuf( true );
    const Coord crd0( SI().maxCoord(false) );
    const float dx = SI().crlDistance();

    for ( int imdl=0; imdl<aimdls_.size(); imdl++ )
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
    DPM(DataPackMgr::FlatID()).add( dp );
    vwr_->setPack( true, dp->id(), false );
}
