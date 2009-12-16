/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiobjfileman.cc,v 1.28 2009-12-16 11:33:04 cvsbert Exp $";


#include "uiobjfileman.h"
#include "uiioobjsel.h"
#include "uiioobjmanip.h"
#include "ioobj.h"
#include "ioman.h"
#include "dirlist.h"
#include "ctxtioobj.h"
#include "filegen.h"
#include "filepath.h"
#include "streamconn.h"
#include "survinfo.h"
#include "keystrs.h"
#include "transl.h"

#include "uilistbox.h"
#include "uitextedit.h"
#include "uibutton.h"


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
    IOM().to( 0 ); IOM().to( ctxt_.getSelKey() );
    selgrp = new uiIOObjSelGrp( this, CtxtIOObj(ctxt_), 0, false );
    selgrp->selectionChg.notify( mCB(this,uiObjFileMan,selChg) );
    selgrp->getListField()->setHSzPol( uiObject::Medium );

    mkdefbut = selgrp->getManipGroup()->addButton( "makedefault.png",
	    mCB(this,uiObjFileMan,makeDefault), "Set as default" );

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
    curimplexists_ = curioobj_ && curioobj_->implExists(true);
    mkdefbut->setSensitive( curimplexists_ );

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


void uiObjFileMan::makeDefault( CallBacker* )
{
    if ( !curioobj_ ) return;
    SI().getPars().set( getDefKey(), curioobj_->key() );
    SI().savePars();
    selgrp->fullUpdate( curioobj_->key() );
}


const char* uiObjFileMan::getDefKey() const
{
    static BufferString ret;
    ctxt_.fillTrGroup();
    ret = IOPar::compKey(sKey::Default,ctxt_.trgroup->userName());
    return ret.buf();
}


double uiObjFileMan::getFileSize( const char* filenm, int& nrfiles ) const
{
    nrfiles = File_exists(filenm) ? 1 : 0;
    if ( nrfiles == 0 )
	return 0;

    double ret = (double)File_getKbSize( filenm );
    if ( ret < 0 ) ret = -ret;
    if ( !File_isDirectory(filenm) )
	return ret;

    DirList dl( filenm );
    nrfiles = 0;
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	int extranrfiles = 0;
	const BufferString subfnm( dl.fullPath(idx) );
	ret += getFileSize( subfnm, extranrfiles );
	nrfiles += extranrfiles;
    }

    return ret;
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
	szstr += doGb ? " GB" : " MB";
    }
    else if ( filesz == 0 )
	szstr = "< 1 kB";
    else
    {
	szstr = filesz;
	szstr += " kB";
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
	{ txt = "-"; infofld->setText( txt ); return txt; }

    BufferString fname( conn->fileName() );
    FilePath fp( fname );
    const bool isdir = File_isDirectory( fname );
    int nrfiles = 1;
    const double totsz = getFileSize( fname, nrfiles );

    txt += "Location: "; txt += fp.pathOnly();
    txt += isdir ? "\nDirectory name: " : "\nFile name: ";
    txt += fp.fileName();
    txt += "\nSize on disk: "; txt += getFileSizeString( totsz );
    if ( nrfiles > 1 )
	{ txt += "\nNumber of files: "; txt += nrfiles; }
    const char* timestr = File_getTime( fname );
    if ( timestr ) { txt += "\nLast modified: "; txt += timestr; }
    int txtsz = txt.size()-1;
    if ( txt[ txtsz ] != '\n' ) txt += "\n";
    txt += "Object ID: "; txt += curioobj_->key(); txt += "\n";
    conn->close();
    delete conn;
    return txt;
}
