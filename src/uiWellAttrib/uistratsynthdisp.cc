/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratsynthdisp.cc,v 1.2 2010-12-06 12:18:39 cvsbert Exp $";

#include "uistratsynthdisp.h"
#include "uiseiswvltsel.h"
#include "uicombobox.h"
#include "uiflatviewer.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "wavelet.h"
#include "seisbuf.h"
#include "aimodel.h"


uiStratSynthDisp::uiStratSynthDisp( uiParent* p, const Strat::LayerModel& lm )
    : uiGroup(p,"LayerModel synthetics display")
    , lm_(lm)
    , tbuf_(*new SeisTrcBuf(false))
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
    tbuf_.deepErase();
    delete &tbuf_;
}


void uiStratSynthDisp::modelChanged()
{
    const int velidx = 0;
    const int denidx = 1;
    for ( int iseq=0; iseq<lm_.size(); iseq++ )
    {
	const Strat::LayerSequence& seq = lm_.sequence( iseq );
	AIModel* aimod = seq.getAIModel( velidx, denidx );
    }
    // Generate synthetics
    // Set datapack to flat viewer
}
