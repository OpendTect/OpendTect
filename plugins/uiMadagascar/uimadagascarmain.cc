
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: uimadagascarmain.cc,v 1.10 2007-07-05 15:33:06 cvsbert Exp $";

#include "uimadagascarmain.h"
#include "uimadbldcmd.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uimenu.h"
#include "uitoolbar.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uimsg.h"
#include "seistrctr.h"
#include "seistrcsel.h"
#include "seispsioprov.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "pixmap.h"
#include "survinfo.h"
#include "filegen.h"


uiMadagascarMain::uiMadagascarMain( uiParent* p )
	: uiDialog( p, Setup( "Madagascar processing",
			      "Processing flow",
			      "0.0.0").menubar(true) )
	, in3dctio_(*mMkCtxtIOObj(SeisTrc)), in2dctio_(*mMkCtxtIOObj(SeisTrc))
	, out3dctio_(*mMkCtxtIOObj(SeisTrc)), out2dctio_(*mMkCtxtIOObj(SeisTrc))
	, inpsctio_(*mMkCtxtIOObj(SeisPS)), outpsctio_(*mMkCtxtIOObj(SeisPS))
	, inpseis3dfld_(0), inpseis2dfld_(0), inpseispsfld_(0)
	, outseis3dfld_(0), outseis2dfld_(0), outseispsfld_(0)
	, subsel3dfld_(0), subsel2dfld_(0), subselpsfld_(0)
    	, idx3d_(-1), idx2d_(-1), idxps_(-1), idxmad_(-1)
{
    setCtrlStyle( uiDialog::DoAndStay );
    createMenus();

    BufferStringSet seistypes;
#   define mAdd(s,idx) { seistypes.add( s ); idx = seistypes.size() - 1; }
    if ( SI().has3D() ) mAdd( "3D cube", idx3d_ );
    if ( SI().has2D() ) mAdd( "2D line", idx2d_ );
    if ( SI().has3D() ) mAdd( "Pre-Stack data", idxps_ );
    mAdd( "Madagascar file", idxmad_ );
    mAdd( "None", idxnone_ );

    uiGroup* inpgrp = crInpGroup( seistypes );

    uiSeparator* sep = new uiSeparator( this, "Hor sep 1", true );
    sep->attach( stretchedBelow, inpgrp, -2 );

    uiGroup* procgrp = crProcGroup();
    procgrp->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "Hor sep 2", true );
    sep->attach( stretchedBelow, procgrp, -2 );

    uiGroup* outgrp = crOutGroup( seistypes );
    outgrp->attach( alignedWith, inpgrp );
    outgrp->attach( ensureBelow, sep );

    finaliseDone.notify( mCB(this,uiMadagascarMain,initWin) );
}


uiMadagascarMain::~uiMadagascarMain()
{
    delete in3dctio_.ioobj; delete &in3dctio_;
    delete in2dctio_.ioobj; delete &in2dctio_;
    delete inpsctio_.ioobj; delete &inpsctio_;
    delete out3dctio_.ioobj; delete &out3dctio_;
    delete out2dctio_.ioobj; delete &out2dctio_;
    delete outpsctio_.ioobj; delete &outpsctio_;
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
    mInsertItem( "&Import flow ...", importFlow );
    mInsertItem( "&Export flow ...", exportFlow );
    menubar->insertItem( mnu );

    uiToolBar* toolbar = new uiToolBar( this, "Flow tools" );
    mAddButton( "newflow.png", newFlow, "Empty this flow" );
    mAddButton( "openflow.png", openFlow, "Open saved flow" );
    mAddButton( "saveflow.png", saveFlow, "Save flow" );
}


