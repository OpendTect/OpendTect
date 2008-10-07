/*
___________________________________________________________________
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Satyaki Maitra
 * DATE     : May 2008
 * RCS	    : $Id: uiflatviewcoltabed.cc,v 1.3 2008-10-07 21:49:01 cvskris Exp $
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
    , colseq_(*new ColTab::Sequence())
    , colTabChgd(this)
{
    ColTab::SM().get( ddpars_.vd_.ctab_.buf(), colseq_ );
    uicoltab_ = new uiColorTable( p, colseq_, false );
    uicoltab_->setEnabManage( false );
    uicoltab_->setInterval( vwr.getDataRange(false) );
    uicoltab_->seqChanged.notify( mCB(this,uiFlatViewColTabEd,colTabChanged) );
    uicoltab_->scaleChanged.notify( mCB(this,uiFlatViewColTabEd,colTabChanged));
    setColTab();
}


uiFlatViewColTabEd::~uiFlatViewColTabEd()
{
    delete &colseq_;
}


void uiFlatViewColTabEd::setColTab()
{
    uicoltab_->setSequence( ddpars_.vd_.ctab_ );
    uicoltab_->setDispPars( ddpars_.vd_ );
}


void uiFlatViewColTabEd::colTabChanged( CallBacker* )
{
    ddpars_.vd_.ctab_ = uicoltab_->colTabSeq().name();
    uicoltab_->getDispPars( ddpars_.vd_ );
    colTabChgd.trigger();
}
