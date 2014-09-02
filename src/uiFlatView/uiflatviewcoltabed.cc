/*
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          May 2008
___________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiflatviewcoltabed.h"

#include "coltab.h"
#include "coltabsequence.h"
#include "flatview.h"

#include "uicolortable.h"


uiFlatViewColTabEd::uiFlatViewColTabEd( uiColorTable& ct, FlatView::Viewer& vwr)
    : ddpars_(vwr.appearance().ddpars_)
    , vwr_( &vwr )
    , colseq_(*new ColTab::Sequence())
    , colTabChgd(this)
    , uicoltab_(ct)
{
    ColTab::SM().get( ddpars_.vd_.ctab_.buf(), colseq_ );
    uicoltab_.enableManage( false );
    uicoltab_.seqChanged.notify( mCB(this,uiFlatViewColTabEd,colTabChanged) );
    uicoltab_.scaleChanged.notify( mCB(this,uiFlatViewColTabEd,colTabChanged) );
    setColTab( vwr );
}


uiFlatViewColTabEd::~uiFlatViewColTabEd()
{
    delete &colseq_;
}


void uiFlatViewColTabEd::setSensitive( bool yn )
{
    mDynamicCastGet(uiColorTableToolBar*,ctab,&uicoltab_);
    if ( ctab ) ctab->setSensitive( yn );
}


void uiFlatViewColTabEd::setColTab( const FlatView::Viewer& vwr )
{
    uicoltab_.setDispPars( vwr.appearance().ddpars_.vd_ );
    uicoltab_.setSequence( vwr.appearance().ddpars_.vd_.ctab_ );
    uicoltab_.setInterval( vwr.getDataRange(false) );
}


void uiFlatViewColTabEd::setColTab( const FlatView::DataDispPars::VD& vdpars )
{
    vdpars_ = vdpars;
    uicoltab_.setDispPars( vdpars );
    uicoltab_.setSequence( vdpars.ctab_ );
    uicoltab_.setInterval( vdpars.mappersetup_.range_ );
    setSensitive( true );
}


void uiFlatViewColTabEd::colTabChanged( CallBacker* )
{
    vdpars_.ctab_ = uicoltab_.colTabSeq().name();
    uicoltab_.getDispPars( vdpars_ );
    colTabChgd.trigger();
}