uiGroup* uiMadagascarMain::crInpGroup( const BufferStringSet& seistypes )
{
    uiGroup* inpgrp = new uiGroup( this, "Input group" );
    intypfld_ = new uiGenInput( inpgrp, "Input",
	    			 StringListInpSpec(seistypes) );
    intypfld_->valuechanged.notify( mCB(this,uiMadagascarMain,typSel) );
    if ( SI().has3D() )
    {
	inpseis3dfld_ = new uiSeisSel( inpgrp, in3dctio_, SeisSelSetup(false) );
	inpseis3dfld_->attach( alignedBelow, intypfld_ );
	inpseis3dfld_->selectiondone.notify(
			mCB(this,uiMadagascarMain,inpSel) );
	subsel3dfld_ = new uiSeis3DSubSel( inpgrp, true );
	subsel3dfld_->attach( alignedBelow, inpseis3dfld_ );
    }
    if ( SI().has2D() )
    {
	inpseis2dfld_ = new uiSeisSel( inpgrp, in2dctio_, SeisSelSetup(true) );
	inpseis2dfld_->attach( alignedBelow, intypfld_ );
	inpseis2dfld_->selectiondone.notify(
			mCB(this,uiMadagascarMain,inpSel) );
	subsel2dfld_ = new uiSeis2DSubSel( inpgrp, false, false );
	subsel2dfld_->attach( alignedBelow, inpseis2dfld_ );
    }
    if ( SI().has3D() )
    {
	inpseispsfld_ = new uiIOObjSel( inpgrp, inpsctio_ );
	inpseispsfld_->attach( alignedBelow, intypfld_ );
	inpseispsfld_->selectiondone.notify(
			mCB(this,uiMadagascarMain,inpSel) );
	subselpsfld_ = subsel3dfld_;
    }
    inpmadfld_ = new uiFileInput( inpgrp, "Input file", uiFileInput::Setup() );
    inpmadfld_->attach( alignedBelow, intypfld_ );
    subselmadfld_ = new uiGenInput( inpgrp, "sfheaderwindow parameters" );
    subselmadfld_->attach( alignedBelow, inpmadfld_ );
    subselmadlbl_ = new uiLabel( inpgrp, "[Empty=All]" );
    subselmadlbl_->attach( rightOf, subselmadfld_ );

    inpgrp->setHAlignObj( inpseis3dfld_ ? inpseis3dfld_ : inpseis2dfld_ );
    return inpgrp;
}


uiGroup* uiMadagascarMain::crProcGroup()
{
    uiGroup* procgrp = new uiGroup( this, "Proc group" );
    const CallBack butpushcb( mCB(this,uiMadagascarMain,butPush) );

    procsfld_ = new uiListBox( procgrp, "Procs fld" );
    procsfld_->setPrefHeightInChar( 8 );
    procsfld_->selectionChanged.notify( mCB(this,uiMadagascarMain,selChg) );
    procsfld_->doubleClicked.notify( mCB(this,uiMadagascarMain,dClick) );

    addbut_ = new uiPushButton( procgrp, "&Add", butpushcb, false );
    addbut_->setToolTip( "Add command to flow" );
    addbut_->setPrefWidthInChar( 10 );
    addbut_->attach( rightOf, procsfld_ );

    editbut_ = new uiPushButton( procgrp, "&Edit", butpushcb, false );
    editbut_->setToolTip( "Edit current command" );
    editbut_->setPrefWidthInChar( 10 );
    editbut_->attach( alignedBelow, addbut_ );

    upbut_ = new uiPushButton( procgrp, "", ioPixmap("uparrow.png"),
	    			butpushcb, true );
    upbut_->setToolTip( "Move current command up" );
    upbut_->attach( alignedBelow, editbut_ );
    upbut_->setPrefWidthInChar( 4 );
    downbut_ = new uiPushButton( procgrp, "", ioPixmap("downarrow.png"),
	    			butpushcb, true );
    downbut_->setToolTip( "Move current command down" );
    downbut_->attach( rightAlignedBelow, editbut_ );
    downbut_->setPrefWidthInChar( 4 );

    uiSeparator* bsep = new uiSeparator( procgrp, "Small hor sep", true );
    bsep->attach( alignedBelow, upbut_ ); bsep->attach( widthSameAs, editbut_ );

    rmbut_ = new uiPushButton( procgrp, "&Remove", butpushcb, true );
    rmbut_->setToolTip( "Remove current command from flow" );
    rmbut_->setPrefWidthInChar( 10 );
    rmbut_->attach( alignedWith, editbut_ ); rmbut_->attach( ensureBelow, bsep);

    procgrp->setHAlignObj( addbut_ );
    return procgrp;
}


