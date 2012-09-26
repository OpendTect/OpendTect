/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2009
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiseispreloadmgr.h"
#include "seisioobjinfo.h"
#include "seiscbvs.h"
#include "seispsioprov.h"
#include "seispreload.h"
#include "seis2dline.h"
#include "strmprov.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "iodirentry.h"
#include "ascstream.h"
#include "ptrman.h"
#include "filepath.h"
#include "file.h"
#include "datapack.h"
#include "survinfo.h"
#include "preloads.h"

#include "uimsg.h"
#include "uilistbox.h"
#include "uitoolbutton.h"
#include "uibuttongroup.h"
#include "uisplitter.h"
#include "uitextedit.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uitaskrunner.h"
#include "uiselsurvranges.h"

const char* cannotloadstr = "Cannot load ";

uiSeisPreLoadMgr::uiSeisPreLoadMgr( uiParent* p )
    : uiDialog(p,Setup("Pre-load manager","Pre-loaded seismic data",
			"103.0.13"))
{
    setCtrlStyle( LeaveOnly );
    uiGroup* topgrp = new uiGroup( this, "Top group" );
    listfld_ = new uiListBox( topgrp, "Loaded entries" );
    listfld_->selectionChanged.notify( mCB(this,uiSeisPreLoadMgr,selChg) );
    topgrp->setHAlignObj( listfld_ );

    const bool has2d = SI().has2D();
    const bool has3d = SI().has3D();
    uiButtonGroup* bgrp = new uiButtonGroup( topgrp, "Manip buttons" );
    bgrp->attach( rightOf, listfld_ );
#   define mAddBut(s,fn) \
    new uiPushButton( bgrp, s, mCB(this,uiSeisPreLoadMgr,fn), false )
    if ( has3d )
	mAddBut("Add Cube",cubeLoadPush);
    if ( has2d )
	mAddBut("Add Lines",linesLoadPush);
    if ( has3d )
    {
	if ( has2d )
	    mAddBut("Add 3D Pre-Stack data",ps3DPush);
	else
	    mAddBut("Add Pre-Stack data",ps3DPush);
    }
    if ( has2d )
    {
	if ( has3d )
	    mAddBut("Add 2D Pre-Stack lines",ps2DPush);
	else
	    mAddBut("Add Pre-Stack data",ps2DPush);
    }
    mAddBut("Unload Selected",unloadPush);

    uiToolButton* opentb = new uiToolButton( topgrp, "openpreload",
	    "Retrieve pre-loads", mCB(this,uiSeisPreLoadMgr,openPush) );
    opentb->attach( leftAlignedBelow, listfld_ );
    uiToolButton* savetb = new uiToolButton( topgrp, "savepreload",
	    "Save pre-loads", mCB(this,uiSeisPreLoadMgr,savePush) );
    savetb->attach( rightAlignedBelow, listfld_ );

    uiGroup* infogrp = new uiGroup( this, "Info Group" );
    infofld_ = new uiTextEdit( infogrp, "Info" );
    infofld_->setPrefHeightInChar( 5 );

    uiSplitter* spl = new uiSplitter( this, "Splitter", false );
    spl->addGroup( topgrp );
    spl->addGroup( infogrp );

    postFinalise().notify( mCB(this,uiSeisPreLoadMgr,fullUpd) );
}


void uiSeisPreLoadMgr::fullUpd( CallBacker* )
{
    fillList();
}


void uiSeisPreLoadMgr::fillList()
{
    listfld_->setEmpty();
    StreamProvider::getPreLoadedIDs( ids_ );
    if ( ids_.isEmpty() ) return;

    for ( int idx=0; idx<ids_.size(); idx++ )
    {
	const MultiID ky( ids_.get(idx) );
	PtrMan<IOObj> ioobj = IOM().get( ky );
	if ( !ioobj )
	{
	    Seis::PreLoader(ky).unLoad();
	    ids_.remove( idx ); idx--;
	    continue;
	}
	listfld_->addItem( ioobj->name() );
    }

    listfld_->setCurrentItem( 0 );
}


