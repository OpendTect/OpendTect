/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiobjfileman.cc,v 1.13 2006-08-30 16:03:27 cvsbert Exp $
________________________________________________________________________

-*/


#include "uiobjfileman.h"
#include "uiioobjsel.h"
#include "ioobj.h"
#include "ioman.h"
#include "ctxtioobj.h"
#include "uitextedit.h"
#include "filegen.h"
#include "filepath.h"
#include "streamconn.h"


static const int cPrefHeight = 10;
static const int cPrefWidth = 30;


uiObjFileMan::uiObjFileMan( uiParent* p, const uiDialog::Setup& s,
			    const IOObjContext& ctxt )
    : uiDialog(p,s)
    , curioobj_(0)
    , ctxt_(*new IOObjContext(ctxt))
{
    setCtrlStyle( LeaveOnly );
}


uiObjFileMan::~uiObjFileMan()
{
    delete curioobj_;
    delete &ctxt_;
}


void uiObjFileMan::createDefaultUI()
{
    IOM().to( ctxt_.getSelKey() );
    selgrp = new uiIOObjSelGrp( this, CtxtIOObj(ctxt_), 0, false );
    selgrp->selectionChg.notify( mCB(this,uiObjFileMan,selChg) );

    infofld = new uiTextEdit( this, "Object Info", true );
    infofld->attach( ensureBelow, selgrp );
    infofld->setPrefHeightInChar( cPrefHeight );
    infofld->setStretch( 2, 0 );
    selgrp->setPrefWidthInChar( cPrefWidth );
}


void uiObjFileMan::selChg( CallBacker* cb )
{
    delete curioobj_;
    curioobj_ = selgrp->nrSel() > 0 ? IOM().get(selgrp->selected(0)) : 0;

    ownSelChg();
    if ( curioobj_ )
	mkFileInfo();
    else
	infofld->setText( "" );

    BufferString msg;
    if ( curioobj_ )
	GetFreeMBOnDiskMsg( GetFreeMBOnDisk(curioobj_), msg );
    toStatusBar( msg );
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
    BufferString txt;
    if ( !curioobj_ )
	return txt;

    mDynamicCastGet(StreamConn*,conn,curioobj_->getConn(Conn::Read))
    if ( !conn )
    {
	BufferString errtxt( "File not found:\n" );
	errtxt += curioobj_->fullUserExpr( true );
	infofld->setText( errtxt );
	return errtxt;
    }

    BufferString fname( conn->fileName() );
    FilePath fp( fname );
    txt += "Location: "; txt += fp.pathOnly();
    txt += "\nFile name: "; txt += fp.fileName();
    txt += "\nFile size: "; txt += getFileSizeString( getFileSize(fname) );
    txt += "\nLast modified: "; txt += File_getTime( fname );
    if ( txt.lastChar() != '\n' ) txt += '\n';
    txt += "Object ID: "; txt += curioobj_->key();
    txt += "\n";
    conn->close();
    delete conn;
    return txt;
}


double uiObjFileMan::getFileSize( const char* filenm )
{
    return (double)File_getKbSize( filenm );
}