uiGroup* uiMadagascarMain::crOutGroup( const BufferStringSet& seistypes )
{

    uiGroup* outgrp = new uiGroup( this, "Output group" );
    outtypfld_ = new uiGenInput( outgrp, "Output",
	    			 StringListInpSpec(seistypes) );
    outtypfld_->valuechanged.notify( mCB(this,uiMadagascarMain,typSel) );
    out2dctio_.ctxt.forread = out3dctio_.ctxt.forread
			    = outpsctio_.ctxt.forread = false;
    if ( SI().has3D() )
    {
	outseis3dfld_ = new uiSeisSel( outgrp, out3dctio_, SeisSelSetup(false));
	outseis3dfld_->attach( alignedBelow, outtypfld_ );
    }
    if ( SI().has2D() )
    {
	outseis2dfld_ = new uiSeisSel( outgrp, out2dctio_, SeisSelSetup(true) );
	outseis2dfld_->attach( alignedBelow, outtypfld_ );
    }
    if ( SI().has3D() )
    {
	outseispsfld_ = new uiIOObjSel( outgrp, outpsctio_ );
	outseispsfld_->attach( alignedBelow, outtypfld_ );
    }
    outmadfld_ = new uiFileInput( outgrp, "Output file",
	    			  uiFileInput::Setup().forread(false) );
    outmadfld_->attach( alignedBelow, outtypfld_ );

    outgrp->setHAlignObj( outseis3dfld_ ? outseis3dfld_ : outseis2dfld_ );
    return outgrp;
}


void uiMadagascarMain::initWin( CallBacker* )
{
    typSel( intypfld_ ); typSel( outtypfld_ );
    butPush( 0 );
}


void uiMadagascarMain::inpSel( CallBacker* cb )
{
    if ( cb == inpmadfld_ ) return;

    const bool isps = cb == inpseispsfld_;
    const bool is2d = cb == inpseis2dfld_;
    uiSeisSubSel* inpsubsel = subsel2dfld_;
    if ( !is2d ) inpsubsel = isps ? subselpsfld_ : subsel3dfld_;
    CtxtIOObj& ctio = is2d ? in2dctio_ : (isps ? inpsctio_ : in3dctio_);

    if ( !ctio.ioobj )
	inpsubsel->clear();
    else
	inpsubsel->setInput( *ctio.ioobj );
}


void uiMadagascarMain::typSel( CallBacker* cb )
{
    if ( cb == outtypfld_ )
    {
	dispFlds( outtypfld_->getIntValue(),
		  outseis3dfld_, outseis2dfld_, outseispsfld_, outmadfld_ );
	return;
    }

    const int choice = intypfld_->getIntValue();
    dispFlds( choice, inpseis3dfld_, inpseis2dfld_, inpseispsfld_, inpmadfld_ );
    if ( subsel3dfld_ )
	subsel3dfld_->display( choice == idx3d_ || choice == idxps_ );
    if ( subsel2dfld_ )
	subsel2dfld_->display( choice == idx2d_ );
    subselmadfld_->display( choice == idxmad_ );
    subselmadlbl_->display( choice == idxmad_ );
}


void uiMadagascarMain::dispFlds( int choice, uiSeisSel* fld3d, uiSeisSel* fld2d,
				 uiIOObjSel* fldps, uiFileInput* fldmad )
{
    if ( fld3d ) fld3d->display( choice == idx3d_ );
    if ( fld2d ) fld2d->display( choice == idx2d_ );
    if ( fldps ) fldps->display( choice == idxps_ );
    fldmad->display( choice == idxmad_ );
}


void uiMadagascarMain::dClick( CallBacker* )
{
    butPush( editbut_ );
}


