/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2003
 RCS:           $Id: uiwellman.cc,v 1.19 2004-05-21 16:55:42 bert Exp $
________________________________________________________________________

-*/

#include "uiwellman.h"
#include "uiwelldlgs.h"
#include "iodirentry.h"
#include "ioobj.h"
#include "ioman.h"
#include "iostrm.h"
#include "ctxtioobj.h"
#include "wellman.h"
#include "welldata.h"
#include "welllogset.h"
#include "welllog.h"
#include "wellmarker.h"
#include "wellreader.h"
#include "wellwriter.h"
#include "welltransl.h"
#include "uiioobjmanip.h"
#include "uigroup.h"
#include "uilistbox.h"
#include "uitextedit.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uigeninputdlg.h"
#include "bufstringset.h"
#include "pixmap.h"
#include "filegen.h"
#include "filepath.h"
#include "strmprov.h"
#include "ptrman.h"
#include "uimsg.h"
#include "survinfo.h"


static const int infoheight = 8;
static const int infowidth = 50;
static const int grpwidth = 75;


uiWellMan::uiWellMan( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Well file management",
				 "Manage wells",
				 "107.1.0").nrstatusflds(1))
    , ctio(*mMkCtxtIOObj(Well))
    , welldata(0)
    , wellrdr(0)
    , fname("")
{
    IOM().to( ctio.ctxt.stdSelKey() );
    entrylist = new IODirEntryList( IOM().dirPtr(), ctio.ctxt );

    uiGroup* topgrp = new uiGroup( this, "Top things" );
    listfld = new uiListBox( topgrp, "Well objects" );
    listfld->setHSzPol( uiObject::medvar );
    for ( int idx=0; idx<entrylist->size(); idx++ )
	listfld->addItem( (*entrylist)[idx]->name() );
    listfld->setCurrentItem(0);
    listfld->selectionChanged.notify( mCB(this,uiWellMan,selChg) );

    manipgrp = new uiIOObjManipGroup( listfld, *entrylist, "well" );

    logsfld = new uiListBox( topgrp, "Available logs", true );
    logsfld->attach( rightTo, manipgrp );
    logsfld->setToolTip( "Available logs" );
    topgrp->setHAlignObj( logsfld );

    butgrp = new uiButtonGroup( topgrp, "" );
    butgrp->attach( rightTo, logsfld );
    uiToolButton* renbut = new uiToolButton( butgrp, "Rename",
				ioPixmap(GetDataFileName("renameobj.png")),
				mCB(this,uiWellMan,renameLogPush) );
    renbut->setToolTip( "Rename selected log" );
    
    const ioPixmap rempm( GetDataFileName("trashcan.png") );
    rembut = new uiToolButton( butgrp, "Remove", 
	    			ioPixmap(GetDataFileName("trashcan.png")),
				mCB(this,uiWellMan,removeLogPush) );
    rembut->setToolTip( "Remove selected log" );

    uiToolButton* expbut = new uiToolButton( butgrp, "Export", 
				ioPixmap(GetDataFileName("export.png")),
				mCB(this,uiWellMan,exportLogs) );
    expbut->setToolTip( "Export log" );

    uiPushButton* markerbut = new uiPushButton( this, "Edit markers ..." );
    markerbut->activated.notify( mCB(this,uiWellMan,edMarkers) );
    markerbut->attach( ensureBelow, topgrp );

    uiPushButton* d2tbut = 0;
    if ( SI().zIsTime() )
    {
	d2tbut = new uiPushButton( this, "Edit Depth/Time Model ..." );
	d2tbut->activated.notify( mCB(this,uiWellMan,edD2T) );
	d2tbut->attach( rightOf, markerbut );
    }

    uiPushButton* logsbut = new uiPushButton( this, "Add logs ..." );
    logsbut->activated.notify( mCB(this,uiWellMan,addLogs) );
    logsbut->attach( alignedBelow, topgrp );
    if ( d2tbut )
	logsbut->attach( ensureRightOf, d2tbut );

    infofld = new uiTextEdit( this, "File Info", true );
    infofld->attach( alignedBelow, markerbut );
    infofld->setPrefHeightInChar( infoheight );
    infofld->setPrefWidthInChar( infowidth );
    topgrp->setPrefWidthInChar( grpwidth );

    selChg( this );
    setCancelText( "" );
}


