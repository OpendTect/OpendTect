/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          April 2002
 RCS:		$Id: uiseismmproc.cc,v 1.4 2002-04-23 16:10:33 bert Exp $
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
#include "uifilebrowser.h"
#include "uiiosel.h"
#include "uimsg.h"
#include "hostdata.h"
#include "iopar.h"


uiSeisMMProc::uiSeisMMProc( uiParent* p, const char* prognm, const IOPar& iop )
	: uiExecutor(p,getFirstJM(prognm,iop))
    	, running(false)
    	, finished(false)
{
    setCancelText( "Quit" );
    setOkText( "" );
    setTitleText("Manage processing");
    delay = 2000;

    tmpstordirfld = new uiIOFileSelect( this, "Temporary storage directory",
	    				false, jm->tempStorageDir() );
    tmpstordirfld->usePar( uiIOFileSelect::tmpstoragehistory );

    machgrp = new uiGroup( this, "Machine handling" );

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
    vwlogbut = new uiPushButton( usedmachgrp, "View log" );
    vwlogbut->activated.notify( mCB(this,uiSeisMMProc,vwLogPush) );
    vwlogbut->attach( rightAlignedBelow, usedmachfld );

    usedmachgrp->attach( rightOf, addbut );
    avmachfld->attach( heightSameAs, usedmachgrp );
    machgrp->setHAlignObj( addbut );
    machgrp->attach( alignedBelow, tmpstordirfld );

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
    jm->poststep.notify( mCB(this,uiSeisMMProc,postStep) );
}


void uiSeisMMProc::doFinalise()
{
    progbar->attach( widthSameAs, machgrp );
    progbar->attach( alignedBelow, progrfld );

    // But we start processing when at least one machine is added.
}


void uiSeisMMProc::postStep( CallBacker* )
{
    const char* txt = jm->progressMessage();
    if ( *txt ) progrfld->append( txt );
    updateCurMachs();
}


void uiSeisMMProc::execFinished()
{
    SeisMMJobMan* newjm = new SeisMMJobMan( *jm );
    if ( newjm->totalNr() < 1 )
    {
	finished = true;
	delete newjm;
	return;
    }

    delete jm; jm = newjm; task_ = newjm;
    first_time = true;
    timerTick(0);
}


void uiSeisMMProc::updateCurMachs()
{
    ObjectSet<BufferString> machs;
    jm->getActiveMachines( machs );
    int curit = usedmachfld->box()->currentItem();
    usedmachfld->box()->empty();
    bool havemachs = machs.size();
    if ( havemachs )
    {
	usedmachfld->box()->addItems( machs );
	deepErase( machs );
	if ( curit >= usedmachfld->box()->size() )
	    curit = usedmachfld->box()->size() - 1;
	usedmachfld->box()->setCurrentItem(curit);
    }
    stopbut->setSensitive( havemachs );
    vwlogbut->setSensitive( havemachs );
}


bool uiSeisMMProc::rejectOK( CallBacker* )
{
    BufferString msg;

    if ( finished )
	msg = "Do you want to discard log files?";
    else
    {
	msg = "This will stop all processing!\n\n";
	msg += "Do you want to remove already processed data?";
    }

    int res = uiMSG().askGoOnAfter( msg );
    if ( res == 2 )
	return false;

    if ( res == 0 )
    {
	if ( !jm->removeTempSeis() )
	{
	    msg = "Could not remove all tempory seismic data from\n";
	    msg += jm->tempStorageDir();
	    uiMSG().warning( msg );
	}
	jm->cleanup();
    }
    return true;

}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiSeisMMProc::addPush( CallBacker* )
{
    for( int idx=0; idx<avmachfld->box()->size(); idx++ )
    {
	if ( avmachfld->box()->isSelected(idx) )
	    jm->addHost( avmachfld->box()->textOfItem(idx) );
    }
    updateCurMachs();

    if ( !running && usedmachfld->box()->size() )
    {
	tmpstordirfld->setSensitive(false);
	jm->setTempStorageDir( tmpstordirfld->getInput() );
	running = true;
	prepareNextCycle(0);
    }
}


int uiSeisMMProc::getCurMach( BufferString& mach ) const
{
    int curit = usedmachfld->box()->currentItem();
    if ( curit < 0 ) return -1;

    mach = usedmachfld->box()->textOfItem(curit);
    int occ = 0;
    for( int idx=0; idx<curit-1; idx++ )
	if ( mach == usedmachfld->box()->textOfItem(idx) )
	    occ++;

    return occ;
}


void uiSeisMMProc::stopPush( CallBacker* )
{
    BufferString mach;
    if ( getCurMach(mach) < 0 ) { pErrMsg("Can't find machine"); return; }
    jm->removeHost( mach );
    updateCurMachs();
}


void uiSeisMMProc::vwLogPush( CallBacker* )
{
    BufferString mach;
    int occ = getCurMach( mach );
    if ( occ < 0 ) { pErrMsg("Can't find machine"); return; }

    BufferString fname;
    if ( !jm->getLogFileName(mach,occ,fname) )
	mErrRet("Cannot find log file")

    uiFileBrowser dlg( this, fname );
    dlg.go();
}
