
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: uimadagascarmain.cc,v 1.22 2008-06-05 12:02:08 cvsraman Exp $";

#include "uimadagascarmain.h"
#include "uimadiosel.h"
#include "uimadbldcmd.h"
#include "madprocflow.h"
#include "madprocflowtr.h"
#include "madprocexec.h"
#include "madio.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uigeninput.h"
#include "uimenu.h"
#include "uitoolbar.h"
#include "uiseparator.h"
#include "uiioobjsel.h"
#include "uifiledlg.h"
#include "uimsg.h"
#include "cubesampling.h"
#include "pixmap.h"
#include "keystrs.h"
#include "ioman.h"
#include "oddirs.h"
#include "seisioobjinfo.h"
#include "strmprov.h"

const char* sKeySeisOutIDKey = "Output Seismics Key";

uiMadagascarMain::uiMadagascarMain( uiParent* p )
	: uiFullBatchDialog(p,Setup("Madagascar processing").menubar(true)
				   .procprognm("odmadexec") )
	, ctio_(*mMkCtxtIOObj(ODMadProcFlow))
	, bldfld_(0)
	, needsave_(false)
{
    setCtrlStyle( uiDialog::DoAndStay );
    setHelpID( "103.5.0" );
    addStdFields( false, false );
    createMenus();

    uiGroup* maingrp = new uiGroup( uppgrp_, "Main group" );

    infld_ = new uiMadIOSel( maingrp, true );
    infld_->selectionMade.notify( mCB(this,uiMadagascarMain,inpSel) );

    uiGroup* procgrp = crProcGroup( maingrp );
    procgrp->attach( alignedBelow, infld_ );

    outfld_ = new uiMadIOSel( maingrp, false );
    outfld_->selectionMade.notify( mCB(this,uiMadagascarMain,inpSel) );
    outfld_->attach( alignedBelow, procgrp );

    bldfld_ = new uiMadagascarBldCmd( uppgrp_ );
    bldfld_->cmdAvailable.notify( mCB(this,uiMadagascarMain,cmdAvail) );

    uiSeparator* sep = new uiSeparator( uppgrp_, "VSep", false );
    sep->attach( rightTo, maingrp );
    bldfld_->attach( rightTo, sep );
    uppgrp_->setHAlignObj( sep );

    setParFileNmDef( "Mad_Proc" );
    finaliseDone.notify( mCB(this,uiMadagascarMain,setButStates) );
}


uiMadagascarMain::~uiMadagascarMain()
{
    delete ctio_.ioobj; delete &ctio_;
}


#define mInsertItem( txt, func ) \
    mnu->insertItem( new uiMenuItem(txt,mCB(this,uiMadagascarMain,func)) )
#define mAddButton(pm,func,tip) \
    toolbar->addButton( pm, mCB(this,uiMadagascarMain,func), tip )

void uiMadagascarMain::createMenus()
{
    uiMenuBar* menubar = menuBar();
    if ( !menubar ) { pErrMsg("huh?"); return; }

    uiPopupMenu* mnu = new uiPopupMenu( this, "&File" );
    mInsertItem( "&New flow ...", newFlow );
    mInsertItem( "&Open flow ...", openFlow );
    mInsertItem( "&Save flow ...", saveFlow );
    mnu->insertSeparator();
    mInsertItem( "&Export flow ...", exportFlow );
    mnu->insertSeparator();
    mInsertItem( "&Quit", reject );
    menubar->insertItem( mnu );

    uiToolBar* toolbar = new uiToolBar( this, "Flow tools" );
    mAddButton( "newflow.png", newFlow, "Empty this flow" );
    mAddButton( "openflow.png", openFlow, "Open saved flow" );
    mAddButton( "saveflow.png", saveFlow, "Save flow" );
}


uiGroup* uiMadagascarMain::crProcGroup( uiGroup* grp )
{
    uiGroup* procgrp = new uiGroup( grp, "Proc group" );
    const CallBack butpushcb( mCB(this,uiMadagascarMain,butPush) );

    uiLabeledListBox* pfld = new uiLabeledListBox( procgrp, "FLOW", false,
						   uiLabeledListBox::LeftMid );
    procsfld_ = pfld->box();
    procsfld_->setPrefWidthInChar( 20 );
    procsfld_->selectionChanged.notify( mCB(this,uiMadagascarMain,selChg) );

    uiButtonGroup* bgrp = new uiButtonGroup( procgrp, "", false );
    bgrp->displayFrame( true );
    upbut_ = new uiToolButton( bgrp, "Up button", butpushcb );
    upbut_->setArrowType( uiToolButton::UpArrow );
    upbut_->setToolTip( "Move current command up" );
    downbut_ = new uiToolButton( bgrp, "Down button", butpushcb );
    downbut_->setArrowType( uiToolButton::DownArrow );
    downbut_->setToolTip( "Move current command down" );
    rmbut_ = new uiToolButton( bgrp, "Remove button", ioPixmap("trashcan.png"),
	    			butpushcb );
    rmbut_->setToolTip( "Remove current command from flow" );
    bgrp->attach( centeredBelow, pfld );

    procgrp->setHAlignObj( pfld );
    return procgrp;
}


