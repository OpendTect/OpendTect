/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          August 2003
 RCS:           $Id: uisurfaceman.cc,v 1.4 2003-10-15 15:15:55 bert Exp $
________________________________________________________________________

-*/


#include "uisurfaceman.h"
#include "iodirentry.h"
#include "ioobj.h"
#include "ioman.h"
#include "iostrm.h"
#include "ctxtioobj.h"
#include "uilistbox.h"
#include "uiioobjmanip.h"
#include "uitextedit.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "pixmap.h"
#include "filegen.h"
#include "binidselimpl.h"
#include "emmanager.h"
#include "emhorizontransl.h"
#include "emsurfaceiodata.h"
#include "emsurfauxdataio.h"
#include "streamconn.h"
#include "uimsg.h"


uiSurfaceMan::uiSurfaceMan( uiParent* p )
        : uiDialog(p,uiDialog::Setup("Surface file management",
                                     "Manage surfaces",
                                     "103.1.0").nrstatusflds(1))
	, ctio(*mMkCtxtIOObj(EMHorizon))
	, ioobj(0)
{
    IOM().to( ctio.ctxt.stdSelKey() );
    entrylist = new IODirEntryList( IOM().dirPtr(), ctio.ctxt );

    uiGroup* topgrp = new uiGroup( this, "Top things" );
    listfld = new uiListBox( topgrp, entrylist->Ptr() );
    listfld->setHSzPol( uiObject::medvar );
    listfld->selectionChanged.notify( mCB(this,uiSurfaceMan,selChg) );

    manipgrp = new uiIOObjManipGroup( listfld, *entrylist, "hor" );
    manipgrp->preRelocation.notify( mCB(this,uiSurfaceMan,relocMsg) );
    manipgrp->postRelocation.notify( mCB(this,uiSurfaceMan,postReloc) );

    attribfld = new uiListBox( topgrp, "Calculated attributes" );
    attribfld->attach( rightTo, manipgrp );
    attribfld->setToolTip( "Calculated attributes" );

    butgrp = new uiButtonGroup( topgrp, "" );
    butgrp->attach( rightTo, attribfld );
    const ioPixmap rempm( GetDataFileName("trashcan.png") );
    rembut = new uiToolButton( butgrp, "Remove", rempm );
    rembut->activated.notify( mCB(this,uiSurfaceMan,remPush) );
    rembut->setToolTip( "Remove this attribute" );

    infofld = new uiTextEdit( this, "File Info", true );
    infofld->attach( alignedBelow, topgrp );
    infofld->setPrefHeightInChar( 8 );
    infofld->setPrefWidthInChar( 50 );
    topgrp->setPrefWidthInChar( 75 );

    selChg( this ); 
    setCancelText( "" );
}


void uiSurfaceMan::selChg( CallBacker* cb )
{
    entrylist->setCurrent( listfld->currentItem() );
    ioobj = entrylist->selected();

    mkFileInfo();
    manipgrp->selChg( cb );

    BufferString msg;
    GetFreeMBOnDiskMsg( GetFreeMBOnDisk(ioobj), msg );
    toStatusBar( msg );
}


void uiSurfaceMan::remPush( CallBacker* )
{
    if ( !attribfld->size() || !attribfld->nrSelected() ) return;
    
    mDynamicCastGet(StreamConn*,conn,ioobj->getConn(Conn::Read))
    if ( !conn ) return;

    const char* attrnm = attribfld->getText();
    int gap = 0;
    for ( int idx=0; ; idx++ )
    {
	if ( gap > 100 ) return;

	BufferString fnm(
		EM::dgbSurfDataWriter::createHovName(conn->fileName(),idx) );
	if ( File_isEmpty(fnm) )
	{ gap++; continue; }

	EM::dgbSurfDataReader rdr( fnm );
	BufferString datanm = rdr.dataName();
	if ( datanm == attrnm )
	{
	    BufferString msg( "Remove attribute: '" );
	    msg += datanm; msg += "'?";
	    if ( uiMSG().askGoOn( msg ) )
		File_remove( (const char*)fnm, NO );

	    selChg(this);
	    return;
	}
    }
}


void uiSurfaceMan::fillAttribList( const ObjectSet<BufferString>& strs )
{
    attribfld->empty();
    for ( int idx=0; idx<strs.size(); idx++)
	attribfld->addItem( strs[idx]->buf() );
    attribfld->selAll( false );
}


void uiSurfaceMan::relocMsg( CallBacker* cb )
{
    toStatusBar( manipgrp->curRelocationMsg() );
}


void uiSurfaceMan::postReloc( CallBacker* cb )
{
    selChg( cb );
}


void uiSurfaceMan::mkFileInfo()
{
    if ( !ioobj )
    {
	infofld->setText( "" );
	return;
    }

#define mRangeTxt(line) \
    txt += sd.rg.start.line; txt += " - "; txt += sd.rg.stop.line; \
    txt += " - "; txt += sd.rg.step.line; \

    BufferString txt;
    BinIDSampler bs;
    EM::SurfaceIOData sd;
    EM::EMM().getSurfaceData( ioobj->key(), sd );
    fillAttribList( sd.valnames );
    txt = "Inline range: "; mRangeTxt(inl);
    txt += "\nCrossline range: "; mRangeTxt(crl);

    mDynamicCastGet(StreamConn*,conn,ioobj->getConn(Conn::Read))
    if ( !conn ) return;

    BufferString fname( conn->fileName() );
    txt += "\nLocation: "; txt += File_getPathOnly( fname );
    txt += "\nFile name: "; txt += File_getFileName( fname );
    txt += "\nFile size: "; txt += getFileSize( fname );

    if ( sd.patches.size() > 1 )
    {
	txt += "\nNr of patches: "; txt += sd.patches.size();
	for ( int idx=0; idx<sd.patches.size(); idx++ )
	{
	    txt += "\n\tPatch "; txt += idx+1; txt += ": "; 
	    txt += sd.patches[idx]->buf();
	}
    }

    infofld->setText( txt );
}


BufferString uiSurfaceMan::getFileSize( const char* filenm )
{
    BufferString szstr;
    double totalsz = (double)File_getKbSize( filenm );

    if ( totalsz > 1024 )
    {
        bool doGb = totalsz > 1048576;
	int nr = doGb ? mNINT(totalsz/10485.76) : mNINT(totalsz/10.24);
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
