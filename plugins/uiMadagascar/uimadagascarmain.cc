/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibatchjobdispatchersel.h"
#include "uimadagascarmain.h"
#include "uimadiosel.h"
#include "uimadbldcmd.h"
#include "madprocflow.h"
#include "madprocflowtr.h"
#include "madprocexec.h"
#include "madio.h"
#include "uilistbox.h"
#include "uitoolbutton.h"
#include "uibuttongroup.h"
#include "uigeninput.h"
#include "uitoolbar.h"
#include "uiseparator.h"
#include "uiioobjseldlg.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "trckeyzsampling.h"
#include "keystrs.h"
#include "ioman.h"
#include "oddirs.h"
#include "seisioobjinfo.h"
#include "strmprov.h"
#include "od_helpids.h"

const char* sKeySeisOutIDKey = "Output Seismics Key";

uiMadagascarMain::uiMadagascarMain( uiParent* p )
	: uiDialog(p,Setup(tr("Madagascar processing"),mNoDlgTitle,
                            mODHelpKey(mMadagascarMainHelpID) )
			   .modal(false) )
	, ctio_(*mMkCtxtIOObj(ODMadProcFlow))
	, bldfld_(0)
	, procflow_(*new ODMad::ProcFlow())
	, needsave_(false)
	, windowHide(this)
{
    setCtrlStyle( uiDialog::RunAndClose );
    createToolBar();

    uiGroup* maingrp = new uiGroup( this, "Main group" );

    infld_ = new uiMadIOSel( maingrp, true );
    infld_->selectionMade.notify( mCB(this,uiMadagascarMain,inpSel) );

    uiGroup* procgrp = crProcGroup( maingrp );
    procgrp->attach( alignedBelow, infld_ );

    outfld_ = new uiMadIOSel( maingrp, false );
    outfld_->selectionMade.notify( mCB(this,uiMadagascarMain,inpSel) );
    outfld_->attach( alignedBelow, procgrp );

    bldfld_ = new uiMadagascarBldCmd( this );
    bldfld_->cmdAvailable.notify( mCB(this,uiMadagascarMain,cmdAvail) );

    uiSeparator* sep = new uiSeparator( this, "VSep", OD::Vertical );
    sep->attach( rightTo, maingrp );
    bldfld_->attach( rightTo, sep );

    batchfld_ = new uiBatchJobDispatcherSel( this, false,
					     Batch::JobSpec::NonODBase );
    batchfld_->attach( ensureBelow, maingrp );
    batchfld_->jobSpec().prognm_ = "od_madexec";
    batchfld_->setJobName( "Mad_Proc" );

    updateCaption();
    postFinalize().notify( mCB(this,uiMadagascarMain,setButStates) );
}


uiMadagascarMain::~uiMadagascarMain()
{
    delete ctio_.ioobj_; delete &ctio_;
    delete &procflow_;
}


#define mAddButton(pm,func,tip) \
    toolbar->addButton( pm, tip, mCB(this,uiMadagascarMain,func) )

void uiMadagascarMain::createToolBar()
{
    uiToolBar* toolbar = new uiToolBar( this, tr("Flow tools") );
    mAddButton( "new", newFlow, tr("Empty this flow") );
    mAddButton( "open", openFlow, tr("Open saved flow") );
    mAddButton( "save", saveFlowCB, tr("Save flow") );
    mAddButton( "export", exportFlow, uiStrings::phrExport( tr("flow")) );
}