uiWellMan::~uiWellMan()
{
    delete ctio.ioobj; delete &ctio;
    delete welldata, wellrdr;
}


void uiWellMan::selChg( CallBacker* cb )
{
    entrylist->setCurrent( listfld->currentItem() );
    const IOObj* selioobj = entrylist->selected();
    ctio.setObj( selioobj ? selioobj->clone() : 0 );

    getCurrentWell();
    mkFileInfo();
    manipgrp->selChg( cb );
    fillLogsFld();

    BufferString msg;
    GetFreeMBOnDiskMsg( GetFreeMBOnDisk(ctio.ioobj), msg );
    toStatusBar( msg );
}


void uiWellMan::getCurrentWell()
{
    fname = ""; 
    delete wellrdr; wellrdr = 0;
    delete welldata; welldata = 0;
    if ( !ctio.ioobj ) return;
    
    mDynamicCastGet(const IOStream*,iostrm,ctio.ioobj)
    if ( !iostrm ) return;
    StreamProvider sp( iostrm->fileName() );
    sp.addPathIfNecessary( iostrm->dirName() );
    fname = sp.fileName();
    welldata = new Well::Data;
    wellrdr = new Well::Reader( fname, *welldata );
    wellrdr->getInfo();
}


void uiWellMan::fillLogsFld()
{
    logsfld->empty();
    if ( !wellrdr ) return;

    BufferStringSet lognms;
    wellrdr->getLogInfo( lognms );
    for ( int idx=0; idx<lognms.size(); idx++)
	logsfld->addItem( lognms.get(idx) );
    logsfld->selAll( false );
}


#define mErrRet(msg) \
{ uiMSG().error(msg); return; }


void uiWellMan::edMarkers( CallBacker* )
{
    if ( !welldata || !wellrdr ) return;
    if ( SI().zIsTime() && !wellrdr->getD2T() )
	mErrRet( "Cannot add markers without depth to time model" );

    Well::Data* wd;
    if ( Well::MGR().isLoaded( ctio.ioobj->key() ) )
	wd = Well::MGR().get( ctio.ioobj->key() );
    else
    {
	if ( !welldata->markers().size() )
	    wellrdr->getMarkers();
	wd = welldata;
    }


    uiMarkerDlg dlg( this );
    dlg.setMarkerSet( wd->markers() );
    if ( !dlg.go() ) return;

    dlg.getMarkerSet( wd->markers() );
    Well::Writer wtr( fname, *wd );
    if ( !wtr.putMarkers() )
	uiMSG().error( "Cannot write new markers to disk" );

    wd->markerschanged.trigger();
}


void uiWellMan::edD2T( CallBacker* )
{
    if ( !welldata || !wellrdr ) return;
    if ( !SI().zIsTime() || !wellrdr->getD2T() )
	mErrRet( "No depth to time model" );

    Well::Data* wd;
    if ( Well::MGR().isLoaded( ctio.ioobj->key() ) )
	wd = Well::MGR().get( ctio.ioobj->key() );
    else
	wd = welldata;


    uiD2TModelDlg dlg( this, *wd->d2TModel() );
    if ( !dlg.go() ) return;
    Well::Writer wtr( fname, *wd );
    if ( !wtr.putD2T() )
	uiMSG().error( "Cannot write new model to disk" );
    wd->d2tchanged.trigger();
}


#define mDeleteLogs() \
    while ( welldata->logs().size() ) \
        delete welldata->logs().remove(0);

void uiWellMan::addLogs( CallBacker* )
{
    if ( !welldata || !wellrdr ) return;
    if ( SI().zIsTime() && !wellrdr->getD2T() )
	mErrRet( "Cannot add logs without depth to time model" );

    wellrdr->getLogs();
    uiLoadLogsDlg dlg( this, *welldata );
    if ( !dlg.go() ) { mDeleteLogs(); return; }

    Well::Writer wtr( fname, *welldata );
    wtr.putLogs();
    fillLogsFld();
    const MultiID& key = ctio.ioobj->key();
    Well::MGR().reload( key );

    mDeleteLogs();
}


