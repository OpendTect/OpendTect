/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: uigmtmainwin.cc,v 1.4 2008-08-14 10:52:52 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtmainwin.h"

#include "filegen.h"
#include "filepath.h"
#include "gmtpar.h"
#include "oddirs.h"
#include "strmdata.h"
#include "strmprov.h"
#include "timer.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uidesktopservices.h"
#include "uifileinput.h"
#include "uigmtbasemap.h"
#include "uigmtoverlay.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitabbar.h"


uiGMTMainWin::uiGMTMainWin( uiParent* p )
    : uiFullBatchDialog(p,uiFullBatchDialog::Setup("Create PostScript Map")
					     .procprognm("odgmtexec")
					     .modal(false))
    , addbut_(0)
    , editbut_(0)
    , tim_(0)
{
    setTitleText( "" );
    setCtrlStyle( LeaveOnly );

    uiGroup* rightgrp = new uiGroup( uppgrp_, "Right group" );
    tabstack_ = new uiTabStack( rightgrp, "Tab" );
    tabstack_->tabBar()->selected.notify( mCB(this,uiGMTMainWin,tabSel) );

    uiParent* tabparent = tabstack_->tabGroup();
    basemapgrp_ = new uiGMTBaseMapGrp( tabparent );
    tabstack_->addTab( basemapgrp_ );
    for ( int idx=0; idx<uiGMTOF().size(); idx++ )
    {
	const char* tabname = uiGMTOF().name( idx );
	uiGMTOverlayGrp* grp = uiGMTOF().create( tabparent, tabname );
	tabstack_->addTab( grp );
	overlaygrps_ += grp;
    }

    addbut_ = new uiPushButton( rightgrp, "&Add",
	    			mCB(this,uiGMTMainWin,addCB), true );
    addbut_->attach( alignedBelow, tabstack_ ); 
    editbut_ = new uiPushButton( rightgrp, "&Replace",
	    			mCB(this,uiGMTMainWin,editCB), true );
    editbut_->attach( rightOf, addbut_ );

    uiSeparator* sep = new uiSeparator( uppgrp_, "VSep", false );
    sep->attach( stretchedLeftTo, rightgrp );

    flowgrp_ = new uiGroup( uppgrp_, "Flow Group" );

    uiLabeledListBox* llb = new uiLabeledListBox( flowgrp_, "Map overlays" );
    flowfld_ = llb->box();
    flowfld_->selectionChanged.notify( mCB(this,uiGMTMainWin,selChg) );

    const CallBack butpushcb( mCB(this,uiGMTMainWin,butPush) );
    uiButtonGroup* bgrp = new uiButtonGroup( flowgrp_, "", false );
    bgrp->displayFrame( true );
    upbut_ = new uiToolButton( bgrp, "Up button", butpushcb );
    upbut_->setArrowType( uiToolButton::UpArrow );
    upbut_->setToolTip( "Move current item up" );
    downbut_ = new uiToolButton( bgrp, "Down button", butpushcb );
    downbut_->setArrowType( uiToolButton::DownArrow );
    downbut_->setToolTip( "Move current item down" );
    rmbut_ = new uiToolButton( bgrp, "Remove button", ioPixmap("trashcan.png"),
	    			butpushcb );
    rmbut_->setToolTip( "Remove current item from flow" );
    bgrp->attach( centeredBelow, llb );


    flowgrp_->setHAlignObj( flowfld_ );
    BufferString defseldir = FilePath(GetDataDir()).add("Misc").fullPath();
    filefld_ = new uiFileInput( uppgrp_, "Select output file",
	    			uiFileInput::Setup().forread(false)
						    .filter("*.ps")
	   					    .defseldir(defseldir) );
    filefld_->attach( alignedBelow, flowgrp_ );
    filefld_->attach( ensureLeftOf, sep );

    createbut_ = new uiPushButton( uppgrp_, "&Create Map",
	    			   mCB(this,uiGMTMainWin,createPush), true );
    createbut_->attach( alignedBelow, filefld_ );

    viewbut_ = new uiPushButton( uppgrp_, "&View Map",
	    			 mCB(this,uiGMTMainWin,viewPush), true );
    viewbut_->attach( rightTo, createbut_ );

    flowgrp_->attach( leftTo, rightgrp );
    flowgrp_->attach( ensureLeftOf, sep );
    uppgrp_->setHAlignObj( sep );
    setParFileNmDef( "GMT_Proc" );
    tabSel(0);
}