void uiSeisPreLoadMgr::selChg( CallBacker* )
{
    const int selidx = listfld_->currentItem();
    if ( ids_.isEmpty() || selidx < 0 )
	{ infofld_->setText(""); return; }

    const MultiID ky( ids_.get(selidx).buf() );
    Seis::PreLoader spl( ky );
    PtrMan<IOObj> ioobj = spl.getIOObj();
    if ( !ioobj )
	{ infofld_->setText(spl.errMsg()); return; }

    SeisIOObjInfo ioinf( *ioobj );
    if ( !ioinf.isOK() )
	{ infofld_->setText("Internal error: IOObj not OK"); return; }
    const Seis::GeomType gt = ioinf.geomType();
    BufferStringSet fnms;
    StreamProvider::getPreLoadedFileNames( ky.buf(), fnms );
    if ( fnms.isEmpty() )
	{ infofld_->setText("No files"); return; }

    BufferString disptxt( "Data type: " ); disptxt += Seis::nameOf( gt );

    switch ( gt )
    {
	case Seis::Vol:
	break;
	case Seis::Line: {
	    BufferStringSet lks; spl.getLineKeys( lks );
	    disptxt += getLinesText( lks );
	} break;
	case Seis::VolPS: {
	    Interval<int> rg = spl.inlRange();
	    if ( !mIsUdf(rg.start) )
	    {
		disptxt += "\nInline range: ";
		disptxt += rg.start; disptxt += "-"; disptxt += rg.stop;
	    }
	} break;
	case Seis::LinePS:
	break;
    }

    float totmem; disptxt += getFilesText( fnms, totmem );
    totmem /= 1024; const int memmb = mNINT32(totmem);
    disptxt += "\nTotal memory in MB: "; disptxt += memmb;
    infofld_->setText( disptxt );
}


BufferString uiSeisPreLoadMgr::getLinesText( const BufferStringSet& lks ) const
{
    BufferStringSet lnms, attrnms;
    for ( int idx=0; idx<lks.size(); idx++ )
    {
	const LineKey lk( lks.get(idx) );
	lnms.addIfNew( lk.lineName() );
	attrnms.addIfNew( lk.attrName() );
    }

    BufferString txt;
    if ( lnms.isEmpty() )
	{ txt = "-"; return txt; }

    txt += "\nLine"; txt += lnms.size() < 2 ? ":" : "s:";
    for ( int iln=0; iln<lnms.size(); iln++ )
	{ txt += iln ? ", " : " "; txt += lnms.get( iln ); }
    txt += "\nAttribute"; txt += attrnms.size() < 2 ? ":" : "s:";
    for ( int iattr=0; iattr<attrnms.size(); iattr++ )
	{ txt += iattr ? ", " : " "; txt += attrnms.get( iattr ); }

    return txt;
}


BufferString uiSeisPreLoadMgr::getFilesText( const BufferStringSet& fnms,
						float& totmem ) const
{
    BufferString txt; const int nrfiles = fnms.size();

    FilePath fp( fnms.get(0) );
    txt += "\nDirectory: "; txt += fp.pathOnly();
    txt += "\nFile name"; if ( nrfiles > 1 ) txt += "s";
    txt += ": "; txt += fp.fileName();
    totmem = 0; const bool needdots = nrfiles > 4;
    for ( int idx=0; idx<nrfiles; idx++ )
    {
	const char* fnm = fnms.get(idx).buf();
	int dpid = StreamProvider::getPreLoadedDataPackID( fnm );
	totmem += DPM(DataPackMgr::BufID()).nrKBytesOf( dpid );

	if ( idx == 0 || (idx < nrfiles-1 && idx > 2) )
	    continue;

	fp.set( fnms.get(idx) );
	txt += " "; txt += fp.fileName();
	if ( needdots && idx == 2 )
	    txt += " ...";
    }
    if ( needdots )
	{ txt += " ("; txt += nrfiles; txt += " files)"; }

    return txt;
}



#define mCheckIOObjExistance( ioobj ) \
if ( !ioobj->implExists( true ) ) \
{ \
    BufferString msg = cannotloadstr; \
    msg += ioobj->name(); \
    uiMSG().error( msg.buf() ); \
    return; \
}



void uiSeisPreLoadMgr::cubeLoadPush( CallBacker* )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisTrc);
    ctio->ctxt.toselect.allowtransls_ = CBVSSeisTrcTranslator::translKey();
    uiIOObjSelDlg dlg( this, *ctio );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    mCheckIOObjExistance( dlg.ioObj() );

    Seis::PreLoader spl( dlg.ioObj()->key() );
    const char* id = spl.id().buf();
    if ( StreamProvider::isPreLoaded(id,true) )
    {
	if ( !uiMSG().askGoOn("This cube is already pre-loaded.\n"
		    	      "Do you want to re-load?") )
	    return;
	spl.unLoad();
    }

    uiTaskRunner tr( this ); spl.setRunner( tr );
    if ( !spl.loadVol() )
    {
	const char* emsg = spl.errMsg();
	if ( emsg )
	    uiMSG().error( emsg );
    }

    fullUpd( 0 );
}


