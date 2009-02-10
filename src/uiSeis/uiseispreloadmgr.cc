/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Feb 2009
-*/

static const char* rcsID = "$Id: uiseispreloadmgr.cc,v 1.3 2009-02-10 16:57:35 cvsbert Exp $";

#include "uiseispreloadmgr.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "strmprov.h"
#include "cbvsio.h"
#include "ioobj.h"
#include "ioman.h"
#include "ptrman.h"
#include "filepath.h"
#include "filegen.h"
#include "datapack.h"
#include "uimsg.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uitextedit.h"
#include "uiioobjsel.h"
#include "uitaskrunner.h"


uiSeisPreLoadMgr::uiSeisPreLoadMgr( uiParent* p )
    : uiDialog(p,Setup("Pre-load manager","Pre-loaded seismic data",
			mTODOHelpID))
{
    setCtrlStyle( LeaveOnly );
    uiGroup* topgrp = new uiGroup( this, "Top group" );
    listfld_ = new uiListBox( topgrp, "Loaded entries" );
    listfld_->selectionChanged.notify( mCB(this,uiSeisPreLoadMgr,selChg) );
    topgrp->setHAlignObj( listfld_ );

    uiButtonGroup* bgrp = new uiButtonGroup( topgrp, "Manip buttons" );
    bgrp->attach( rightOf, listfld_ );
#   define mAddBut(s,fn) \
    new uiPushButton( bgrp, s, mCB(this,uiSeisPreLoadMgr,fn), false )
    mAddBut("Add Cube",cubeLoadPush);
    mAddBut("Add Lines",linesLoadPush);
    mAddBut("Add 3D Pre-Stack lines",ps3DPush);
    mAddBut("Add 2D Pre-Stack lines",ps2DPush);
    mAddBut("Unload Selected",unloadPush);

    infofld_ = new uiTextEdit( this, "Info" );
    infofld_->attach( alignedBelow, topgrp );
    infofld_->attach( widthSameAs, topgrp );
    infofld_->setPrefHeightInChar( 5 );

    finaliseDone.notify( mCB(this,uiSeisPreLoadMgr,fullUpd) );
}


void uiSeisPreLoadMgr::fullUpd( CallBacker* )
{
    fillList();
}


void uiSeisPreLoadMgr::fillList()
{
    listfld_->empty();
    StreamProvider::getPreLoadedIDs( ids_ );
    if ( ids_.isEmpty() ) return;

    for ( int idx=0; idx<ids_.size(); idx++ )
    {
	const MultiID ky( ids_.get(idx) );
	PtrMan<IOObj> ioobj = IOM().get( ky );
	if ( !ioobj )
	{
	    StreamProvider::unLoad( ky.buf(), true );
	    ids_.remove( idx ); idx--;
	    continue;
	}
	listfld_->addItem( ioobj->name() );
    }

    listfld_->setSelected( 0 );
}


void uiSeisPreLoadMgr::selChg( CallBacker* )
{
    const int selidx = listfld_->currentItem();
    if ( ids_.isEmpty() || selidx < 0 )
	{ infofld_->setText(""); return; }

    const MultiID ky( ids_.get(selidx) );
    PtrMan<IOObj> ioobj = IOM().get( ky );
    if ( !ioobj )
	{ infofld_->setText("Internal error: cannot find IOObj"); return; }

    SeisIOObjInfo ioinf( *ioobj );
    if ( !ioinf.isOK() )
	{ infofld_->setText("Internal error: IOObj not OK"); return; }
    const Seis::GeomType gt = ioinf.geomType();
    BufferStringSet fnms;
    StreamProvider::getPreLoadedFileNames( ky.buf(), fnms );
    const int nrfiles = fnms.size();
    if ( nrfiles < 1 )
	{ infofld_->setText("No files"); return; }

    BufferString disptxt( "Data type: " ); disptxt += Seis::nameOf( gt );

    switch ( gt )
    {
	//TODO
	case Seis::Vol:
	break;
	case Seis::Line:
	break;
	case Seis::VolPS:
	break;
	case Seis::LinePS:
	break;
    }

    FilePath fp( fnms.get(0) );
    disptxt += "\nDirectory: "; disptxt += fp.pathOnly();
    disptxt += "\nFile name"; if ( nrfiles > 1 ) disptxt += "s";
    disptxt += ": "; disptxt += fp.fileName();
    float totmem = 0;
    for ( int idx=0; idx<nrfiles; idx++ )
    {
	const char* fnm = fnms.get(idx).buf();
	int dpid = StreamProvider::getPreLoadedDataPackID( fnm );
	totmem += DPM(DataPackMgr::BufID()).nrKBytesOf( dpid );

	if ( idx == 0 || (idx < nrfiles-1 && idx > 3) )
	    continue;

	fp.set( fnms.get(idx) );
	disptxt += " "; disptxt += fp.fileName();
	if ( nrfiles > 5 && idx == 3 )
	    disptxt += " ...";
    }

    totmem /= 1024; const int memmb = mNINT(totmem);
    disptxt += "\nTotal memory in MB: "; disptxt += memmb;
    infofld_->setText( disptxt );
}


void uiSeisPreLoadMgr::cubeLoadPush( CallBacker* )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisTrc);
    ctio->ctxt.trglobexpr = "CBVS";
    uiIOObjSelDlg dlg( this, *ctio );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    const BufferString basefnm = CBVSIOMgr::baseFileName(
				    dlg.ioObj()->fullUserExpr(true) );
    const MultiID keyid( dlg.ioObj()->key() );
    const char* id = keyid.buf();

    if ( StreamProvider::isPreLoaded(id,true) )
    {
	if ( !uiMSG().askGoOn("This cube is already pre-loaded.\n"
		    	      "Do you want to re-load?") )
	    return;
	StreamProvider::unLoad( id, true );
    }

    uiTaskRunner tr( this );
    for ( int idx=0; true; idx++ )
    {
	const BufferString fnm( CBVSIOMgr::getFileName(basefnm,idx) );
	if ( !File_exists(fnm) || !StreamProvider::preLoad(fnm,tr,id) )
	    break;
    }

    fullUpd( 0 );
}


void uiSeisPreLoadMgr::linesLoadPush( CallBacker* )
{
    uiMSG().error( "TODO: implement pre-load lines" );
}


void uiSeisPreLoadMgr::ps3DPush( CallBacker* )
{
    uiMSG().error( "TODO: implement pre-load PS 3D data" );
}


void uiSeisPreLoadMgr::ps2DPush( CallBacker* )
{
    uiMSG().error( "TODO: implement pre-load PS 2D data" );
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

    StreamProvider::unLoad( ids_.get(selidx), true );
    fillList();
    int newselidx = selidx;
    if ( newselidx >= ids_.size() )
	newselidx = ids_.size() - 1;
    if ( newselidx >= 0 )
	listfld_->setCurrentItem( newselidx );
    else
	selChg( 0 );
}
