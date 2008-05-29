/*
___________________________________________________________________
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Satyaki Maitra
 * DATE     : May 2008
 * RCS	    : $Id: uiflatviewcoltabed.cc,v 1.1 2008-05-29 11:36:25 cvssatyaki Exp $
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
    , colTabChged(this)
{
    ColTab::SM().get( ddpars_.vd_.ctab_.buf(), colseq_ );
    uicoltab_ = new uiColorTable( p, colseq_, false );
    uicoltab_->setEnabManage( false );
    uicoltab_->setInterval( vwr.getDataRange(false) );
    uicoltab_->seqChanged.notify( mCB(this,uiFlatViewColTabEd,colTabChanged) );
    uicoltab_->scaleChanged.notify( mCB(this,uiFlatViewColTabEd,colTabChanged));
    colTabChanged(0);
}


uiFlatViewColTabEd::~uiFlatViewColTabEd()
{
    delete &colseq_;
}


void uiFlatViewColTabEd::setColTab()
{
    uicoltab_->setTable( ddpars_.vd_.ctab_ );
    uicoltab_->setDispPars( ddpars_.vd_ );
}


void uiFlatViewColTabEd::colTabChanged( CallBacker* )
{
    ddpars_.vd_.ctab_ = uicoltab_->colTabSeq().name();
    uicoltab_->getDispPars( ddpars_.vd_ );
    colTabChged.trigger();
}