class uiSeisPreLoadMgrSel2D : public uiDialog
{
public:

uiSeisPreLoadMgrSel2D( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Preload selection","Select lines/attributes",
				 "103.0.14"))
    , ctio_(*mMkCtxtIOObj(SeisTrc))
{
    ctio_.ctxt.toselect.allowtransls_ = "2D";
    IOM().to( ctio_.ctxt.getSelKey() );
    IODirEntryList del( IOM().dirPtr(), ctio_.ctxt );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	if ( del[idx]->ioobj )
	    { ctio_.setObj( del[idx]->ioobj->clone() ); break; }
    }

    lssel_ = new uiIOObjSel( this, ctio_ );
    lssel_->selectionDone.notify( mCB(this,uiSeisPreLoadMgrSel2D,lsSel) );
    uiGroup* boxgrp = new uiGroup( this, "List boxes" );
    uiLabeledListBox* lllb = new uiLabeledListBox( boxgrp, "Line(s)", true,
	    			 uiLabeledListBox::AboveMid );
    linesel_ = lllb->box();
    uiLabeledListBox* allb = new uiLabeledListBox( boxgrp, "Attribute(s)", true,
	    			 uiLabeledListBox::AboveMid );
    allb->attach( rightOf, lllb );
    attrsel_ = allb->box();
    boxgrp->setHAlignObj( allb );
    boxgrp->attach( alignedBelow, lssel_ );

    if ( ctio_.ioobj )
	lsSel(0);
}

~uiSeisPreLoadMgrSel2D()
{
    delete ctio_.ioobj; delete &ctio_;
}

void lsSel( CallBacker* )
{
    if ( !ctio_.ioobj ) return;

    lnms_.erase(); attrnms_.erase();
    Seis2DLineSet ls( *ctio_.ioobj );
    for ( int idx=0; idx<ls.nrLines(); idx++ )
    {
	lnms_.addIfNew( ls.lineName(idx) );
	attrnms_.addIfNew( ls.attribute(idx) );
    }

    linesel_->setEmpty(); attrsel_->setEmpty();
    linesel_->addItems( lnms_ ); attrsel_->addItems( attrnms_ );
    linesel_->selectAll(); attrsel_->selectAll();
}


bool acceptOK( CallBacker* )
{
    if ( !lssel_->commitInput() || !ctio_.ioobj )
    {
	uiMSG().error( "Please select a Line Set" );
	return false;
    }

    lnms_.erase(); attrnms_.erase();
    linesel_->getSelectedItems(lnms_); attrsel_->getSelectedItems(attrnms_);
    if ( lnms_.isEmpty() || attrnms_.isEmpty() )
    {
	uiMSG().error( "Please select one or more lines and attributes" );
	return false;
    }
    return true;
}

    CtxtIOObj&	ctio_;
    uiIOObjSel*	lssel_;
    uiListBox*	linesel_;
    uiListBox*	attrsel_;

    BufferStringSet	lnms_;
    BufferStringSet	attrnms_;

};


void uiSeisPreLoadMgr::linesLoadPush( CallBacker* )
{
    uiSeisPreLoadMgrSel2D dlg( this );
    if ( !dlg.go() ) return;

    mCheckIOObjExistance( dlg.ctio_.ioobj );

    Seis::PreLoader spl( dlg.ctio_.ioobj->key() );
    uiTaskRunner tr( this ); spl.setRunner( tr );
    if ( !spl.loadLines(dlg.lnms_,dlg.attrnms_) )
    {
	const char* emsg = spl.errMsg();
	if ( emsg )
	    uiMSG().error( emsg );
    }

    fullUpd( 0 );
}


void uiSeisPreLoadMgr::ps3DPush( CallBacker* )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisPS3D);
    ctio->ctxt.toselect.allowtransls_ = CBVSSeisTrcTranslator::translKey();
    uiIOObjSelDlg dlg( this, *ctio, "Select data store/part to load" );
    dlg.setCaption( "Select data store" );
    uiSelNrRange* inlrgfld = new uiSelNrRange( dlg.selGrp()->getTopGroup(),
	    				uiSelNrRange::Inl, false );
    inlrgfld->attach( centeredBelow, dlg.selGrp()->getListField() );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    mCheckIOObjExistance( dlg.ioObj() );

    Seis::PreLoader spl( dlg.ioObj()->key() );
    Interval<int> inlrg; assign(inlrg,inlrgfld->getRange());
    uiTaskRunner tr( this ); spl.setRunner( tr );
    if ( !spl.loadPS3D(&inlrg) )
    {
	const char* emsg = spl.errMsg();
	if ( emsg )
	    uiMSG().error( emsg );
    }

    fullUpd( 0 );
}


