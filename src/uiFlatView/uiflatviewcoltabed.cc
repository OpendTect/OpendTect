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
    : ctseltool_(ctab.selTool())
    , colTabChgd(this)
    , refreshReq(this)
{
    mAttachCB( ctseltool_.seqChanged, uiFlatViewColTabEd::seqChgCB );
    mAttachCB( ctseltool_.seqModified, uiFlatViewColTabEd::seqChgCB );
    mAttachCB( ctseltool_.mapper().objectChanged(),
		uiFlatViewColTabEd::mapperChgCB );
    mAttachCB( ctseltool_.refreshReq, uiFlatViewColTabEd::refreshReqCB );
}


uiFlatViewColTabEd::~uiFlatViewColTabEd()
{
    detachAllNotifiers();
}


void uiFlatViewColTabEd::setSensitive( bool yn )
{
    ctseltool_.asParent()->setSensitive( yn );
}


void uiFlatViewColTabEd::setDisplayPars( const VDDispPars& vdpars )
{
    vdpars_ = vdpars;
    ctseltool_.setSeqName( vdpars_.colseqname_ );
    ctseltool_.setMapper( *vdpars_.mapper_ );
    setSensitive( true );
}


void uiFlatViewColTabEd::mapperChgCB( CallBacker* cb )
{
    *vdpars_.mapper_ = ctseltool_.mapper();
    colTabChgd.trigger();
}


void uiFlatViewColTabEd::seqChgCB( CallBacker* cb )
{
    vdpars_.colseqname_ = ctseltool_.seqName();
    colTabChgd.trigger();
}


void uiFlatViewColTabEd::refreshReqCB( CallBacker* cb )
{
    refreshReq.trigger();
}
