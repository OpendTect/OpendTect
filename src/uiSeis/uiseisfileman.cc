/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiseisfileman.cc,v 1.19 2003-01-16 11:26:25 bert Exp $
________________________________________________________________________

-*/


#include "uiseisfileman.h"
#include "iodirentry.h"
#include "ioobj.h"
#include "ioman.h"
#include "iodir.h"
#include "iostrm.h"
#include "cbvsio.h"
#include "ctxtioobj.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uimergeseis.h"
#include "uiseiscbvsimp.h"
#include "uifiledlg.h"
#include "uitextedit.h"
#include "seistrctr.h"
#include "filegen.h"
#include "binidselimpl.h"
#include "ptrman.h"
#include "transl.h"


uiSeisFileMan::uiSeisFileMan( uiParent* p )
        : uiDialog(p,uiDialog::Setup("Seismic file management",
                                     "Manage seismic cubes",
                                     "103.1.0"))
	, ctio(*new CtxtIOObj(SeisTrcTranslator::ioContext()))
	, ioobj(0)
{
    IOM().to( ctio.ctxt.stdSelKey() );
    ctio.ctxt.trglobexpr = "CBVS";
    entrylist = new IODirEntryList( IOM().dirPtr(), ctio.ctxt );

    uiGroup* topgrp = new uiGroup( this, "Top things" );
    listfld = new uiListBox( topgrp, entrylist->Ptr() );
    listfld->setHSzPol( uiObject::medvar );
    listfld->selectionChanged.notify( mCB(this,uiSeisFileMan,selChg) );

    uiGroup* butgrp = new uiGroup( topgrp, "Action buttons" );
    butgrp->attach( rightOf, listfld );
    rembut = new uiPushButton( butgrp, "Remove ..." );
    rembut->activated.notify( mCB(this,uiSeisFileMan,removePush) );
    renamebut = new uiPushButton( butgrp, "Rename ..." );
    renamebut->activated.notify( mCB(this,uiSeisFileMan,renamePush) );
    renamebut->attach( alignedBelow, rembut );
    relocbut = new uiPushButton( butgrp, "Move ..." );
    relocbut->activated.notify( mCB(this,uiSeisFileMan,relocatePush) );
    relocbut->attach( alignedBelow, renamebut );
    copybut = new uiPushButton( butgrp, "Copy ..." );
    copybut->activated.notify( mCB(this,uiSeisFileMan,copyPush) );
    copybut->attach( alignedBelow, relocbut );
    mergebut = new uiPushButton( butgrp, "Merge ..." );
    mergebut->activated.notify( mCB(this,uiSeisFileMan,mergePush) );
    mergebut->attach( alignedBelow, copybut );

    renamebut->attach( widthSameAs, rembut );
    relocbut->attach( widthSameAs, rembut );
    copybut->attach( widthSameAs, rembut );
    mergebut->attach( widthSameAs, rembut );
    listfld->attach( heightSameAs, butgrp );
  
    infofld = new uiTextEdit( this, "File Info", true );
    infofld->attach( alignedBelow, topgrp );
    infofld->setPrefHeightInChar( 8 );
    infofld->setPrefWidthInChar( 50 );

    selChg( this ); 
    setCancelText( "" );
}


void uiSeisFileMan::selChg( CallBacker* )
{
    entrylist->setCurrent( listfld->currentItem() );
    ioobj = entrylist->selected();
    mkFileInfo();
}


void uiSeisFileMan::mkFileInfo()
{
    if ( !ioobj )
    {
	infofld->setText( "" );
	return;
    }

#define mRangeTxt(line) \
    txt += bs.start.line; txt += " - "; txt += bs.stop.line; \
    txt += " - "; txt += bs.step.line; \

    BufferString txt;
    BinIDSampler bs;
    StepInterval<float> zrg;
    if ( SeisTrcTranslator::getRanges( *ioobj, bs, zrg ) )
    {
	txt = "Inline range: "; mRangeTxt(inl);
	txt += "\nCrossline range: "; mRangeTxt(crl);
	txt += "\nZ-range: "; txt += zrg.start; txt += " - "; 
			        txt += zrg.stop; txt += " - ";
				txt += zrg.step;
    }

    if ( ioobj->pars().size() )
    {
	if ( ioobj->pars().hasKey("Type") )
	{ txt += "\nType: "; txt += ioobj->pars().find( "Type" ); }

	const char* optstr = "Optimized direction";
	if ( ioobj->pars().hasKey(optstr) )
	{ txt += "\nOptimized direction: "; txt += ioobj->pars().find(optstr); }
    }

    mDynamicCastGet(IOStream*,iostrm,ioobj)
    if ( iostrm )
    {
	BufferString fname( iostrm->fileName() );
	if ( !File_isAbsPath(fname) )
	{
	    fname = GetDataDir();
	    fname = File_getFullPath( fname, "Seismics" );
	    fname = File_getFullPath( fname, iostrm->fileName() );
	}
	txt += "\nLocation: "; txt += File_getPathOnly( fname );
	txt += "\nFile name: "; txt += File_getFileName( fname );
	txt += "\nFile size: "; txt += getFileSize( fname );
    }

    
    infofld->setText( txt );
}


void uiSeisFileMan::refreshList( int curitm )
{
    entrylist->fill( IOM().dirPtr() );
    if ( curitm >= entrylist->size() ) curitm--;
    entrylist->setCurrent( curitm );
    ioobj = entrylist->selected();
    listfld->empty();
    listfld->addItems( entrylist->Ptr() );
}