uiGroup* uiMadagascarMain::crProcGroup( uiGroup* grp )
{
    uiGroup* procgrp = new uiGroup( grp, "Proc group" );
    const CallBack butpushcb( mCB(this,uiMadagascarMain,butPush) );

    uiListBox::Setup su( OD::ChooseOnlyOne, tr("FLOW") );
    procsfld_ = new uiListBox( procgrp, su );
    procsfld_->setPrefWidthInChar( 20 );
    procsfld_->selectionChanged.notify( mCB(this,uiMadagascarMain,selChg) );

    uiButtonGroup* bgrp = new uiButtonGroup( procgrp, "", OD::Horizontal );
    bgrp->displayFrame( true );
    upbut_ = new uiToolButton( bgrp, uiToolButton::UpArrow,
				tr("Move current command up"), butpushcb );
    downbut_ = new uiToolButton( bgrp, uiToolButton::DownArrow,
				tr("Move current command down"), butpushcb );
    rmbut_ = new uiToolButton( bgrp, "remove",
				tr("Remove current command from flow"),
				butpushcb );
    bgrp->attach( centeredBelow, procsfld_ );

    procgrp->setHAlignObj( procsfld_ );
    return procgrp;
}


void uiMadagascarMain::inpSel( CallBacker* cb )
{
    if ( cb )
	needsave_ = true;

    IOPar& inpar = procflow_.input();
    infld_->fillPar( inpar );
    outfld_->useParIfNeeded( inpar );

    IOPar& outpar = procflow_.output();
    outfld_->fillPar( outpar );
    BufferString inptyp( inpar.find(sKey::Type()) );
    BufferString outptyp( outpar.find(sKey::Type()) );
}

#undef mErrRet
#define mErrRet(s) { uiMSG().error(s); return false; }

void uiMadagascarMain::cmdAvail( CallBacker* cb )
{
    ODMad::Proc* proc = bldfld_->proc();

    if ( !proc ) return;
    if ( !proc->errMsg().isEmpty() )
    {
	uiMSG().error( proc->errMsg() );
	return;
    }

    if ( bldfld_->isAdd() )
    {
	procsfld_->addItem( toUiString(proc->getSummary()) );
	procflow_ += proc;
	needsave_ = true;
	procsfld_->setCurrentItem( procsfld_->size() - 1 );
    }
    else
    {
	const int curidx = procsfld_->currentItem();
	if ( curidx < 0 ) return;
	needsave_ = true;
	procsfld_->setItemText( curidx, toUiString(proc->getSummary()) );
	ODMad::Proc* prevproc = procflow_.replace( curidx, proc );
	delete prevproc;
    }

    setButStates( this );
}


void uiMadagascarMain::hideReq( CallBacker* cb )
{
    bldfld_->display( false );
}


void uiMadagascarMain::butPush( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,tb,cb)
    int curidx = procsfld_->currentItem();
    const int sz = procsfld_->size();

    if ( tb == rmbut_ )
    {
	if ( curidx < 0 ) return;
	needsave_ = true;
	procsfld_->removeItem( curidx );
	ODMad::Proc* prevproc = procflow_.removeSingle( curidx );
	delete prevproc;
	if ( curidx >= procsfld_->size() )
	    curidx--;
    }
    else if ( tb == upbut_ || tb == downbut_ )
    {
	if ( curidx < 0 ) return;
	const bool isup = tb == upbut_;
	const int newcur = curidx + (isup ? -1 : 1);
	if ( newcur >= 0 && newcur < sz )
	{
	    BufferString tmp( procsfld_->textOfItem(newcur) );
	    procsfld_->setItemText( newcur, toUiString(procsfld_->getText()) );
	    procsfld_->setItemText( curidx, tmp );
	    procflow_.swap( curidx, newcur );
	    curidx = newcur;
	    needsave_ = true;
	}
    }

    if ( curidx >= 0 )
	procsfld_->setCurrentItem( curidx );

    setButStates(0);
}


void uiMadagascarMain::setButStates( CallBacker* cb )
{
    const bool havesel = !procsfld_->isEmpty();
    rmbut_->setSensitive( havesel );
    selChg( cb );
    inpSel(0);
}


