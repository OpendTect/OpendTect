/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          April 2002
 RCS:		$Id: uiseismmproc.cc,v 1.2 2002-04-22 14:40:23 bert Exp $
________________________________________________________________________

-*/

#include "uiseismmproc.h"
#include "seismmjobman.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uiprogressbar.h"
#include "uibutton.h"
#include "uitextedit.h"
#include "uiseparator.h"
#include "hostdata.h"
#include "iopar.h"


uiSeisMMProc::uiSeisMMProc( uiParent* p, const char* prognm, const IOPar& iop )
	: uiExecutor(p,getFirstJM(prognm,iop))
{
    setCancelText( "Quit" );
    setOkText( "" );
    setTitleText("Manage processing");
    delay = 2000;

    uiGroup* machgrp = new uiGroup( this, "Machine handling" );
    machgrp->attach( alignedBelow, progbar );
    progbar->attach( widthSameAs, machgrp );

    avmachfld = new uiLabeledListBox( machgrp, "Available hosts", true,
				      uiLabeledListBox::AboveMid );
    HostDataList hdl;
    for ( int idx=0; idx<hdl.size(); idx++ )
	avmachfld->box()->addItem( hdl[idx]->shortestName() );

    addbut = new uiPushButton( machgrp, ">> Add >>" );
    addbut->activated.notify( mCB(this,uiSeisMMProc,addPush) );
    addbut->attach( rightOf, avmachfld );

    uiGroup* usedmachgrp = new uiGroup( machgrp, "Machine handling" );
    usedmachfld = new uiLabeledListBox( usedmachgrp, "Used hosts", false,
				        uiLabeledListBox::AboveMid );
    stopbut = new uiPushButton( usedmachgrp, "Stop" );
    stopbut->activated.notify( mCB(this,uiSeisMMProc,stopPush) );
    stopbut->attach( alignedBelow, usedmachfld );
    vwlogfld = new uiPushButton( usedmachgrp, "View log" );
    vwlogfld->activated.notify( mCB(this,uiSeisMMProc,vwLogPush) );
    vwlogfld->attach( rightAlignedBelow, usedmachfld );

    usedmachgrp->attach( rightOf, addbut );
    avmachfld->attach( heightSameAs, usedmachgrp );

    uiSeparator* sep = new uiSeparator( this, "Hor sep", true );
    sep->attach( stretchedBelow, machgrp );
    uiLabel* lbl = new uiLabel( this, "Progress" );
    lbl->attach( alignedBelow, sep );
    progrfld = new uiTextEdit( this, "Processing progress", true );
    progrfld->attach( alignedBelow, lbl );
    progrfld->attach( widthSameAs, sep );
    progrfld->setPrefHeightInChar( 7 );
}


Executor& uiSeisMMProc::getFirstJM( const char* prognm, const IOPar& iopar )
{
    const char* res = iopar.find( "Output Seismics Key" );
    BufferString seisoutkey( res ? res : "Output.1.Seismic ID" );
    res = iopar.find( "Inline Range Key" );
    BufferString ilrgkey( res ? res : "Output.1.In-line range" );
    jm = new SeisMMJobMan( prognm, iopar, seisoutkey, ilrgkey );
    newJM();
    return *jm;
}


uiSeisMMProc::~uiSeisMMProc()
{
    delete jm;
}


void uiSeisMMProc::newJM()
{
    if ( !jm ) return;
    jm->poststep.notify( mCB(this,uiSeisMMProc,dispProgress) );
}


void uiSeisMMProc::dispProgress( CallBacker* )
{
    const char* txt = jm->progressMessage();
    if ( *txt ) progrfld->append( txt );
}


bool uiSeisMMProc::rejectOK( CallBacker* )
{
    return true;
}


void uiSeisMMProc::addPush( CallBacker* )
{
}


void uiSeisMMProc::stopPush( CallBacker* )
{
}


void uiSeisMMProc::vwLogPush( CallBacker* )
{
}
