/*
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          May 2008
___________________________________________________________________

-*/

#include "uiflatviewcoltabed.h"

#include "coltabsequence.h"
#include "uicoltabsel.h"


uiFlatViewColTabEd::uiFlatViewColTabEd( uiColTabToolBar& ctab )
    : colTabChgd(this)
    , ctseltool_(ctab.selTool())
{
    mAttachCB( ctseltool_.seqChanged, uiFlatViewColTabEd::seqChgCB );
    mAttachCB( ctseltool_.mapperSetup()->objectChanged(),
		uiFlatViewColTabEd::mapperChgCB );
}


uiFlatViewColTabEd::~uiFlatViewColTabEd()
{
    detachAllNotifiers();
}


void uiFlatViewColTabEd::setSensitive( bool yn )
{
    ctseltool_.asParent()->setSensitive( yn );
}


void uiFlatViewColTabEd::setColTab( const FlatView::DataDispPars::VD& vdpars )
{
    vdpars_ = vdpars;
    ctseltool_.setSeqName( vdpars.colseqname_ );
    ctseltool_.useMapperSetup( *vdpars.mappersetup_ );
    setSensitive( true );
}


void uiFlatViewColTabEd::mapperChgCB( CallBacker* cb )
{
    // mGetMonitoredChgData( cb, chgdata );
    seqChgCB( 0 );
}


void uiFlatViewColTabEd::seqChgCB( CallBacker* cb )
{
    vdpars_.colseqname_ = ctseltool_.seqName();
    *vdpars_.mappersetup_ = *ctseltool_.mapperSetup();
    colTabChgd.trigger();
}
