/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          September 2003
 RCS:           $Id: uiwellman.cc,v 1.3 2003-10-15 15:15:55 bert Exp $
________________________________________________________________________

-*/

#include "uiwellman.h"
#include "iodirentry.h"
#include "ioobj.h"
#include "ioman.h"
#include "iostrm.h"
#include "ctxtioobj.h"
#include "wellimpasc.h"
#include "welltransl.h"
#include "welldata.h"
#include "welllogset.h"
#include "welllog.h"
#include "wellmarker.h"
#include "wellreader.h"
#include "wellwriter.h"
#include "uilistbox.h"
#include "uiioobjmanip.h"
#include "uitextedit.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "pixmap.h"
#include "filegen.h"
#include "streamconn.h"

#include "uitable.h"
#include "uicolor.h"
#include "ptrman.h"
#include "uimsg.h"

#include "uilistbox.h"
#include "uifileinput.h"
#include "uilabel.h"


static const int infoheight = 8;
static const int infowidth = 50;
static const int grpwidth = 75;


uiWellMan::uiWellMan( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Surface file management",
				 "Manage wells",
				 "").nrstatusflds(1))
    , ctio(*mMkCtxtIOObj(Well))
    , ioobj(0)
{
    IOM().to( ctio.ctxt.stdSelKey() );
    entrylist = new IODirEntryList( IOM().dirPtr(), ctio.ctxt );

    uiGroup* topgrp = new uiGroup( this, "Top things" );
    listfld = new uiListBox( topgrp, entrylist->Ptr() );
    listfld->setHSzPol( uiObject::medvar );
    listfld->selectionChanged.notify( mCB(this,uiWellMan,selChg) );

    manipgrp = new uiIOObjManipGroup( listfld, *entrylist, "well" );

    logsfld = new uiListBox( topgrp, "Available logs" );
    logsfld->attach( rightTo, manipgrp );
    logsfld->setToolTip( "Available logs" );

    butgrp = new uiButtonGroup( topgrp, "" );
    butgrp->attach( rightTo, logsfld );
    const ioPixmap rempm( GetDataFileName("trashcan.png") );
    rembut = new uiToolButton( butgrp, "Remove", rempm );
    rembut->activated.notify( mCB(this,uiWellMan,remLogPush) );
    rembut->setToolTip( "Remove selected log" );

    uiPushButton* markerbut = new uiPushButton( this, "Edit markers ..." );
    markerbut->activated.notify( mCB(this,uiWellMan,addMarkers) );
    markerbut->attach( alignedBelow, topgrp );

    uiPushButton* logsbut = new uiPushButton( this, "Add logs ..." );
    logsbut->activated.notify( mCB(this,uiWellMan,addLogs) );
    logsbut->attach( rightOf, markerbut );

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
    delete &ctio;
}


void uiWellMan::selChg( CallBacker* cb )
{
    entrylist->setCurrent( listfld->currentItem() );
    ioobj = entrylist->selected();

    mkFileInfo();
    manipgrp->selChg( cb );

    BufferString msg;
    GetFreeMBOnDiskMsg( GetFreeMBOnDisk(ioobj), msg );
    toStatusBar( msg );
}


void uiWellMan::remLogPush( CallBacker* )
{
    if ( !logsfld->size() || !logsfld->nrSelected() ) return;

    return;
}


void uiWellMan::fillLogsFld( const Well::LogSet& logset )
{
    logsfld->empty();
    for ( int idx=0; idx<logset.size(); idx++)
	logsfld->addItem( logset.getLog(idx).name() );
    logsfld->selAll( false );
}


void uiWellMan::mkFileInfo()
{
    if ( !ioobj )
    {
	infofld->setText( "" );
	return;
    }

    BufferString txt;
    mDynamicCastGet(StreamConn*,conn,ioobj->getConn(Conn::Read))
    if ( !conn ) return;

    BufferString fname( conn->fileName() );
    txt += "\nLocation: "; txt += File_getPathOnly( fname );
    txt += "\nFile name: "; txt += File_getFileName( fname );
    txt += "\nFile size: "; txt += getFileSize( fname );

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


void uiWellMan::addMarkers( CallBacker* )
{
    if ( !ioobj ) return;

    mDynamicCastGet(StreamConn*,conn,ioobj->getConn(Conn::Read))
    if ( !conn ) return;
    PtrMan<Well::Data> well = new Well::Data;
    Well::Reader rdr( conn->fileName(), *well );
    if ( !rdr.getMarkers() )
    {
//	uiMSG().error( "Cannot read markers" );
//	return;
    }

    uiMarkerDlg dlg( this );
    dlg.setMarkerSet( well->markers() );
    if ( dlg.go() )
    {
	dlg.getMarkerSet( well->markers() );
	Well::Writer wtr( conn->fileName(), *well );
	wtr.putMarkers();
    }
}


void uiWellMan::addLogs( CallBacker* )
{
    if ( !ioobj ) return;

    mDynamicCastGet(StreamConn*,conn,ioobj->getConn(Conn::Read))
    if ( !conn ) return;
    PtrMan<Well::Data> well = new Well::Data;
    Well::Reader rdr( conn->fileName(), *well );
    rdr.getLogs();

    uiLoadLogsDlg dlg( this, *well );
    if ( dlg.go() )
    {
	Well::Writer wtr( conn->fileName(), *well );
	wtr.putLogs();
    }
}



// ==================================================================

static const char* collbls[] =
{
    "Name", "Depth", "Color", 0
};

static const int maxnrrows = 10;
static const int initnrrows = 5;

uiMarkerDlg::uiMarkerDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Well Markers","Define marker properties"))
{
    table = new uiTable( this, uiTable::Setup().rowdesc("Marker")
	    				       .rowcangrow(), "Table" );
    table->setColumnLabels( collbls );
    table->setColumnReadOnly( 2, true );
    table->setNrRows( initnrrows );
    markerAdded(0);

    table->rowInserted.notify( mCB(this,uiMarkerDlg,markerAdded) );
    table->leftClicked.notify( mCB(this,uiMarkerDlg,mouseClick) );
}


void uiMarkerDlg::markerAdded( CallBacker* )
{
    const int nrrows = table->nrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	BufferString labl( "Marker " );
	labl += idx+1;
	table->setRowLabel( idx, labl );
    }
}