void uiWellMan::exportLogs( CallBacker* )
{
    if ( !logsfld->size() || !logsfld->nrSelected() ) return;

    BoolTypeSet issel;
    for ( int idx=0; idx<logsfld->size(); idx++ )
	issel += logsfld->isSelected(idx);

    if ( !welldata->d2TModel() )
	wellrdr->getD2T();

    wellrdr->getLogs();
    uiExportLogs dlg( this, *welldata, issel );
    dlg.go();

    mDeleteLogs();
}


void uiWellMan::removeLogPush( CallBacker* )
{
    if ( !logsfld->size() || !logsfld->nrSelected() ) return;

    BufferString msg;
    msg = logsfld->nrSelected() == 1 ? "This log " : "These logs ";
    msg += "will be removed from disk.\nDo you wish to continue?";
    if ( !uiMSG().askGoOn(msg) )
	return;

    wellrdr->getLogs();
    BufferStringSet logs2rem;
    for ( int idx=0; idx<logsfld->size(); idx++ )
    {
	if ( logsfld->isSelected(idx) )
	    logs2rem.add( welldata->logs().getLog(idx).name() );
    }

    for ( int idx=0; idx<logs2rem.size(); idx++ )
    {
	BufferString& name = logs2rem.get( idx );
	for ( int logidx=0; logidx<welldata->logs().size(); logidx++ )
	{
	    if ( name == welldata->logs().getLog(logidx).name() )
	    {
		Well::Log* log = welldata->logs().remove( logidx );
		delete log;
		break;
	    }
	}
    }

    logs2rem.deepErase();

    if ( wellrdr->removeAll(Well::IO::sExtLog) )
    {
	Well::Writer wtr( fname, *welldata );
	wtr.putLogs();
	fillLogsFld();
    }

    const MultiID& key = ctio.ioobj->key();
    Well::MGR().reload( key );

    mDeleteLogs();
}


void uiWellMan::renameLogPush( CallBacker* )
{
    if ( !logsfld->size() || !logsfld->nrSelected() ) mErrRet("No log selected")

    const int lognr = logsfld->currentItem() + 1;
    FilePath fp( fname ); fp.setExtension( 0 );
    BufferString logfnm = Well::IO::mkFileName( fp.fullPath(),
	    					Well::IO::sExtLog, lognr );
    StreamProvider sp( logfnm );
    StreamData sdi = sp.makeIStream();
    bool res = wellrdr->addLog( *sdi.istrm );
    sdi.close();
    if ( !res ) 
	mErrRet("Cannot read selected log")

    Well::Log* log = welldata->logs().remove( 0 );

    BufferString titl( "Rename '" );
    titl += log->name(); titl += "'";
    uiGenInputDlg dlg( this, titl, "New name",
    			new StringInpSpec(log->name()) );
    if ( !dlg.go() ) return;

    BufferString newnm = dlg.text();
    if ( logsfld->isPresent(newnm) )
	mErrRet("Name already in use")

    log->setName( newnm );
    Well::Writer wtr( fname, *welldata );
    StreamData sdo = sp.makeOStream();
    wtr.putLog( *sdo.ostrm, *log );
    sdo.close();
    fillLogsFld();
    const MultiID& key = ctio.ioobj->key();
    Well::MGR().reload( key );
    delete log;
}


void uiWellMan::mkFileInfo()
{
    if ( !welldata || !wellrdr || !ctio.ioobj )
    {
	infofld->setText( "" );
	return;
    }

    FilePath fp( fname );
    BufferString txt;
    txt += "File location: "; txt += fp.pathOnly();
    txt += "\nFile name: "; txt += fp.fileName();
    txt += "\nFile size: "; txt += getFileSize( fname );

#define mAddWellInfo(key,str) \
    if ( str.size() ) \
    {	txt += "\n"; txt += key; txt += ": "; txt += str; }

    Well::Info& info = welldata->info();
    mAddWellInfo(Well::Info::sKeyuwid,info.uwid)
    mAddWellInfo(Well::Info::sKeyoper,info.oper)
    mAddWellInfo(Well::Info::sKeystate,info.state)
    mAddWellInfo(Well::Info::sKeycounty,info.county)

    infofld->setText( txt );
}


BufferString uiWellMan::getFileSize( const char* filenm )
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