class uiSeisPreLoadMgrPS2DSel : public uiIOObjSelDlg
{
public:

uiSeisPreLoadMgrPS2DSel( uiParent* p, CtxtIOObj& ctio )
    : uiIOObjSelDlg( p, ctio, "Select data store" )
{
    setCaption( "Pre-load data" );
    uiLabeledListBox* llb = new uiLabeledListBox( selGrp(), "Lines to load",
					  true, uiLabeledListBox::AboveMid );
    lnmsfld_ = llb->box();
    llb->attach( rightOf, selGrp()->getTopGroup() );
    selGrp()->selectionChg.notify( mCB(this,uiSeisPreLoadMgrPS2DSel,dsSel) );
}

void dsSel( CallBacker* )
{
    lnmsfld_->setEmpty();
    selGrp()->processInput();
    if ( !ioObj() ) return;

    SeisIOObjInfo sii( *ioObj() );
    lnms_.erase();
    sii.getLineNames( lnms_ );
    lnmsfld_->addItems( lnms_ );
    lnmsfld_->selectAll();
}

bool acceptOK( CallBacker* )
{
    if ( !ioObj() )
    {
	uiMSG().error( "Please select a 2D Pre-Stack Data Store" );
	return false;
    }
    lnms_.erase();
    lnmsfld_->getSelectedItems( lnms_ );
    if ( lnms_.isEmpty() )
    {
	uiMSG().error( "Please select one or more lines" );
	return false;
    }

    return true;
}

    uiListBox*	lnmsfld_;

    BufferStringSet lnms_;

};


void uiSeisPreLoadMgr::ps2DPush( CallBacker* )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisPS2D);
    ctio->ctxt.toselect.allowtransls_ = CBVSSeisTrcTranslator::translKey();
    uiSeisPreLoadMgrPS2DSel dlg( this, *ctio );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    mCheckIOObjExistance( dlg.ioObj() );

    Seis::PreLoader spl( dlg.ioObj()->key() );
    uiTaskRunner tr( this ); spl.setRunner( tr );
    if ( !spl.loadPS2D(dlg.lnms_) )
    {
	const char* emsg = spl.errMsg();
	if ( emsg )
	    uiMSG().error( emsg );
    }

    fullUpd( 0 );
}


void uiSeisPreLoadMgr::unloadPush( CallBacker* )
{
    const int selidx = listfld_->currentItem();
    if ( selidx < 0 ) return;

    BufferString msg( "Unload '" );
    msg += listfld_->textOfItem( selidx );
    msg += "'?\n(This will not delete the object from disk)";
    if ( !uiMSG().askGoOn( msg ) )
	return;

    Seis::PreLoader spl( MultiID(ids_.get(selidx)) );
    spl.unLoad();

    fillList();
    int newselidx = selidx;
    if ( newselidx >= ids_.size() )
	newselidx = ids_.size() - 1;
    if ( newselidx >= 0 )
	listfld_->setCurrentItem( newselidx );
    else
	selChg( 0 );
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiSeisPreLoadMgr::openPush( CallBacker* )
{
    CtxtIOObj ctio( PreLoadsTranslatorGroup::ioContext() );
    ctio.ctxt.forread = true;
    ctio.fillDefault();
    uiIOObjSelDlg dlg( this, ctio, "Open pre-load settings" );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    const BufferString fnm( dlg.ioObj()->fullUserExpr(true) );
    delete ctio.ioobj;
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	mErrRet( BufferString("Cannot open input file:\n",fnm) )

    ascistream astrm( *sd.istrm,true );
    IOPar iop( astrm );
    if ( iop.isEmpty() )
	mErrRet( "No valid objects found" )

    uiTaskRunner tr( this );
    Seis::PreLoader::load( iop, &tr );
    fullUpd( 0 );
}


void uiSeisPreLoadMgr::savePush( CallBacker* )
{
    if ( ids_.isEmpty() ) return;

    CtxtIOObj ctio( PreLoadsTranslatorGroup::ioContext() );
    ctio.ctxt.forread = false;
    uiIOObjSelDlg dlg( this, ctio, "Save pre-load settings" );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    const BufferString fnm( dlg.ioObj()->fullUserExpr(true) );
    delete ctio.ioobj;
    StreamData sd( StreamProvider(fnm).makeOStream() );
    if ( !sd.usable() )
	mErrRet( BufferString("Cannot open output file:\n",fnm) )

    IOPar alliop;
    for ( int iobj=0; iobj<ids_.size(); iobj++ )
    {
	const MultiID id( ids_.get(iobj).buf() );
	IOPar iop; const BufferString parkey( "Seis.", iobj );
	Seis::PreLoader spl( id ); spl.fillPar( iop );
	alliop.mergeComp( iop, parkey );
    }

    ascostream astrm( *sd.ostrm );
    if ( !astrm.putHeader("Pre-loads") )
	mErrRet( BufferString("Cannot write to output file:\n",fnm) )
    alliop.putTo( astrm );
}
