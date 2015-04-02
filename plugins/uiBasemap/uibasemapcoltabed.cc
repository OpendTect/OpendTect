/*
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2015
___________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uibasemapcoltabed.h"

#include "coltabsequence.h"
#include "uicolortable.h"


uiBasemapColTabEd::uiBasemapColTabEd( uiColorTableToolBar& ctab )
    : colTabChgd(this)
    , uicoltab_(ctab)
{
    uicoltab_.enableManage( false );
    mAttachCB( uicoltab_.seqChanged, uiBasemapColTabEd::colTabChanged );
    mAttachCB( uicoltab_.scaleChanged, uiBasemapColTabEd::colTabChanged );
}


uiBasemapColTabEd::~uiBasemapColTabEd()
{
    detachAllNotifiers();
}


void uiBasemapColTabEd::setSensitive( bool yn )
{
    uicoltab_.setSensitive( yn );
}


void uiBasemapColTabEd::setColTab( const FlatView::DataDispPars::VD& vdpars )
{
    vdpars_ = vdpars;
    uicoltab_.setDispPars( vdpars );
    uicoltab_.setSequence( vdpars.ctab_ );
    uicoltab_.setInterval( vdpars.mappersetup_.range_ );
    setSensitive( true );
}


void uiBasemapColTabEd::colTabChanged( CallBacker* )
{
    vdpars_.ctab_ = uicoltab_.colTabSeq().name();
    uicoltab_.getDispPars( vdpars_ );
    colTabChgd.trigger();
}

