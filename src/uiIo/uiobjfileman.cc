/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiobjfileman.cc,v 1.3 2004-12-24 10:35:57 bert Exp $
________________________________________________________________________

-*/


#include "uiobjfileman.h"
#include "iodirentry.h"
#include "ioobj.h"
#include "iodir.h"
#include "ioman.h"
#include "ctxtioobj.h"
#include "uilistbox.h"
#include "uiioobjmanip.h"
#include "uitextedit.h"
#include "filegen.h"
#include "filepath.h"
#include "streamconn.h"


static const int cPrefHeight = 10;
static const int cPrefWidth = 75;


uiObjFileMan::uiObjFileMan( uiParent* p, const uiDialog::Setup& s,
			    CtxtIOObj& ctio_ )
    : uiDialog(p,s)
    , ctio(ctio_)
{
    setCancelText( "" );
    setOkText( "Dismiss" );
}


uiObjFileMan::~uiObjFileMan()
{
    IOM().newIODir.remove( mCB(this,uiObjFileMan,postIomChg) );
    delete ctio.ioobj; delete &ctio;
}


void uiObjFileMan::createDefaultUI( const char* ext )
{
    IOM().to( ctio.ctxt.stdSelKey() );
    entrylist = new IODirEntryList( IOM().dirPtr(), ctio.ctxt );
    IOM().newIODir.notify( mCB(this,uiObjFileMan,postIomChg) );

    topgrp = new uiGroup( this, "Top things" );
    listfld = new uiListBox( topgrp, "Objects" );
    listfld->setHSzPol( uiObject::medvar );
    for ( int idx=0; idx<entrylist->size(); idx++ )
	listfld->addItem( (*entrylist)[idx]->name() );
    listfld->setCurrentItem(0);
    listfld->selectionChanged.notify( mCB(this,uiObjFileMan,selChg) );
    listfld->rightButtonClicked.notify( mCB(this,uiObjFileMan,rightClicked) );

    manipgrp = new uiIOObjManipGroup( listfld, *entrylist, ext );
    manipgrp->preRelocation.notify( mCB(this,uiObjFileMan,relocMsg) );
    manipgrp->postRelocation.notify( mCB(this,uiObjFileMan,postReloc) );

    infofld = new uiTextEdit( this, "File Info", true );
    infofld->attach( centeredBelow, topgrp );
    infofld->setPrefHeightInChar( cPrefHeight );
    infofld->setPrefWidthInChar( cPrefWidth );
    topgrp->setPrefWidthInChar( cPrefWidth );
}


void uiObjFileMan::postIomChg( CallBacker* cb )
{
    MultiID selkey;
    if ( ctio.ioobj ) selkey = ctio.ioobj->key();
    entrylist->fill( IOM().dirPtr() );
    listfld->empty();
    int selidx = -1;
    for ( int idx=0; idx<entrylist->size(); idx++ )
    {
	const IODirEntry& de = *(*entrylist)[idx];
	if ( de.ioobj && selkey == de.ioobj->key() )
	    selidx = idx;
	listfld->addItem( (*entrylist)[idx]->name() );
    }
    if ( selidx >= 0 )
	listfld->setCurrentItem( selidx );
    else if ( entrylist->size() > 0 )
	listfld->setCurrentItem( 0 );

    selChg(cb);
}


void uiObjFileMan::selChg( CallBacker* cb )
{
    entrylist->setCurrent( listfld->currentItem() );
    const IOObj* selioobj = entrylist->selected();
    ctio.setObj( selioobj ? selioobj->clone() : 0 );

    ownSelChg();

    mkFileInfo();
    manipgrp->selChg( cb );

    BufferString msg;
    GetFreeMBOnDiskMsg( GetFreeMBOnDisk(ctio.ioobj), msg );
    toStatusBar( msg );
}


void uiObjFileMan::relocMsg( CallBacker* )
{
    toStatusBar( manipgrp->curRelocationMsg() );
}


void uiObjFileMan::postReloc( CallBacker* )
{
    int curidx = 
	entrylist->ObjectSet<IODirEntry>::indexOf( entrylist->current() );
    listfld->setCurrentItem( curidx );
}


BufferString uiObjFileMan::getFileSizeString( double filesz )
{
    BufferString szstr;
    if ( filesz > 1024 )
    {
        const bool doGb = filesz > 1048576;
	const int nr = doGb ? mNINT(filesz/10485.76) : mNINT(filesz/10.24);
	szstr = nr/100; 
	const int rest = nr%100; 
	szstr += rest < 10 ? ".0" : "."; szstr += rest;
	szstr += doGb ? " (Gb)" : " (Mb)";
    }
    else if ( filesz < 0 )
	szstr = "-";
    else if ( !filesz )
	szstr = "< 1 (kB)";
    else
    {
	szstr = filesz;
	szstr += " (kB)";
    }

    return szstr;
}


BufferString uiObjFileMan::getFileInfo()
{
    mDynamicCastGet(StreamConn*,conn,ctio.ioobj->getConn(Conn::Read))
    if ( !conn ) { infofld->setText( "" ); return ""; }

    BufferString txt;
    BufferString fname( conn->fileName() );
    FilePath fp( fname );
    txt += "\nLocation: "; txt += fp.pathOnly();
    txt += "\nFile name: "; txt += fp.fileName();
    txt += "\nFile size: "; txt += getFileSizeString( getFileSize(fname) );
    conn->close();
    delete conn;
    return txt;
}


double uiObjFileMan::getFileSize( const char* filenm )
{
    return (double)File_getKbSize( filenm );
}