uiGMTMainWin::~uiGMTMainWin()
{
    deepErase( pars_ );
    delete tim_;
}


void uiGMTMainWin::tabSel( CallBacker* )
{
    if ( !tabstack_ || !addbut_ || !editbut_ )
	return;

    uiGroup* grp = tabstack_->currentPage();
    if ( !grp ) return;

    BufferString tabname = grp->name();
    const bool isoverlay = tabname != "Basemap";
    addbut_->setSensitive( isoverlay );
    editbut_->setSensitive( isoverlay );
}


void uiGMTMainWin::butPush( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,tb,cb)
    int curidx = flowfld_->currentItem();
    const int sz = flowfld_->size();

    if ( tb == rmbut_ )
    {
	if ( curidx < 0 ) return;
	flowfld_->removeItem( curidx );
	GMTPar* tmppar = pars_.remove( curidx );
	delete tmppar;
	if ( curidx >= flowfld_->size() )
	    curidx--;
    }
    else if ( tb == upbut_ || tb == downbut_ )
    {
	if ( curidx < 0 ) return;
	const bool isup = tb == upbut_;
	const int newcur = curidx + (isup ? -1 : 1);
	if ( newcur >= 0 && newcur < sz )
	{
	    BufferString tmp( flowfld_->textOfItem(newcur) );
	    flowfld_->setItemText( newcur, flowfld_->getText() );
	    flowfld_->setItemText( curidx, tmp );
	    pars_.swap( curidx, newcur );
	    curidx = newcur;
	}
    }

    if ( curidx >= 0 )
	flowfld_->setCurrentItem( curidx );

    setButStates(0);
}


void uiGMTMainWin::setButStates( CallBacker* cb )
{
    const bool havesel = !flowfld_->isEmpty();
    rmbut_->setSensitive( havesel );
    selChg( 0 );
}