void uiMadagascarMain::butPush( CallBacker* cb )
{
    mDynamicCastGet(uiPushButton*,pb,cb)
    int curidx = procsfld_->currentItem();
    const int sz = procsfld_->size();

    if ( pb == rmbut_ )
    {
	if ( curidx < 0 ) return;
	procsfld_->removeItem( curidx );
	if ( curidx >= procsfld_->size() )
	    curidx--;
    }
    else if ( pb == addbut_ || pb == editbut_ )
    {
	const bool isadd = pb == addbut_;
	if ( !isadd && curidx < 0 ) return;

	BufferString cmd( isadd ? "" : procsfld_->getText() );
	uiMadagascarBldCmd dlg( this, cmd, true );
	dlg.applyReq.notify( mCB(this,uiMadagascarMain,immediateAdd) );
	if ( dlg.go() )
	{
	    if ( !isadd )
		procsfld_->setItemText( curidx, dlg.command() );
	    else
	    {
		const int sz = procsfld_->size();
		const BufferString lastcmd(
				sz ? procsfld_->textOfItem(sz-1) : "" );
		const BufferString newcmd( dlg.command() );
		if ( newcmd != lastcmd )
		    procsfld_->addItem( newcmd );
		curidx = procsfld_->size() - 1;
	    }
	}
    }
    else if ( pb == upbut_ || pb == downbut_ )
    {
	if ( curidx < 0 ) return;
	const bool isup = pb == upbut_;
	const int newcur = curidx + (isup ? -1 : 1);
	if ( newcur >= 0 && newcur < sz )
	{
	    BufferString tmp( procsfld_->textOfItem(newcur) );
	    procsfld_->setItemText( newcur, procsfld_->getText() );
	    procsfld_->setItemText( curidx, tmp );
	    curidx = newcur;
	}
    }

    if ( curidx >= 0 )
	procsfld_->setCurrentItem( curidx );
    setButStates();
}


void uiMadagascarMain::immediateAdd( CallBacker* c )
{
    mDynamicCastGet(uiMadagascarBldCmd*,dlg,c)
    if ( !dlg ) return;
    procsfld_->addItem( dlg->command() );
}


void uiMadagascarMain::setButStates()
{
    const bool havesel = !procsfld_->isEmpty();
    editbut_->setSensitive( havesel );
    rmbut_->setSensitive( havesel );
    selChg( 0 );
}


void uiMadagascarMain::selChg( CallBacker* )
{
    const int curidx = procsfld_->isEmpty() ? -1 : procsfld_->currentItem();
    const int sz = procsfld_->size();
    upbut_->setSensitive( sz > 1 && curidx > 0 );
    downbut_->setSensitive( sz > 1 && curidx >= 0 && curidx < sz-1 );
}


void uiMadagascarMain::newFlow( CallBacker* )
{
}


void uiMadagascarMain::openFlow( CallBacker* )
{
}


void uiMadagascarMain::saveFlow( CallBacker* )
{
}


void uiMadagascarMain::importFlow( CallBacker* )
{
}


void uiMadagascarMain::exportFlow( CallBacker* )
{
}

#define mErrRet(s1,s2,s3) { uiMSG().error(s1,s2,s3); return false; }

bool uiMadagascarMain::ioOK( int choice, bool inp )
{
    if ( choice == idx3d_ || choice == idx2d_ )
    {
	uiSeisSel* ss = choice == idx3d_ ? (inp?inpseis3dfld_:outseis3dfld_)
					 : (inp?inpseis2dfld_:outseis2dfld_);
	if ( !ss->commitInput(!inp) )
	    mErrRet("Please select the ", inp?"input":"output", " seismics")
	if ( !inp && choice == 0 && out3dctio_.ioobj->implExists(false)
	   && !uiMSG().askGoOn("Output cube exists. Overwrite?") )
	    return false;
    }
    else if ( choice == idxps_ )
    {
	uiIOObjSel* objsel = inp ? inpseispsfld_ : outseispsfld_;
	if ( !objsel->commitInput(!inp) )
	    mErrRet("Please select the ", inp?"input":"output", " data store")
    }
    else if ( choice == idxmad_ )
    {
	uiFileInput* madfld = inp ? inpmadfld_ : outmadfld_;
	const BufferString fnm( madfld->fileName() );
	if ( fnm.isEmpty() || (inp && !File_exists(fnm)) )
	    mErrRet("Please specify the ", inp?"input":"output", " file")
    }

    return true;
}

bool uiMadagascarMain::acceptOK( CallBacker* )
{
    const int inpchoice = intypfld_->getIntValue();
    const int outchoice = outtypfld_->getIntValue();
    if ( !ioOK(inpchoice,true) || !ioOK(outchoice,false) )
	return false;

    return false;
}