void uiMarkerDlg::mouseClick( CallBacker* )
{
    uiTable::RowCol rc = table->notifiedCell();
    if ( rc.col != 2 ) return;

    Color newcol = table->getColor( rc );
    if ( select(newcol,this,"Marker color") )
	table->setColor( rc, newcol );
}



void uiMarkerDlg::setMarkerSet( const ObjectSet<Well::Marker>& markers )
{
    const int nrmarkers = markers.size();
    if ( !nrmarkers ) return;

    int nrrows = nrmarkers + initnrrows < maxnrrows ? nrmarkers + initnrrows 
						    : nrmarkers;
    table->setNrRows( nrrows );
    for ( int idx=0; idx<nrmarkers; idx++ )
    {
	const Well::Marker* marker = markers[idx];
	table->setText( uiTable::RowCol(idx,0), marker->name() );
	table->setValue( uiTable::RowCol(idx,1), marker->dah );
	table->setColor( uiTable::RowCol(idx,2), marker->color );
    }

    markerAdded(0);
}


void uiMarkerDlg::getMarkerSet( ObjectSet<Well::Marker>& markers ) const
{
    deepErase( markers );
    const int nrrows = table->nrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	const char* name = table->text( uiTable::RowCol(idx,0) );
	if ( !name || !*name ) continue;

	Well::Marker* marker = new Well::Marker( name );
	marker->dah = table->getfValue( uiTable::RowCol(idx,1) );
	marker->color = table->getColor( uiTable::RowCol(idx,2) );
	markers += marker;
    }
}


// ==================================================================

static const char* lasfilefilt = "*.las;;*.LAS;;*.txt;;*";
static const char* ftlbltxt = "(ft)";
static const char* mlbltxt = "(m)";
static const float defundefval = -999.25;


uiLoadLogsDlg::uiLoadLogsDlg( uiParent* p, Well::Data& wd_ )
    : uiDialog(p,uiDialog::Setup("Logs","Define log parameters",""))
    , wd(wd_)
{
    lasfld = new uiFileInput( this, "Input (pseudo-)LAS logs file",
			      uiFileInput::Setup().filter(lasfilefilt)
			      			  .withexamine() );
    lasfld->setDefaultSelectionDir( GetDataDir() );
    lasfld->valuechanged.notify( mCB(this,uiLoadLogsDlg,lasSel) );

    intvfld = new uiGenInput( this, "Depth interval to load",
			      FloatInpIntervalSpec(false) );
    intvfld->attach( alignedBelow, lasfld );
    unitlbl = new uiLabel( this, mlbltxt );
    unitlbl->attach( rightOf, intvfld );

    udffld = new uiGenInput( this, "Undefined value in logs",
                    FloatInpSpec(defundefval));
    udffld->attach( alignedBelow, intvfld );

    logsfld = new uiLabeledListBox( this, "Select logs", true );
    logsfld->attach( alignedBelow, udffld );
}


void uiLoadLogsDlg::lasSel( CallBacker* )
{
    const char* lasfnm = lasfld->text();
    if ( !lasfnm || !*lasfnm ) return;

    Well::Data wd_; Well::AscImporter wdai( wd_ );
    Well::AscImporter::LasFileInfo lfi;
    const char* res = wdai.getLogInfo( lasfnm, lfi );
    if ( res ) { uiMSG().error( res ); return; }

    logsfld->box()->empty();
    logsfld->box()->addItems( lfi.lognms );

    udffld->setValue( lfi.undefval );
    intvfld->setValue( lfi.zrg );
}


bool uiLoadLogsDlg::acceptOK( CallBacker* )
{
    Well::AscImporter wdai( wd );
    Well::AscImporter::LasFileInfo lfi;
    lfi.undefval = udffld->getValue();
    assign( lfi.zrg, intvfld->getFInterval() );
    for ( int idx=0; idx<logsfld->box()->size(); idx++ )
    {
	if ( logsfld->box()->isSelected(idx) )
	    lfi.lognms += new BufferString( logsfld->box()->textOfItem(idx) );
    }

    const char* lasfnm = lasfld->text();
    if ( !lasfnm || !*lasfnm ) 
    { uiMSG().error("Enter valid filename"); return false; }

    const char* res = wdai.getLogs( lasfnm, lfi, false );
    if ( res ) { uiMSG().error( res ); return false; }

    return true;
}