void uiMadagascarMain::selChg( CallBacker* cb )
{
    const int curidx = procsfld_->isEmpty() ? -1 : procsfld_->currentItem();
    const int sz = procsfld_->size();
    upbut_->setSensitive( sz > 1 && curidx > 0 );
    downbut_->setSensitive( sz > 1 && curidx >= 0 && curidx < sz-1 );

    if ( cb == this || !bldfld_ ) return;
    const ODMad::Proc* proc = procflow_.validIdx(curidx)? procflow_[curidx] : 0;
    bldfld_->setProc( proc );
}


void uiMadagascarMain::newFlow( CallBacker* )
{
    if ( !askSave() ) return;

    deepErase( procflow_ );
    procflow_.setName( 0 );
    procflow_.input().setEmpty();
    procflow_.output().setEmpty();
    updateCaption();
}


void uiMadagascarMain::openFlow( CallBacker* )
{
    if ( !askSave() ) return;

    ctio_.ctxt_.forread_ = true;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( dlg.go() )
    {
	ctio_.setObj( dlg.ioObj()->clone() );
	uiString emsg;
	deepErase( procflow_ );
	procsfld_->setEmpty();
	if ( !ODMadProcFlowTranslator::retrieve(procflow_,ctio_.ioobj_,emsg) )
	    uiMSG().error( emsg );
	else
	{
	    infld_->usePar( procflow_.input() );
	    outfld_->usePar( procflow_.output() );
	    for ( int idx=0; idx<procflow_.size(); idx++ )
		procsfld_->addItem( toUiString(procflow_[idx]->getSummary()) );

	    procsfld_->setCurrentItem( procsfld_->size() - 1 );
	    needsave_ = false;
	    procflow_.setName( ctio_.ioobj_->name() );
	    updateCaption();
	}
    }
}


void uiMadagascarMain::saveFlowCB( CallBacker* )
{
    saveFlow();
}


bool uiMadagascarMain::saveFlow()
{
    ctio_.ctxt_.forread_ = false;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( !dlg.go() )
	return false;

    ctio_.setObj( dlg.ioObj()->clone() );
    uiString emsg;
    if ( !ODMadProcFlowTranslator::store(procflow_,ctio_.ioobj_,emsg) )
	mErrRet( emsg )

    needsave_ = false;
    procflow_.setName( ctio_.ioobj_->name() );
    updateCaption();
    return true;
}


bool uiMadagascarMain::askSave( bool withcancel )
{
    if ( !needsave_ ) return true;

    uiString msg = tr("Current Madagascar flow %1 is not saved")
		 .arg(procflow_.name());
    const int ret = uiMSG().askSave( msg, withcancel );
    if ( ret < 0 ) return false;
    if ( !ret ) return true;

    const bool saved = saveFlow();
    return withcancel ? saved : true;
}


void uiMadagascarMain::updateCaption()
{
    const char* flowname = procflow_.name();
    const uiString cptn = tr("Madagascar processing [%1]")
        .arg( flowname && *flowname ? flowname : "New Flow" );
    setCaption( cptn );
}


void uiMadagascarMain::exportFlow( CallBacker* )
{
    const MultiID dirid( -1, ODMad::sKeyMadSelKey() );
    IOM().to( dirid );
    uiFileDialog dlg( this, false );
    dlg.setDirectory( IOM().curDirName() );
    if ( !dlg.go() ) return;
    uiMSG().error( toUiString("export needs implementation" ) );
}


bool uiMadagascarMain::rejectOK( CallBacker* )
{
    windowHide.trigger();
    return true;
}


bool uiMadagascarMain::fillPar()
{
    uiString errmsg;
    if (!procflow_.isOK(errmsg))
	mErrRet(errmsg)

    IOPar& iop =  batchfld_->jobSpec().pars_;
    iop.setEmpty();
    procflow_.fillPar( iop );

    iop.set( sKeySeisOutIDKey, "Output.ID" );
    return true;
}


bool uiMadagascarMain::acceptOK( CallBacker* )
{
    return fillPar() && batchfld_->start();
}
