/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratsynthdisp.cc,v 1.3 2010-12-06 16:16:23 cvsbert Exp $";

#include "uistratsynthdisp.h"
#include "uiseiswvltsel.h"
#include "uicombobox.h"
#include "uiflatviewer.h"
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
    deepErase( aimdls_ );
}


void uiStratSynthDisp::emptyPacks()
{
    vwr_->setPack( true, DataPack::cNoID(), false );
    vwr_->setPack( false, DataPack::cNoID(), false );
}


#define mErrRet { emptyPacks(); return; }

void uiStratSynthDisp::modelChanged()
{
    IOObj* ioobj = IOM().get( wvltfld_->getID() );
    if ( !ioobj ) mErrRet
    PtrMan<Wavelet> wvlt = Wavelet::get( ioobj );
    delete ioobj;
    if ( !wvlt ) mErrRet

    const Coord crd0( SI().minCoord(false) );
    const float dx = SI().crlDistance();
    Seis::SynthGenerator synthgen( *wvlt );
    const int velidx = 0;
    const int denidx = 1;
    SeisTrcBuf* tbuf = new SeisTrcBuf( true );
    for ( int iseq=0; iseq<lm_.size(); iseq++ )
    {
	const Strat::LayerSequence& seq = lm_.sequence( iseq );
	AIModel* aimod = seq.getAIModel( velidx, denidx );
	aimdls_ += aimod;
	synthgen.generate( *aimod );
	SeisTrc* newtrc = new SeisTrc( synthgen.result() );
	const int trcnr = iseq + 1;
	newtrc->info().nr = trcnr;
	newtrc->info().coord = crd0;;
	newtrc->info().coord.x -= trcnr * dx;
	newtrc->info().binid = SI().transform( newtrc->info().coord );
	tbuf->add( new SeisTrc(synthgen.result()) );
    }
    SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( tbuf, Seis::Line,
	    			SeisTrcInfo::TrcNr, "Seismic" );
    dp->setName( "Model synthetics" );
    DPM(DataPackMgr::FlatID()).add( dp );
    vwr_->setPack( true, dp->id(), false );
}