void uiMadagascarMain::inpSel( CallBacker* )
{
    needsave_ = true;
    IOPar inpar;
    infld_->fillPar( inpar );
    outfld_->useParIfNeeded( inpar );

    IOPar outpar;
    outfld_->fillPar( outpar );
    BufferString inptyp = inpar.find( sKey::Type );
    BufferString outptyp = outpar.find( sKey::Type );

    if ( inptyp==Seis::nameOf(Seis::Vol) && outptyp==Seis::nameOf(Seis::Vol) )
	singmachfld_->setSensitive( true );
    else
    {
	singmachfld_->setValue( true );
	singmachfld_->setSensitive( false );
    }
}

#undef mErrRet
#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiMadagascarMain::getFlow( ODMad::ProcFlow& pf )
{
    if ( !infld_->fillPar(pf.input()) )
	mErrRet("Please specify input parameters");
    if ( !outfld_->fillPar(pf.output()) )
	mErrRet("Please specify output parameters");

    pf.procs().deepErase();
    const int nrprocs = procsfld_->size();
    for ( int idx=0; idx<nrprocs; idx++ )
	pf.addProc( procsfld_->textOfItem(idx) );

    const bool issinglemach = singmachfld_->getBoolValue();
    if ( !issinglemach )
    {
	IOPar& inpar = pf.input();
	IOPar& outpar = pf.output();
	BufferString inptyp = inpar.find( sKey::Type );
	BufferString outptyp = outpar.find( sKey::Type );
	BufferString vol3dtypnm = Seis::nameOf( Seis::Vol );
	if ( inptyp != vol3dtypnm || outptyp != vol3dtypnm )
	    mErrRet("Multi-Machine processing supported for 3D volumes only")
	
	IOPar* subselpar = inpar.subselect( sKey::Subsel );
	if ( !subselpar )
	    subselpar = new IOPar;

	BufferString subseltyp = subselpar->find( sKey::Type );
	if ( subseltyp != sKey::Range )
	{
	    MultiID inpid;
	    if ( !inpar.get(sKey::ID,inpid) )
		mErrRet("Input ID missing")

	    const SeisIOObjInfo info( inpid );
	    CubeSampling cs;
	    info.getRanges( cs );
	    subselpar->set( sKey::Type, sKey::Range );
	    cs.fillPar( *subselpar );
	    inpar.mergeComp( *subselpar, sKey::Subsel );
	}

	outpar.mergeComp( *subselpar, sKey::Subsel );
	delete subselpar;
    }

    return true;
}


void uiMadagascarMain::setFlow( const ODMad::ProcFlow& pf )
{
    infld_->usePar( pf.input() );
    outfld_->usePar( pf.output() );
    procsfld_->empty();
    for ( int idx=0; idx<pf.procs().size(); idx++ )
	procsfld_->addItem( pf.procs().get(idx) );
}


void uiMadagascarMain::cmdAvail( CallBacker* cb )
{
    const BufferString cmd = bldfld_->command();
    if ( cmd.isEmpty() ) return;

    if ( bldfld_->isAdd() )
    {
	procsfld_->addItem( cmd );
	needsave_ = true;
	procsfld_->setCurrentItem( procsfld_->size() - 1 );
    }
    else
    {
	const int curidx = procsfld_->currentItem();
	if ( curidx < 0 ) return;
	needsave_ = true;
	procsfld_->setItemText( curidx, cmd );
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
	    procsfld_->setItemText( newcur, procsfld_->getText() );
	    procsfld_->setItemText( curidx, tmp );
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

    if ( cb == this || curidx < 0 || !bldfld_ ) return;
    bldfld_->setCmd( procsfld_->textOfItem(curidx) );
}


void uiMadagascarMain::newFlow( CallBacker* )
{
    ODMad::ProcFlow pf;
    setFlow( pf );
}


void uiMadagascarMain::openFlow( CallBacker* )
{
    if ( needsave_ )
    {
	if ( !uiMSG().askGoOn("Current flow has not been saved, continue?") )
	    return;
    }

    ctio_.ctxt.forread = true;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( dlg.go() )
    {
	ctio_.setObj( dlg.ioObj()->clone() );
	BufferString emsg; ODMad::ProcFlow pf;
	if ( !ODMadProcFlowTranslator::retrieve(pf,ctio_.ioobj,emsg) )
	    uiMSG().error( emsg );
	else
	{
	    setFlow( pf );
	    needsave_ = false;
	}
    }
}


void uiMadagascarMain::saveFlow( CallBacker* )
{
    ctio_.ctxt.forread = false;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( dlg.go() )
    {
	ctio_.setObj( dlg.ioObj()->clone() );
	BufferString emsg; ODMad::ProcFlow pf; getFlow( pf );
	if ( !ODMadProcFlowTranslator::store(pf,ctio_.ioobj,emsg) )
	    uiMSG().error( emsg );
	else
	    needsave_ = false;
    }
}


void uiMadagascarMain::exportFlow( CallBacker* )
{
    IOM().to( ODMad::sKeyMadSelKey );
    uiFileDialog dlg( this, false );
    dlg.setDirectory( IOM().curDir() );
    if ( !dlg.go() ) return;
    uiMSG().error( "Export needs implementation" );
}


bool uiMadagascarMain::fillPar( IOPar& iop )
{
    ODMad::ProcFlow pf;
    if ( !getFlow(pf) )
	return false;

    pf.fillPar( iop );
    iop.set( sKeySeisOutIDKey, "Output.ID" );
    return true;
}


bool uiMadagascarMain::rejectOK( CallBacker* )
{
    if ( needsave_ )
    {
	if ( !uiMSG().askGoOn("Current flow has not been saved, quit anyway?") )
	    return false;
    }

    return true;
}
