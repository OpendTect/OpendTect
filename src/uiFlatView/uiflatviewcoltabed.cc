/*
___________________________________________________________________
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Satyaki Maitra
 * DATE     : May 2008
 * RCS	    : $Id: uiflatviewcoltabed.cc,v 1.4 2008-10-27 11:21:08 cvssatyaki Exp $
___________________________________________________________________
-*/


#include "uiflatviewcoltabed.h"

#include "callback.h"
#include "coltab.h"
#include "coltabsequence.h"
#include "flatview.h"

#include "uicolortable.h"


uiFlatViewColTabEd::uiFlatViewColTabEd( uiParent* p, FlatView::Viewer& vwr )
    : ddpars_(vwr.appearance().ddpars_)
    , vwr_( &vwr )
    , colseq_(*new ColTab::Sequence())
    , colTabChgd(this)
{
    ColTab::SM().get( ddpars_.vd_.ctab_.buf(), colseq_ );
    uicoltab_ = new uiColorTable( p, colseq_, false );
    uicoltab_->setEnabManage( false );
    uicoltab_->seqChanged.notify( mCB(this,uiFlatViewColTabEd,colTabChanged) );
    uicoltab_->scaleChanged.notify( mCB(this,uiFlatViewColTabEd,colTabChanged));
    setColTab( vwr );
}


uiFlatViewColTabEd::~uiFlatViewColTabEd()
{
    delete &colseq_;
}


void uiFlatViewColTabEd::setColTab( const FlatView::Viewer& vwr)
{
    uicoltab_->setInterval( vwr.getDataRange(false) );
    uicoltab_->setSequence( vwr.appearance().ddpars_.vd_.ctab_ );
    uicoltab_->setDispPars( vwr.appearance().ddpars_.vd_ );
}


void uiFlatViewColTabEd::colTabChanged( CallBacker* )
{
    ddpars_.vd_.ctab_ = uicoltab_->colTabSeq().name();
    uicoltab_->getDispPars( ddpars_.vd_ );
    colTabChgd.trigger();
}
