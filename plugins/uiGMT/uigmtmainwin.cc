/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
________________________________________________________________________

-*/

#include "uigmtmainwin.h"

#include "clientservicebase.h"
#include "file.h"
#include "filepath.h"
#include "gmtclip.h"
#include "gmtpar.h"
#include "gmtprocflow.h"
#include "gmtprocflowtr.h"
#include "initgmtplugin.h"
#include "keystrs.h"
#include "oddirs.h"

#include "uibatchjobdispatchersel.h"
#include "uitoolbutton.h"
#include "uibuttongroup.h"
#include "uidesktopservices.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uifiledlg.h"
#include "uigmtbasemap.h"
#include "uigmtclip.h"
#include "uigmtoverlay.h"
#include "uiioobjseldlg.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitoolbar.h"
#include "od_helpids.h"


uiGMTMainWin::uiGMTMainWin( uiParent* p )
    : uiDialog(p,uiDialog::Setup(getCaptionStr(),mNoDlgTitle,
                                 mODHelpKey(mGMTMainWinHelpID) )
				.modal(false) )
    , ctio_(*mMkCtxtIOObj(ODGMTProcFlow))
    , needsave_(false)
{
    setCtrlStyle( CloseOnly );

    uiGroup* rightgrp = new uiGroup( this, "Right group" );
    tabstack_ = new uiTabStack( rightgrp, "Tab" );

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

    addbut_ = new uiPushButton( rightgrp, uiStrings::sAdd(),
				mCB(this,uiGMTMainWin,addCB), true );
    addbut_->setToolTip( tr("Add to current flow") );
    addbut_->attach( alignedBelow, tabstack_ );
    editbut_ = new uiPushButton( rightgrp, uiStrings::sReplace(),
				mCB(this,uiGMTMainWin,editCB), true );
    editbut_->setToolTip( tr("Update current item in flow") );
    editbut_->attach( rightOf, addbut_ );
    resetbut_ = new uiPushButton( rightgrp, uiStrings::sReset(),
				mCB(this,uiGMTMainWin,resetCB), true );
    resetbut_->setToolTip( tr("Reset input fields") );
    resetbut_->attach( rightOf, editbut_ );

    //Needs the buttons:
    mAttachCB( tabstack_->selChange(), uiGMTMainWin::tabSel );

    uiSeparator* sep = new uiSeparator( this, "VSep", OD::Vertical );
    sep->attach( stretchedLeftTo, rightgrp );

    flowgrp_ = new uiGroup( this, "Flow Group" );

    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Map overlays") );
    flowfld_ = new uiListBox( flowgrp_, su );
    mAttachCB( flowfld_->selectionChanged, uiGMTMainWin::selChg );

    const CallBack butpushcb( mCB(this,uiGMTMainWin,butPush) );
    uiButtonGroup* bgrp = new uiButtonGroup( flowgrp_, "", OD::Horizontal );
    bgrp->displayFrame( true );
    upbut_ = new uiToolButton( bgrp, uiToolButton::UpArrow,
				tr("Move current item up"), butpushcb );
    downbut_ = new uiToolButton( bgrp, uiToolButton::DownArrow,
				 tr("Move current item down"), butpushcb );
    rmbut_ = new uiToolButton( bgrp, "remove",
				tr("Remove current item from flow"), butpushcb);
    bgrp->attach( centeredBelow, flowfld_ );


    flowgrp_->setHAlignObj( flowfld_ );
    BufferString defseldir = File::Path(GetDataDir()).add("Misc").fullPath();
    uiFileSel::Setup fssu;
    fssu.setForWrite().initialselectiondir( defseldir )
	.setFormat( tr("Postscript files"), "ps", "eps" );
    filefld_ = new uiFileSel( this, uiStrings::sOutputFile(), fssu );
    filefld_->attach( alignedBelow, flowgrp_ );
    filefld_->attach( ensureLeftOf, sep );

    createbut_ = new uiPushButton( this, tr("Create Map"),
				   mCB(this,uiGMTMainWin,createPushCB), true );
    createbut_->attach( alignedBelow, filefld_ );

    viewbut_ = new uiPushButton( this, tr("View Map"),
				 mCB(this,uiGMTMainWin,viewPush), true );
    viewbut_->attach( rightTo, createbut_ );

    flowgrp_->attach( leftTo, rightgrp );
    flowgrp_->attach( ensureLeftOf, sep );
    batchfld_ = new uiBatchJobDispatcherSel( rightgrp, true,
					     Batch::JobSpec::NonODBase,
                          OS::BatchWait );
    /* OS::BatchWait is only needed for now as not all 
    batch programs are registered */
    batchfld_->jobSpec().prognm_ = "od_gmtexec";
    batchfld_->setJobName( "GMT_Proc" );
    batchfld_->display( false );

    uiToolBar* toolbar = new uiToolBar( this, tr("Flow Tools") );
    toolbar->addButton( "open", tr("Open Flow"),
			mCB(this,uiGMTMainWin,openFlow) );
    toolbar->addButton( "save", tr("Save Current Flow"),
			mCB(this,uiGMTMainWin,saveFlow) );
    toolbar->addButton( "clear", tr("Clear flow"),
			mCB(this,uiGMTMainWin,newFlow) );

    mAttachCB( postFinalise(), uiGMTMainWin::postFinaliseCB );
}


