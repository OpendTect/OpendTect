/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigmtmainwin.cc,v 1.29 2012/02/13 09:21:45 cvsbert Exp $";

#include "uigmtmainwin.h"

#include "file.h"
#include "filepath.h"
#include "gmtclip.h"
#include "gmtpar.h"
#include "gmtprocflow.h"
#include "gmtprocflowtr.h"
#include "oddirs.h"
#include "strmdata.h"
#include "strmprov.h"
#include "timer.h"

#include "uitoolbutton.h"
#include "uibuttongroup.h"
#include "uidesktopservices.h"
#include "uifileinput.h"
#include "uigmtbasemap.h"
#include "uigmtclip.h"
#include "uigmtoverlay.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitoolbar.h"


uiGMTMainWin::uiGMTMainWin( uiParent* p )
    : uiFullBatchDialog(p,uiFullBatchDialog::Setup("GMT Mapping Tool")
					     .procprognm("od_gmtexec")
					     .modal(false)
					     .showoutputopts(false))
    , addbut_(0)
    , editbut_(0)
    , ctio_(*mMkCtxtIOObj(ODGMTProcFlow))
    , tim_(0)
    , needsave_(false)
{
    setTitleText( "" );
    setCtrlStyle( LeaveOnly );
    setHelpID( "103.5.2" );

    uiGroup* rightgrp = new uiGroup( uppgrp_, "Right group" );
    tabstack_ = new uiTabStack( rightgrp, "Tab" );
    tabstack_->selChange().notify( mCB(this,uiGMTMainWin,tabSel) );

    uiParent* tabparent = tabstack_->tabGroup();
    basemapgrp_ = new uiGMTBaseMapGrp( tabparent );
    tabstack_->addTab( basemapgrp_ );
    for ( int idx=0; idx<uiGMTOF().size(); idx++ )
    {
	const char* tabname = uiGMTOF().name( idx );
	uiGMTOverlayGrp* grp = uiGMTOF().create( tabparent, tabname );
	if ( !grp )
	    continue;

	tabstack_->addTab( grp );
	overlaygrps_ += grp;
    }

    addbut_ = new uiPushButton( rightgrp, "&Add",
	    			mCB(this,uiGMTMainWin,addCB), true );
    addbut_->setToolTip( "Add to current flow" );
    addbut_->attach( alignedBelow, tabstack_ ); 
    editbut_ = new uiPushButton( rightgrp, "&Replace",
	    			mCB(this,uiGMTMainWin,editCB), true );
    editbut_->setToolTip( "Update current item in flow" );
    editbut_->attach( rightOf, addbut_ );
    resetbut_ = new uiPushButton( rightgrp, "Re&set",
	    			mCB(this,uiGMTMainWin,resetCB), true );
    resetbut_->setToolTip( "Reset input fields" );
    resetbut_->attach( rightOf, editbut_ );

    uiSeparator* sep = new uiSeparator( uppgrp_, "VSep", false );
    sep->attach( stretchedLeftTo, rightgrp );

    flowgrp_ = new uiGroup( uppgrp_, "Flow Group" );

    uiLabeledListBox* llb = new uiLabeledListBox( flowgrp_, "Map overlays" );
    flowfld_ = llb->box();
    flowfld_->selectionChanged.notify( mCB(this,uiGMTMainWin,selChg) );

    const CallBack butpushcb( mCB(this,uiGMTMainWin,butPush) );
    uiButtonGroup* bgrp = new uiButtonGroup( flowgrp_, "", false );
    bgrp->displayFrame( true );
    upbut_ = new uiToolButton( bgrp, uiToolButton::UpArrow,
	    			"Move current item up", butpushcb );
    downbut_ = new uiToolButton( bgrp, uiToolButton::DownArrow,
	    			 "Move current item down", butpushcb );
    rmbut_ = new uiToolButton( bgrp, "trashcan.png",
	    			"Remove current item from flow", butpushcb );
    bgrp->attach( centeredBelow, llb );


    flowgrp_->setHAlignObj( flowfld_ );
    BufferString defseldir = FilePath(GetDataDir()).add("Misc").fullPath();
    filefld_ = new uiFileInput( uppgrp_, "Output file",
			uiFileInput::Setup(uiFileDialog::Gen)
			.forread(false).filter("*.ps").defseldir(defseldir) );
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

    uiToolBar* toolbar = new uiToolBar( this, "Flow Tools" );
    toolbar->addButton( "newflow.png", "New flow",
	    		mCB(this,uiGMTMainWin,newFlow) );
    toolbar->addButton( "openflow.png", "Open Flow",
	    		mCB(this,uiGMTMainWin,openFlow) );
    toolbar->addButton( "saveflow.png", "Save Current Flow",
			mCB(this,uiGMTMainWin,saveFlow) );

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


void uiGMTMainWin::newFlow( CallBacker* )
{
    if ( needsave_ )
    {
	if ( !uiMSG().askContinue("Current flow has not been saved, continue?") )
	    return;
    }

    filefld_->clear();
    pars_.erase();
    flowfld_->setEmpty();
    basemapgrp_->reset();
    for ( int idx=0; idx<overlaygrps_.size(); idx++ )
	overlaygrps_[idx]->reset();

    needsave_ = false;
}


void uiGMTMainWin::openFlow( CallBacker* )
{
    if ( needsave_ )
    {
	if ( !uiMSG().askContinue("Current flow has not been saved, continue?") )
	    return;
    }

    ctio_.ctxt.forread = true;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( dlg.go() )
    {
	ctio_.setObj( dlg.ioObj()->clone() );
	BufferString emsg; ODGMT::ProcFlow pf;
	if ( !ODGMTProcFlowTranslator::retrieve(pf,ctio_.ioobj,emsg) )
	    uiMSG().error( emsg );
	else
	{
	    usePar( pf.pars() );
	    needsave_ = false;
	}
    }
}


void uiGMTMainWin::saveFlow( CallBacker* )
{
    ctio_.ctxt.forread = false;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( !dlg.go() ) return;

    ctio_.setObj( dlg.ioObj()->clone() );
    BufferString emsg; ODGMT::ProcFlow pf;
    IOPar& par = pf.pars();

    BufferString fnm = filefld_->fileName();
    if ( !fnm.isEmpty() )
	par.set( sKey::FileName, fnm );

    IOPar basemappar;
    basemappar.set( ODGMT::sKeyGroupName, "Basemap" );
    if ( !basemapgrp_->fillPar(basemappar) )
	 return;

    BufferString numkey = "0";
    par.mergeComp( basemappar, numkey );
    for ( int ldx=0; ldx<pars_.size(); ldx++ )
    {
	numkey = ldx + 1;
	par.mergeComp( *pars_[ldx], numkey );
    }

    if ( !ODGMTProcFlowTranslator::store(pf,ctio_.ioobj,emsg) )
	uiMSG().error( emsg );
    else
	needsave_ = false;
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
	needsave_ = true;
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
	    needsave_ = true;
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

    FixedString tabname = pars_[selidx]->find( ODGMT::sKeyGroupName );
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
    needsave_ = true;
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
    needsave_ = true;
}


void uiGMTMainWin::resetCB( CallBacker* )
{
    uiGroup* grp = tabstack_->currentPage();
    if ( !grp ) return;

    mDynamicCastGet( uiGMTBaseMapGrp*, basegrp, grp );
    if ( basegrp )
    {
	basemapgrp_->reset();
	return;
    }

    mDynamicCastGet( uiGMTOverlayGrp*, gmtgrp, grp );
    if ( !gmtgrp )
	return;

    gmtgrp->reset();
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
    if ( !File::exists(fp.fullPath()) )
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
    BufferString dirnm = fp.pathOnly();
    if ( !File::isDirectory(dirnm.buf()) || !File::isWritable(dirnm.buf()) )
	mErrRet("Output directory is not writable")

    fp.setExtension( "ps" );
    fnm = fp.fullPath();
    if ( File::exists(fnm.buf()) && !File::isWritable(fnm.buf()) )
	mErrRet("Output file already exists and is read only")

    par.set( sKey::FileName, fnm );
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
    BufferString numkey( "", idx++ );
    par.mergeComp( basemappar, numkey );
    bool isclippingon = false;
    for ( int ldx=0; ldx<pars_.size(); ldx++ )
    {
	numkey = idx++;
	pars_[ldx]->set( ODGMT::sKeyMapDim, mapdim );
	pars_[ldx]->set( ODGMT::sKeyXRange, xrg );
	pars_[ldx]->set( ODGMT::sKeyYRange, yrg );
	par.mergeComp( *pars_[ldx], numkey );
	mDynamicCastGet(const GMTClip*,gmtclip,pars_[ldx])
	if ( gmtclip )
	{
	    const bool isstart = gmtclip->isStart();
	    if ( isclippingon && isstart )
		mErrRet("Start of clipping without terminating the previous \
			 clipping" );

	    if ( !isclippingon && !isstart )
		mErrRet("Termination of clipping without a start" );

	    isclippingon = isstart;
	}
    }

    if ( isclippingon )
    {
	IOPar termclippingpar;
	termclippingpar.set( ODGMT::sKeyGroupName, "Clipping" );
	uiGMTClipGrp::getTerminatingPars( termclippingpar );
	numkey = idx;
	par.mergeComp( termclippingpar, numkey );
    }

    return true;
}


bool uiGMTMainWin::usePar( const IOPar& par )
{
    FixedString fnm = par.find( sKey::FileName );
    if ( fnm )
	filefld_->setFileName( fnm );

    int idx = 0;
    PtrMan<IOPar> basemappar = par.subselect( idx++ );
    if ( basemappar )
	basemapgrp_->usePar( *basemappar );

    flowfld_->setEmpty();
    pars_.erase();
    while ( true )
    {
	PtrMan<IOPar> subpar = par.subselect( idx++ );
	if ( !subpar )
	    break;

	GMTPar* gmtpar = GMTPF().create( *subpar );
	if ( !gmtpar )
	    continue;

	flowfld_->addItem( gmtpar->userRef() );
	pars_ += gmtpar;
    }

    return true;
}
