/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiseisfileman.cc,v 1.42 2004-10-04 15:04:04 bert Exp $
________________________________________________________________________

-*/


#include "uiseisfileman.h"
#include "iodirentry.h"
#include "ioobj.h"
#include "iodir.h"
#include "ioman.h"
#include "iostrm.h"
#include "iopar.h"
#include "cbvsio.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "uiseisioobjinfo.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uimergeseis.h"
#include "uiseiscbvsimp.h"
#include "uiioobjmanip.h"
#include "uitextedit.h"
#include "pixmap.h"
#include "seistrctr.h"
#include "filegen.h"
#include "filepath.h"
#include "cubesampling.h"
#include "survinfo.h"


uiSeisFileMan::uiSeisFileMan( uiParent* p )
        : uiDialog(p,uiDialog::Setup("Seismic file management",
                                     "Manage seismic cubes",
                                     "103.1.0").nrstatusflds(1))
	, ctio(*mMkCtxtIOObj(SeisTrc))
{
    IOM().to( ctio.ctxt.stdSelKey() );
    ctio.ctxt.trglobexpr = "CBVS`2D";
    entrylist = new IODirEntryList( IOM().dirPtr(), ctio.ctxt );

    uiGroup* topgrp = new uiGroup( this, "Top things" );
    listfld = new uiListBox( topgrp, "Seismic cubes/Line sets" );
    listfld->setHSzPol( uiObject::medvar );
    for ( int idx=0; idx<entrylist->size(); idx++ )
	listfld->addItem( (*entrylist)[idx]->name() );
    listfld->setCurrentItem(0);
    listfld->selectionChanged.notify( mCB(this,uiSeisFileMan,selChg) );

    manipgrp = new uiIOObjManipGroup( listfld, *entrylist, "cbvs" );
    manipgrp->preRelocation.notify( mCB(this,uiSeisFileMan,relocMsg) );
    manipgrp->postRelocation.notify( mCB(this,uiSeisFileMan,postReloc) );

    const ioPixmap copypm( GetDataFileName("copyobj.png") );
    copybut = new uiToolButton( manipgrp, "copy toolbut", copypm );
    copybut->activated.notify( mCB(this,uiSeisFileMan,copyPush) );
    copybut->setToolTip( "Copy cube" );
    const ioPixmap mergepm( GetDataFileName("mergeseis.png") );
    mergebut = new uiToolButton( manipgrp, "merge toolbut", mergepm );
    mergebut->activated.notify( mCB(this,uiSeisFileMan,mergePush) );
    mergebut->setToolTip( "Merge blocks of inlines into cube" );

    infofld = new uiTextEdit( this, "File Info", true );
    infofld->attach( centeredBelow, topgrp );
    infofld->setPrefHeightInChar( 9 );
    infofld->setPrefWidthInChar( 50 );
    topgrp->setPrefWidthInChar( 50 );

    selChg( this ); 
    setCancelText( "" );
    setOkText( "Dismiss" );
}


uiSeisFileMan::~uiSeisFileMan()
{
    delete ctio.ioobj; delete &ctio;
}


void uiSeisFileMan::selChg( CallBacker* cb )
{
    entrylist->setCurrent( listfld->currentItem() );
    const IOObj* selioobj = entrylist->selected();
    const bool is2d = selioobj && SeisTrcTranslator::is2D( *selioobj );
    ctio.setObj( selioobj ? selioobj->clone() : 0 );
    copybut->setSensitive( !is2d && selioobj && ctio.ioobj->implExists(true) );
    mergebut->setSensitive( is2d );
    mkFileInfo();
    manipgrp->selChg( cb );

    BufferString msg;
    GetFreeMBOnDiskMsg( GetFreeMBOnDisk(ctio.ioobj), msg );
    toStatusBar( msg );
}


void uiSeisFileMan::relocMsg( CallBacker* cb )
{
    toStatusBar( manipgrp->curRelocationMsg() );
}


void uiSeisFileMan::postReloc( CallBacker* cb )
{
    int curidx = 
	entrylist->ObjectSet<IODirEntry>::indexOf( entrylist->current() );
    listfld->setCurrentItem( curidx );
}


void uiSeisFileMan::mkFileInfo()
{
    if ( !ctio.ioobj )
    {
	infofld->setText( "" );
	return;
    }

#define mRangeTxt(line) \
    txt += cs.hrg.start.line; txt += " - "; txt += cs.hrg.stop.line; \
    txt += " ["; txt += cs.hrg.step.line; txt += "]"

#define mZRangeTxt(memb) \
    txt += SI().zIsTime() ? mNINT(1000*cs.zrg.memb) : cs.zrg.memb

    BufferString txt; CubeSampling cs;
    uiSeisIOObjInfo oinf( *ctio.ioobj, false );
    if ( oinf.getRanges(cs) )
    {
	txt = "";
	if ( !mIsUndefInt(cs.hrg.stop.inl) )
	    { txt += "Inline range: "; mRangeTxt(inl); }
	if ( !mIsUndefInt(cs.hrg.stop.crl) )
	    { txt += "\nCrossline range: "; mRangeTxt(crl); }
	txt += "\nZ-range: "; mZRangeTxt(start); txt += " - ";
	mZRangeTxt(stop); txt += " ["; mZRangeTxt(step); txt += "]";
    }

    if ( ctio.ioobj->pars().size() )
    {
	if ( ctio.ioobj->pars().hasKey("Type") )
	{ txt += "\nType: "; txt += ctio.ioobj->pars().find( "Type" ); }

	const char* optstr = "Optimized direction";
	if ( ctio.ioobj->pars().hasKey(optstr) )
	{ txt += "\nOptimized direction: ";
	    txt += ctio.ioobj->pars().find(optstr); }
    }

    mDynamicCastGet(const IOStream*,iostrm,ctio.ioobj)
    if ( iostrm )
    {
	BufferString fname( iostrm->fileName() );
	FilePath fp( fname );
	if ( !fp.isAbsolute() )
	    fp.set( GetDataDir() ).add( "Seismics" ).add( fname );
	fname = fp.fullPath();
	txt += "\nLocation: "; txt += fp.pathOnly();
	txt += "\nFile name: "; txt += fp.fileName();
	txt += "\nFile size: "; txt += getFileSize( fname );
    }

    
    infofld->setText( txt );
}


void uiSeisFileMan::copyPush( CallBacker* )
{
    if ( !ctio.ioobj ) return;
    mDynamicCastGet(const IOStream*,iostrm,ctio.ioobj)
    if ( !iostrm ) { pErrMsg("IOObj not IOStream"); return; }

    MultiID key( iostrm->key() );
    uiSeisImpCBVS dlg( this, iostrm );
    dlg.go();
    manipgrp->refreshList( key );
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
    if ( !ctio.ioobj ) return;

    MultiID key( ctio.ioobj->key() );
    uiMergeSeis dlg( this );
    dlg.go();
    manipgrp->refreshList( key );
}
