
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: uimadagascarmain.cc,v 1.4 2007-06-12 10:24:46 cvsbert Exp $";

#include "uimadagascarmain.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uifileinput.h"
#include "uigeninputdlg.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uiseparator.h"
#include "uimsg.h"
#include "seistrctr.h"
#include "seistrcsel.h"
#include "seispsioprov.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "survinfo.h"
#include "filegen.h"


uiMadagascarMain::uiMadagascarMain( uiParent* p )
	: uiDialog( p, Setup( "Madagacar processing",
			      "Processing flow",
			      "0.0.0") )
	, in3dctio_(*mMkCtxtIOObj(SeisTrc)), in2dctio_(*mMkCtxtIOObj(SeisTrc))
	, out3dctio_(*mMkCtxtIOObj(SeisTrc)), out2dctio_(*mMkCtxtIOObj(SeisTrc))
	, inpsctio_(*mMkCtxtIOObj(SeisPS)), outpsctio_(*mMkCtxtIOObj(SeisPS))
	, inpseis3dfld_(0), inpseis2dfld_(0), inpseispsfld_(0)
	, outseis3dfld_(0), outseis2dfld_(0), outseispsfld_(0)
	, subsel3dfld_(0), subsel2dfld_(0), subselpsfld_(0)
    	, idx3d_(-1), idx2d_(-1), idxps_(-1), idxmad_(-1)
{
    BufferStringSet seistypes;
#   define mAdd(s,idx) { seistypes.add( s ); idx = seistypes.size() - 1; }
    if ( SI().has3D() ) mAdd( "3D cube", idx3d_ );
    if ( SI().has2D() ) mAdd( "2D line", idx2d_ );
    if ( SI().has3D() ) mAdd( "Pre-Stack data", idxps_ );
    mAdd( "Madagascar file", idxmad_ );
    mAdd( "None", idxnone_ );

    uiGroup* inpgrp = new uiGroup( this, "Input group" );
    intypfld_ = new uiGenInput( inpgrp, "Input",
	    			 StringListInpSpec(seistypes) );
    intypfld_->valuechanged.notify( mCB(this,uiMadagascarMain,typSel) );
    SeisSelSetup sss3d; sss3d.is2d_ = false;
    SeisSelSetup sss2d; sss2d.is2d_ = true;
    if ( SI().has3D() )
    {
	inpseis3dfld_ = new uiSeisSel( inpgrp, in3dctio_, sss3d );
	inpseis3dfld_->attach( alignedBelow, intypfld_ );
	inpseis3dfld_->selectiondone.notify(
			mCB(this,uiMadagascarMain,inpSel) );
	subsel3dfld_ = new uiSeis3DSubSel( inpgrp, true );
	subsel3dfld_->attach( alignedBelow, inpseis3dfld_ );
    }
    if ( SI().has2D() )
    {
	inpseis2dfld_ = new uiSeisSel( inpgrp, in2dctio_, sss2d );
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
    inpgrp->setHAlignObj( inpseis3dfld_ ? inpseis3dfld_ : inpseis2dfld_ );

    uiSeparator* sep = new uiSeparator( this, "Hor sep 1", true );
    sep->attach( stretchedBelow, inpgrp, -2 );

    const CallBack butpushcb( mCB(this,uiMadagascarMain,butPush) );
    uiGroup* procgrp = new uiGroup( this, "Proc group" );
    procsfld_ = new uiListBox( procgrp, "Procs fld" );
    addbut_ = new uiPushButton( procgrp, "&Add", butpushcb, false );
    addbut_->setPrefWidthInChar( 10 );
    addbut_->attach( rightOf, procsfld_ );
    editbut_ = new uiPushButton( procgrp, "&Edit", butpushcb, false );
    editbut_->setPrefWidthInChar( 10 );
    editbut_->attach( alignedBelow, addbut_ );
    rmbut_ = new uiPushButton( procgrp, "&Remove", butpushcb, false );
    rmbut_->setPrefWidthInChar( 10 );
    rmbut_->attach( alignedBelow, editbut_ );
    procgrp->setHAlignObj( addbut_ );
    procgrp->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "Hor sep 2", true );
    sep->attach( stretchedBelow, procgrp, -2 );

    uiGroup* outgrp = new uiGroup( this, "Output group" );
    outtypfld_ = new uiGenInput( outgrp, "Output",
	    			 StringListInpSpec(seistypes) );
    outtypfld_->valuechanged.notify( mCB(this,uiMadagascarMain,typSel) );
    out2dctio_.ctxt.forread = out3dctio_.ctxt.forread
			    = outpsctio_.ctxt.forread = false;
    if ( SI().has3D() )
    {
	outseis3dfld_ = new uiSeisSel( outgrp, out3dctio_, sss3d );
	outseis3dfld_->attach( alignedBelow, outtypfld_ );
    }
    if ( SI().has2D() )
    {
	outseis2dfld_ = new uiSeisSel( outgrp, out2dctio_, sss2d );
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
}


void uiMadagascarMain::dispFlds( int choice, uiSeisSel* fld3d, uiSeisSel* fld2d,
				 uiIOObjSel* fldps, uiFileInput* fldmad )
{
    if ( fld3d ) fld3d->display( choice == idx3d_ );
    if ( fld2d ) fld2d->display( choice == idx2d_ );
    if ( fldps ) fldps->display( choice == idxps_ );
    fldmad->display( choice == idxmad_ );
}


void uiMadagascarMain::butPush( CallBacker* cb )
{
    mDynamicCastGet(uiPushButton*,pb,cb)
    if ( pb == rmbut_ )
    {
	const int curidx = procsfld_->currentItem();
	procsfld_->removeItem( curidx );
	if ( curidx > 0 && curidx >= procsfld_->size() )
	    procsfld_->setCurrentItem( curidx-1 );
    }
    else if ( pb == addbut_ || pb == editbut_ )
    {
	const bool isadd = pb == addbut_;
	BufferString curcmd( isadd ? "" : procsfld_->getText() );
	uiGenInputDlg dlg( this, "Madagascar command specification",
			   "Command line", new StringInpSpec(curcmd) );
	if ( dlg.go() )
	{
	    curcmd = dlg.text( 0 );
	    if ( isadd )
		procsfld_->addItem( curcmd );
	    else
		procsfld_->setItemText( procsfld_->currentItem(), curcmd );
	}
    }
    setButStates();
}


void uiMadagascarMain::setButStates()
{
    const bool havesel = !procsfld_->isEmpty() && procsfld_->currentItem() >= 0;
    editbut_->setSensitive( havesel );
    rmbut_->setSensitive( havesel );
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

    // MadagascarExecutor exec;
    // exec.set ...
    // uiExecutor dlg( this, tst_ );
    // return dlg.go();
    return true;
}