uiGMTMainWin::~uiGMTMainWin()
{
    detachAllNotifiers();
    deepErase( pars_ );
}


void uiGMTMainWin::postFinaliseCB( CallBacker* )
{
    tabSel( nullptr );
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
    if ( needsave_ &&
	 !uiMSG().askContinue(tr("Current flow has not been saved, continue?")))
	return;

    filefld_->clear();
    deepErase( pars_ );
    flowfld_->setEmpty();
    basemapgrp_->reset();
    for ( int idx=0; idx<overlaygrps_.size(); idx++ )
	overlaygrps_[idx]->reset();

    needsave_ = false;
}


void uiGMTMainWin::openFlow( CallBacker* )
{
    if ( needsave_ &&
	 !uiMSG().askContinue(tr("Current flow has not been saved, continue?")))
	return;

    ctio_.ctxt_.forread_ = true;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( dlg.go() )
    {
	ctio_.setObj( dlg.ioObj()->clone() );
	uiString emsg; ODGMT::ProcFlow pf;
	if ( !ODGMTProcFlowTranslator::retrieve(pf,ctio_.ioobj_,emsg) )
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
    ctio_.ctxt_.forread_ = false;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( !dlg.go() ) return;

    ctio_.setObj( dlg.ioObj()->clone() );
    uiString emsg; ODGMT::ProcFlow pf;
    IOPar& par = pf.pars();

    BufferString fnm = filefld_->fileName();
    if ( !fnm.isEmpty() )
	par.set( sKey::FileName(), fnm );

    IOPar basemappar;
    basemappar.set( ODGMT::sKeyGroupName(), "Basemap" );
    if ( !basemapgrp_->fillPar(basemappar) )
	 return;

    BufferString numkey = "0";
    par.mergeComp( basemappar, numkey );
    for ( int ldx=0; ldx<pars_.size(); ldx++ )
    {
	numkey = ldx + 1;
	par.mergeComp( *pars_[ldx], numkey );
    }

    if ( !ODGMTProcFlowTranslator::store(pf,ctio_.ioobj_,emsg) )
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
	GMTPar* tmppar = pars_.removeSingle( curidx );
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
	    const uiString curtxt(
				toUiString(flowfld_->textOfItem(curidx)) );
	    const uiString newcurtxt(
				toUiString(flowfld_->textOfItem(newcur)) );
	    flowfld_->setItemText( newcur, curtxt );
	    flowfld_->setItemText( curidx, newcurtxt );
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
    if ( !pars_.validIdx(selidx) )
    {
	tabstack_->setCurrentPage( basemapgrp_ );
	return;
    }

    const FixedString tabname = pars_[selidx]->find( ODGMT::sKeyGroupName() );
    for ( int idx=0; idx<overlaygrps_.size(); idx++ )
    {
	if ( tabname == overlaygrps_[idx]->name().buf() )
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
    iop.set( ODGMT::sKeyGroupName(), gmtgrp->name() );
    if ( !gmtgrp->fillPar(iop) )
	return;

    GMTPar* par = GMTPF().create( iop, nullptr );
    if ( !par ) return;

    flowfld_->addItem( toUiString(par->userRef()) );
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
    iop.set( ODGMT::sKeyGroupName(), gmtgrp->name() );
    if ( !gmtgrp->fillPar(iop) )
	return;

    GMTPar* par = GMTPF().create( iop, nullptr );
    flowfld_->setItemText( selidx, toUiString(par->userRef()) );
    delete pars_.replace( selidx, par );
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


void uiGMTMainWin::createPushCB( CallBacker* )
{
    if ( !GMT::hasGMT() )
    {
        uiMSG().error( tr( "GMT installation not found. Cannot start." ) );
        return;
    }

    if ( !fillPar() )
        return;

    BatchServiceClientMgr& batchmgr = BatchServiceClientMgr::getMgr();
    const bool firstproc = procids_.isEmpty();
    if ( firstproc )
        mAttachCB( batchmgr.batchStarted, uiGMTMainWin::batchStartedCB );

    createbut_->setSensitive( false );
    filefld_->setSensitive( false );
    const bool canview = viewbut_->isSensitive();
    viewbut_->setSensitive( false );
    Batch::ID procid;
    const bool submitok = batchfld_->start( &procid );
    if ( submitok )
        procids_ += procid;
    else
    {
        if ( firstproc )
            mDetachCB( batchmgr.batchStarted, uiGMTMainWin::batchStartedCB );
        createbut_->setSensitive( true );
        filefld_->setSensitive( true );
        viewbut_->setSensitive( canview );
    }
}


void uiGMTMainWin::batchStartedCB( CallBacker* cb )
{
    if ( !cb )
        return;

    BatchServiceClientMgr& batchmgr = BatchServiceClientMgr::getMgr();
    if ( !batchmgr.isOK() )
        return;

    mCBCapsuleUnpack(Network::Service::ID,servid,cb);
    const BufferString servicenm = batchmgr.serviceName( servid );
    const Network::Service::SubID procid = batchmgr.serviceSubID( servid );
    if ( !procids_.isPresent(procid) || servicenm != "od_gmtexec" )
        return;

    procids_ -= procid;
    if ( procids_.isEmpty() )
        mDetachCB( batchmgr.batchStarted, uiGMTMainWin::batchStartedCB );

    if ( servids_.isEmpty() )
        mAttachCB( batchmgr.batchEnded, uiGMTMainWin::batchEndedCB );

    servids_ += servid;
    outfnms_.add( filefld_->text() );

    createbut_->setSensitive( true );
    filefld_->setSensitive( true );

    //Only needed for now as not all batch programs are registered:
    batchmgr.sendAction( servid, BatchServiceClientMgr::sKeyDoWork() );
}


void uiGMTMainWin::batchEndedCB( CallBacker* cb )
{
    if ( !cb )
        return;

    mCBCapsuleUnpack(Network::Service::ID,servid,cb);
    if ( !servids_.isPresent(servid) )
        return;

    const int idx = servids_.indexOf( servid );
    servids_.removeSingle( idx );

    if ( servids_.isEmpty() )
    {
        BatchServiceClientMgr& batchmgr = BatchServiceClientMgr::getMgr();
        mDetachCB( batchmgr.batchEnded, uiGMTMainWin::batchEndedCB );
    }

    const BufferString mUnusedVar outfnm = outfnms_.get( idx );
    outfnms_.removeSingle( idx );
    //TODO: If outfnm is not the current on screen, propose to view it
    viewbut_->setSensitive( true );
}


void uiGMTMainWin::viewPush( CallBacker* )
{
    if ( !viewbut_ || !filefld_ )
	return;

    BufferString psfilenm = filefld_->fileName();
    if ( psfilenm.isEmpty() )
	return;

    File::Path psfp( psfilenm );
    psfp.setExtension( "ps" );
    psfilenm = psfp.fullPath();

    if ( !uiDesktopServices::openUrl(psfilenm) )
	uiMSG().error(tr("Cannot open PostScript file %1").arg(psfilenm));
}


bool uiGMTMainWin::fillPar()
{
    BufferString fnm = filefld_->fileName();
    if ( fnm.isEmpty() )
	mErrRet(uiStrings::phrSpecify(tr("an output file name")))

    File::Path fp( fnm );
    BufferString dirnm = fp.pathOnly();
    if ( !File::isDirectory(dirnm.buf()) || !File::isWritable(dirnm.buf()) )
	mErrRet(tr("Output directory is not writable"))

    fp.setExtension( "ps" );
    fnm = fp.fullPath();
    if ( File::exists(fnm.buf()) && !File::isWritable(fnm.buf()) )
	mErrRet(tr("Output file already exists and is read only"))

    IOPar& par = batchfld_->jobSpec().pars_;
    par.setEmpty();
    par.set( sKey::FileName(), fnm );
    int idx = 0;
    Interval<float> mapdim, xrg, yrg;
    IOPar basemappar;
    basemappar.set( ODGMT::sKeyGroupName(), "Basemap" );
    if ( !basemapgrp_->fillPar(basemappar) )
	 return false;

    basemappar.setYN( ODGMT::sKeyClosePS(), !pars_.size() );
    basemappar.get( ODGMT::sKeyMapDim(), mapdim );
    basemappar.get( ODGMT::sKeyXRange(), xrg );
    basemappar.get( ODGMT::sKeyYRange(), yrg );
    BufferString numkey( "", idx++ );
    par.mergeComp( basemappar, numkey );
    bool isclippingon = false;
    for ( int ldx=0; ldx<pars_.size(); ldx++ )
    {
	numkey = idx++;
	pars_[ldx]->set( ODGMT::sKeyMapDim(), mapdim );
	pars_[ldx]->set( ODGMT::sKeyXRange(), xrg );
	pars_[ldx]->set( ODGMT::sKeyYRange(), yrg );
	par.mergeComp( *pars_[ldx], numkey );
	mDynamicCastGet(const GMTClip*,gmtclip,pars_[ldx])
	if ( gmtclip )
	{
	    const bool isstart = gmtclip->isStart();
	    if ( isclippingon && isstart )
		mErrRet(tr("Start of clipping without terminating the previous \
			    clipping") );

	    if ( !isclippingon && !isstart )
		mErrRet(tr("Termination of clipping without a start") );

	    isclippingon = isstart;
	}
    }

    if ( isclippingon )
    {
	IOPar termclippingpar;
	termclippingpar.set( ODGMT::sKeyGroupName(), "Clipping" );
	uiGMTClipGrp::getTerminatingPars( termclippingpar );
	numkey = idx;
	par.mergeComp( termclippingpar, numkey );
    }

    return true;
}


bool uiGMTMainWin::usePar( const IOPar& par )
{
    FixedString fnm = par.find( sKey::FileName() );
    if ( fnm )
	filefld_->setFileName( fnm );

    int idx = 0;
    PtrMan<IOPar> basemappar = par.subselect( idx++ );
    if ( basemappar )
	basemapgrp_->usePar( *basemappar );

    flowfld_->setEmpty();
    deepErase( pars_ );
    while ( true )
    {
	PtrMan<IOPar> subpar = par.subselect( idx++ );
	if ( !subpar )
	    break;

	GMTPar* gmtpar = GMTPF().create( *subpar, nullptr );
	if ( !gmtpar )
	    continue;

	flowfld_->addItem( toUiString(gmtpar->userRef()) );
	pars_ += gmtpar;
    }

    return true;
}


uiString uiGMTMainWin::getCaptionStr() const
{
    return tr("GMT Mapping Tool for GMT v%1").arg(GMT::versionStr() );
}