void uiSeisFileMan::removePush( CallBacker* )
{
    if ( !ioobj ) return;
    int curitm = listfld->currentItem();
    if ( ioobj->implRemovable() )
    {
	BufferString msg( "Remove '" );
	if ( !ioobj->isLink() )
	    { msg += ioobj->fullUserExpr(YES); msg += "'?"; }
	else
	{
	    FileNameString fullexpr( ioobj->fullUserExpr(YES) );
	    msg += File_getFileName(fullexpr);
	    msg += "'\n- and everything in it! - ?";
	}
	if ( !uiMSG().askGoOn(msg) ) return;

	if ( !fullImplRemove(*ioobj) )
	{
	    msg = "Could not remove '";
	    msg += ioobj->fullUserExpr(YES); msg += "'";
	    uiMSG().warning( msg );
	}
    }

    entrylist->curRemoved();
    IOM().removeAux( ioobj->key() );
    IOM().dirPtr()->permRemove( ioobj->key() );
    refreshList( curitm );
}


void uiSeisFileMan::renamePush( CallBacker* )
{
    if ( !ioobj ) return;
    BufferString fulloldname = ioobj->fullUserExpr(true);
    int curitm = listfld->currentItem();
    BufferString entrynm = listfld->getText();
    char* ptr = entrynm.buf();
    skipLeadingBlanks( ptr );
    uiGenInputDlg dlg( this, "Rename seismic cube", "New name",
	    		new StringInpSpec(ptr) );
    if ( dlg.go() )
    {
	if ( listfld->isPresent( dlg.text() ) )
	    if ( !uiMSG().askGoOn("Filename exists, overwrite?") )
		return;

	MultiID key = ioobj->key();
	if ( IOM().setFileName( key, dlg.text() ) )
	{
	    PtrMan<IOObj> locioobj = IOM().get( key );
	    handleMultiFiles( fulloldname, locioobj->fullUserExpr(true) );
	}
    }
    refreshList( curitm );
}


void uiSeisFileMan::relocatePush( CallBacker* )
{
    if ( !ioobj ) return;
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    if ( !iostrm ) { pErrMsg("IOObj not IOStream"); return; }

    const FileNameString fulloldname = ioobj->fullUserExpr(true);
    if ( !File_exists(fulloldname) )
    {
	uiMSG().error( "File does not exist" );
        return;
    }

    const char* dirpath = File_getPathOnly( fulloldname );
    uiFileDialog dlg( this, uiFileDialog::DirectoryOnly, dirpath, 0,
	   	      "Select destination directory" );
    if ( !dlg.go() ) return;
    BufferString newdirpath = dlg.fileName();
    BufferString fname = File_getFileName( fulloldname );
    BufferString fullnewname = File_getFullPath(newdirpath,fname);
    if ( File_exists(fullnewname) )
    {
	uiMSG().error( "New name exists at given location\n"
		       "Please select another location" );
	return;
    }

    UsrMsg( "Moving cube ..." );
    if ( !File_rename(fulloldname,fullnewname) )
    {
	uiMSG().error( "Could not move cube to new location" );
	UsrMsg( "" );
	return;
    }

    iostrm->setFileName( fullnewname );
    handleMultiFiles( fulloldname, fullnewname );

    int curitm = listfld->currentItem();
    if ( IOM().dirPtr()->commitChanges( ioobj ) )
	refreshList( curitm );
}


void uiSeisFileMan::copyPush( CallBacker* )
{
    if ( !ioobj ) return;
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    if ( !iostrm ) { pErrMsg("IOObj not IOStream"); return; }

    const int curitm = listfld->currentItem();
    uiSeisImpCBVS dlg( this, iostrm );
    dlg.go();
    refreshList( curitm );
}


void uiSeisFileMan::handleMultiFiles( const char* fulloldname,
					const char* fullnewname )
{
    for ( int inr=1; ; inr++ )
    {
	BufferString fnm( CBVSIOMgr::getFileName(fulloldname,inr) );
	if ( File_isEmpty(fnm) ) break;

	BufferString newfn( CBVSIOMgr::getFileName(fullnewname,inr) );
	BufferString msg = "Moving part "; msg += inr+1; msg += " ...";
	UsrMsg( msg );
	if ( !File_rename(fnm,newfn) )
	{
	    msg = "Move aborted: could not move part "; msg += inr+1;
	    msg += "\nThe cube data is now partly moved.";
	    msg += "\nPlease contact system administration to fix the problem";
	    uiMSG().error( msg );
	    UsrMsg( "" );
	    break;
	}
    }
    UsrMsg( "" );
}


BufferString uiSeisFileMan::getFileSize( const char* filenm )
{
    BufferString szstr;
    double totalsz = 0;
    for ( int inr=0; ; inr++ )
    {
	BufferString fullnm( CBVSIOMgr::getFileName(filenm,inr) );
	if ( !File_exists(fullnm) ) break;
	
	totalsz += (double)File_getKbSize( fullnm );
    }

    if ( totalsz > 1024 )
    {
        bool doGb = totalsz > 1048576;
	int nr = doGb ? (int)(totalsz/10485.76+.5) : (int)(totalsz/10.24+.5);
	szstr += nr/100; 
	int rest = nr%100; 
	szstr += rest < 10 ? ".0" : "."; szstr += rest;
	szstr += doGb ? " (Gb)" : " (Mb)";
    }
    else if ( !totalsz )
    {
	szstr += File_isEmpty(filenm) ? "-" : "< 1 (kB)";
    }
    else
    { szstr += totalsz; szstr += " (kB)"; }

    return szstr;
}


void uiSeisFileMan::mergePush( CallBacker* )
{
    int curitm = listfld->currentItem();
    uiMergeSeis dlg( this );
    dlg.go();
    refreshList( curitm );
}