void uiGMTMainWin::selChg( CallBacker* )
{
    if ( !flowfld_ || flowfld_->isEmpty() )
	return;

    const int selidx = flowfld_->currentItem();
    if ( selidx < 0 || selidx > pars_.size() )
    {
	tabstack_->setCurrentPage( basemapgrp_ );
	return;
    }

    BufferString tabname = pars_[selidx]->find( ODGMT::sKeyGroupName );
    for ( int idx=0; idx<overlaygrps_.size(); idx++ )
    {
	if ( tabname == overlaygrps_[idx]->name() )
	{
	    overlaygrps_[idx]->usePar( *pars_[selidx] );
	    tabstack_->setCurrentPage( overlaygrps_[idx] );
	}
    }

    upbut_->setSensitive( selidx );
    downbut_->setSensitive( selidx < (flowfld_->size()-1) );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

void uiGMTMainWin::addCB( CallBacker* )
{
    uiGroup* grp = tabstack_->currentPage();
    if ( !grp ) return;

    mDynamicCastGet( uiGMTOverlayGrp*, gmtgrp, grp );
    if ( !gmtgrp ) return;

    IOPar iop;
    iop.set( ODGMT::sKeyGroupName, gmtgrp->name() );
    if ( !gmtgrp->fillPar(iop) )
	return;

    GMTPar* par = GMTPF().create( iop );
    if ( !par ) return;

    flowfld_->addItem( par->userRef() );
    pars_ += par;
    flowfld_->setCurrentItem( flowfld_->size() - 1 );
    setButStates( 0 );
}


void uiGMTMainWin::editCB( CallBacker* )
{
    const int selidx = flowfld_->currentItem();
    if ( selidx < 0 || selidx >= flowfld_->size() )
	return;

    uiGroup* grp = tabstack_->currentPage();
    if ( !grp ) return;

    mDynamicCastGet( uiGMTOverlayGrp*, gmtgrp, grp );
    if ( !gmtgrp ) return;

    IOPar iop;
    iop.set( ODGMT::sKeyGroupName, gmtgrp->name() );
    if ( !gmtgrp->fillPar(iop) )
	return;

    GMTPar* par = GMTPF().create( iop );
    flowfld_->setItemText( selidx, par->userRef() );
    GMTPar* tmppar = pars_.replace( selidx, par );
    delete tmppar;
}


void uiGMTMainWin::createPush( CallBacker* )
{
    viewbut_->setSensitive( false );
    if ( !acceptOK(0) )
	return;

    tim_ = new Timer( "Status" );
    tim_->tick.notify( mCB(this,uiGMTMainWin,checkFileCB) );
    tim_->start( 100, false );
}


void uiGMTMainWin::checkFileCB( CallBacker* )
{
    FilePath fp( filefld_->fileName() );
    fp.setExtension( "tmp" );
    if ( !File_exists(fp.fullPath()) )
	return;

    tim_->stop();
    StreamProvider sp( fp.fullPath() );
    viewbut_->setSensitive( true );
    sp.remove();
}


void uiGMTMainWin::viewPush( CallBacker* )
{
    if ( !viewbut_ || !filefld_ )
	return;

    BufferString psfilenm = filefld_->fileName();
    if ( psfilenm.isEmpty() )
	return;

    FilePath psfp( psfilenm );
    psfp.setExtension( "ps" );
    psfilenm = psfp.fullPath();

    if ( !uiDesktopServices::openUrl(psfilenm) )
	uiMSG().error("Cannot open PostScript file ",psfilenm);
}


bool uiGMTMainWin::fillPar( IOPar& par )
{
    BufferString fnm = filefld_->fileName();
    if ( fnm.isEmpty() )
	mErrRet("Please specify an output file name")

    FilePath fp( fnm );
    fp.setExtension( "ps" );
    par.set( sKey::FileName, fp.fullPath() );
    int idx = 0;
    Interval<float> mapdim, xrg, yrg;
    IOPar basemappar;
    basemappar.set( ODGMT::sKeyGroupName, "Basemap" );
    if ( !basemapgrp_->fillPar(basemappar) )
	 return false;

    basemappar.setYN( ODGMT::sKeyClosePS, !pars_.size() );
    basemappar.get( ODGMT::sKeyMapDim, mapdim );
    basemappar.get( ODGMT::sKeyXRange, xrg );
    basemappar.get( ODGMT::sKeyYRange, yrg );
    BufferString numkey = idx++;
    par.mergeComp( basemappar, numkey );
    IOPar legendpar;
    makeLegendPar( legendpar );
    const bool haslegends = legendpar.size() > 1;
    if ( haslegends )
	legendpar.set( ODGMT::sKeyMapDim, mapdim );

    for ( int ldx=0; ldx<pars_.size(); ldx++ )
    {
	numkey = idx++;
	const bool closeps = !haslegends && ( ldx == pars_.size() - 1 );
	pars_[ldx]->setYN( ODGMT::sKeyClosePS, closeps );
	pars_[ldx]->set( ODGMT::sKeyMapDim, mapdim );
	pars_[ldx]->set( ODGMT::sKeyXRange, xrg );
	pars_[ldx]->set( ODGMT::sKeyYRange, yrg );
	par.mergeComp( *pars_[ldx], numkey );
    }

    if ( !pars_.size() )
	return true;

    numkey = idx;
    par.mergeComp( legendpar, numkey );
    return true;
}


void uiGMTMainWin::makeLegendPar( IOPar& legpar ) const
{
    legpar.set( ODGMT::sKeyGroupName, "Legend" );
    int pdx = 0;
    for ( int idx=0; idx<pars_.size(); idx++ )
    {
	IOPar par;
	if ( !pars_[idx]->fillLegendPar(par) )
	    continue;

	BufferString numkey = pdx++;
	legpar.mergeComp( par, numkey );
    }
}

