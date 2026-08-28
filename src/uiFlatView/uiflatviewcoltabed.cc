/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiflatviewcoltabed.h"

#include "coltabsequence.h"
#include "uicolortable.h"


uiFlatViewColTabEd::uiFlatViewColTabEd( uiColorTableToolBar& ctab,
					bool enabmanage )
    : colTabChgd(this)
    , uicoltab_(ctab)
{
    uicoltab_.enableManage( enabmanage );
    mAttachCB( uicoltab_.seqChanged, uiFlatViewColTabEd::colTabChangedCB );
    mAttachCB( uicoltab_.scaleChanged, uiFlatViewColTabEd::colTabChangedCB );
}


uiFlatViewColTabEd::~uiFlatViewColTabEd()
{
    detachAllNotifiers();
}


void uiFlatViewColTabEd::setInterval( const Interval<float>& range )
{
    vdpars_.mappersetup_.range_ = range;
    uicoltab_.setInterval( range );
}


void uiFlatViewColTabEd::setEditable( bool yn )
{
    uicoltab_.setEditable( yn );
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
    setInterval( vdpars.mappersetup_.range_ );
    setEditable( true );
}


void uiFlatViewColTabEd::colTabChangedCB( CallBacker* )
{
    vdpars_.ctab_ = uicoltab_.colTabSeq().name();
    const bool needrescale =
	vdpars_.mappersetup_.needsReClip( uicoltab_.colTabMapperSetup() );
    uicoltab_.getDispPars( vdpars_ );
    if ( needrescale )
	vdpars_.mappersetup_.range_ = Interval<float>::udf();

    colTabChgd.trigger();
    setEditable( true );
}
