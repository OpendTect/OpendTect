/*
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          May 2008
___________________________________________________________________

-*/

#include "uiflatviewcoltabed.h"

#include "coltabsequence.h"
#include "uicolortable.h"


uiFlatViewColTabEd::uiFlatViewColTabEd( uiColorTableToolBar& ctab )
    : colTabChgd(this)
    , uicoltab_(ctab)
{
    uicoltab_.enableManage( false );
    mAttachCB( uicoltab_.seqChanged, uiFlatViewColTabEd::colTabChanged );
    mAttachCB( uicoltab_.scaleChanged, uiFlatViewColTabEd::colTabChanged );
}


uiFlatViewColTabEd::~uiFlatViewColTabEd()
{
    detachAllNotifiers();
}


void uiFlatViewColTabEd::setSensitive( bool yn )
{
    uicoltab_.setSensitive( yn );
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
    const bool needrescale =
	vdpars_.mappersetup_.needsReClip( uicoltab_.colTabMapperSetup() );
    uicoltab_.getDispPars( vdpars_ );
    if ( needrescale )
	vdpars_.mappersetup_.range_ = Interval<float>::udf();
    colTabChgd.trigger();
}
