/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvey.cc,v 1.81 2007-08-30 10:12:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisurvey.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uicanvas.h"
#include "uiconvpos.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uisurvinfoed.h"
#include "uisurvmap.h"
#include "uisetdatadir.h"
#include "uitextedit.h"
#include "uifileinput.h"
#include "uicursor.h"
#include "uimain.h"
#include "uifont.h"
#include "iodrawtool.h"
#include "dirlist.h"
#include "ioman.h"
#include "ctxtioobj.h"
#include "filegen.h"
#include "filepath.h"
#include "oddirs.h"
#include "iostrm.h"
#include "strmprov.h"
#include "envvars.h"
#include "cubesampling.h"
#include "odver.h"
#include <iostream>
#include <math.h>

extern "C" const char* GetSurveyName();
extern "C" const char* GetSurveyFileName();
extern "C" void SetSurveyName(const char*);


static BufferString getTrueDir( const char* dn )
{
    BufferString dirnm = dn;
    FilePath fp;
    while ( File_isLink(dirnm) )
    {
	BufferString newdirnm = File_linkTarget(dirnm);
	fp.set( newdirnm );
	if ( !fp.isAbsolute() )
	{
	    FilePath dirnmfp( dirnm );
	    dirnmfp.setFileName( 0 );
	    fp.setPath( dirnmfp.fullPath() );
	}
	dirnm = fp.fullPath();
    }
    return dirnm;
}


static bool copySurv( const char* from, const char* todirnm, int mb )
{
    FilePath fp( GetBaseDataDir() ); fp.add( todirnm );
    const BufferString todir( fp.fullPath() );
    if ( File_exists(todir) )
    {
	BufferString msg( "A survey '" );
	msg += todirnm;
	msg += "' already exists.\nYou will have to remove it first";
        uiMSG().error( msg );
        return false;
    }

    BufferString msg;
    if ( mb > 0 )
	{ msg += mb; msg += " MB of data"; }
    else
	msg += "An unknown amount";
    msg += " of data needs to be copied.\nDuring the copy, OpendTect will "
	    "freeze.\nDepending on the data transfer rate, this can take "
	    "a long time!\n\nDo you wish to continue?";
    if ( !uiMSG().askGoOn( msg ) )
	return false;

    const BufferString fromdir = getTrueDir( FilePath(from).fullPath() );

    uiCursorChanger cc( uiCursor::Wait );
    if ( !File_copy( fromdir, todir, YES ) )
    {
        uiMSG().error( "Cannot copy the survey data" );
        return false;
    }

    File_makeWritable( todir, YES, YES );
    return true;
}


class TutHandling
{
public:

static bool copy()
{
    static const char* dirnm = "Tutorial";
    BufferString from = mGetSetupFileName( dirnm );
    if ( !File_exists(from) )
    {
        uiMSG().error( "Tutorial not installed" );
        return false;
    }

    return copySurv( from, dirnm, 4 );
}


static void fill()
{
    FilePath fp( GetBaseDataDir() );
    fp.add( "Tutorial" ).add( "Seismics" );
    IOM().setRootDir( fp.fullPath() );
    IOM().to( IOObjContext::StdSelTypeNames[0] );
    IOObj* ioobj = IOM().get( "100010.2" );
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    if ( !ioobj || !iostrm )
    {
	BufferString errmsg = "Original Tutorial survey is corrupt!";
	uiMSG().error( errmsg );
	return;
    }

    BufferString fname( iostrm->fileName() );
    fname = mGetSetupFileName( fname );
    iostrm->setFileName( fname );
    IOM().commitChanges( *iostrm );
}

};


uiSurvey::uiSurvey( uiParent* p, bool isgdi )
    : uiDialog(p,uiDialog::Setup("Survey selection",
	       "Select and setup survey","0.3.1"))
    , initialdatadir(GetBaseDataDir())
    , survinfo(0)
    , survmap(0)
    , mapcanvas(0)
{
    SurveyInfo::produceWarnings( false );
    const int lbwidth = 250;
    const int mapwdth = 300;
    const int maphght = 300;
    const int noteshght = 5;
    const int totwdth = lbwidth + mapwdth + 10;

    const char* ptr = GetBaseDataDir();
    if ( !ptr ) return;

    mkDirList();

    uiGroup* rightgrp = new uiGroup( this, "Survey selection right" );
    mapcanvas = new uiCanvas( rightgrp, "Survey map" );
    mapcanvas->setPrefHeight( maphght );
    mapcanvas->setPrefWidth( mapwdth );
    mapcanvas->setStretch( 0, 0 );
    mapcanvas->preDraw.notify( mCB(this,uiSurvey,doCanvas) );

    uiGroup* leftgrp = new uiGroup( this, "Survey selection left" );
    listbox = new uiListBox( leftgrp, dirlist, "Surveys" );
    listbox->setCurrentItem( GetSurveyName() );
    listbox->selectionChanged.notify( mCB(this,uiSurvey,selChange) );
    listbox->doubleClicked.notify( mCB(this,uiSurvey,accept) );
    listbox->setPrefWidth( lbwidth );
    listbox->setStretch( 2, 2 );
    leftgrp->attach( leftOf, rightgrp );

    newbut = new uiPushButton( leftgrp, "&New",
	    			mCB(this,uiSurvey,newButPushed), false );
    newbut->attach( rightOf, listbox );
    newbut->setPrefWidthInChar( 12 );
    rmbut = new uiPushButton( leftgrp, "&Remove",
	    			mCB(this,uiSurvey,rmButPushed), false );
    rmbut->attach( alignedBelow, newbut );
    rmbut->setPrefWidthInChar( 12 );
    editbut = new uiPushButton( leftgrp, "&Edit",
	    			mCB(this,uiSurvey,editButPushed), false );
    editbut->attach( alignedBelow, rmbut );
    editbut->setPrefWidthInChar( 12 );
    copybut = new uiPushButton( leftgrp, "C&opy",
	    			mCB(this,uiSurvey,copyButPushed), false );
    copybut->attach( alignedBelow, editbut );
    copybut->setPrefWidthInChar( 12 );

    convbut = new uiPushButton( rightgrp, "&X/Y <-> I/C",
	    			mCB(this,uiSurvey,convButPushed), false );
    convbut->attach( centeredBelow, mapcanvas );
    uiButton* tutbut = 0;
    if ( isgdi )
    {
	static const char* tutdirnm = "Tutorial";
	const bool direxists = dirlist.indexOf(tutdirnm) >= 0;
	BufferString dirnm( mGetSetupFileName(tutdirnm) );
	const bool tutinst = File_exists( dirnm );
	if ( tutinst && !direxists )
	{
	    tutbut = new uiPushButton( leftgrp, "&Get Tutorial", true );
	    tutbut->attach( alignedBelow, listbox );
	    tutbut->activated.notify( mCB(this,uiSurvey,tutButPushed) );
	}
    }

    datarootbut = new uiPushButton( leftgrp, "&Set Data Root",
	    			mCB(this,uiSurvey,dataRootPushed), false );
    datarootbut->attach( tutbut ? rightAlignedBelow : centeredBelow, listbox );

    uiSeparator* horsep1 = new uiSeparator( this );
    horsep1->setPrefWidth( totwdth );
    horsep1->attach( stretchedBelow, rightgrp, -2 );
    horsep1->attach( ensureBelow, leftgrp );

    uiGroup* infoleft = new uiGroup( this, "Survey info left" );
    uiGroup* inforight = new uiGroup( this, "Survey info right" );
    infoleft->attach( alignedBelow, leftgrp );
    infoleft->attach( ensureBelow, horsep1 );
    inforight->attach( alignedBelow, rightgrp );
    inforight->attach( ensureBelow, horsep1 );

    infoleft->setFont( uiFontList::get(FontData::key(FontData::ControlSmall)) );
    inforight->setFont( uiFontList::get(FontData::key(FontData::ControlSmall)));

    inllbl = new uiLabel( infoleft, "" ); 
    crllbl = new uiLabel( infoleft, "" );
    zlbl = new uiLabel( inforight, "" ); 
    binlbl = new uiLabel( inforight, "" );
#if 0
    inllbl->setHSzPol( uiObject::widevar );
    crllbl->setHSzPol( uiObject::widevar );
    zlbl->setHSzPol( uiObject::widevar );
    binlbl->setHSzPol( uiObject::widevar );
#else
    inllbl->setPrefWidthInChar( 40 );
    crllbl->setPrefWidthInChar( 40 );
    zlbl->setPrefWidthInChar( 40 );
    binlbl->setPrefWidthInChar( 40 );
#endif

    crllbl->attach( alignedBelow, inllbl );
    binlbl->attach( alignedBelow, zlbl );
   
    uiSeparator* horsep2 = new uiSeparator( this );
    horsep2->attach( stretchedBelow, infoleft, -2 );
    horsep2->setPrefWidth( totwdth );

    uiLabel* notelbl = new uiLabel( this, "Notes:" );
    notelbl->attach( alignedBelow, horsep2 );
    notes = new uiTextEdit( this, "Notes" );
    notes->attach( alignedBelow, notelbl);
    notes->setPrefHeightInChar( noteshght );
    notes->setPrefWidth( totwdth );
   
    getSurvInfo(); 
    mkInfo();
    setOkText( "&Ok (Select)" );
}


uiSurvey::~uiSurvey()
{
    delete survinfo;
    delete survmap;
}


void uiSurvey::newButPushed( CallBacker* )
{
    if ( !mapcanvas ) return;
    BufferString oldnm = listbox->getText();
  
    FilePath fp( GetSoftwareDir() );
    fp.add( "data" ).add( "BasicSurvey" );
    delete survinfo;
    survinfo = SurveyInfo::read( fp.fullPath() );
    survinfo->dirname = "";
    mkInfo();
    if ( !survInfoDialog() )
	updateInfo(0);

    rmbut->setSensitive(true);
    editbut->setSensitive(true);
    convbut->setSensitive(true);
}


void uiSurvey::editButPushed( CallBacker* )
{
    if ( !survInfoDialog() )
	updateInfo(0);
}


class uiSurveyGetCopyDir : public uiDialog
{
public:

uiSurveyGetCopyDir( uiParent* p, const char* cursurv )
	: uiDialog(p,uiDialog::Setup("Import survey from location",
		   "Copy surveys from any data root","0.3.1"))
{
    BufferString curfnm;
    if ( cursurv && *cursurv )
    {
	FilePath fp( GetBaseDataDir() ); fp.add( cursurv );
	curfnm = fp.fullPath();
    }
    inpfld = new uiFileInput( this,
	    	"Opendtect survey directory to copy",
		uiFileInput::Setup(curfnm).directories(true));
    inpfld->setDefaultSelectionDir( GetBaseDataDir() );
    inpfld->valuechanged.notify( mCB(this,uiSurveyGetCopyDir,inpSel) );

    newdirnmfld = new uiGenInput( this, "New survey directory name", "" );
    newdirnmfld->attach( alignedBelow, inpfld );
}


void inpSel( CallBacker* )
{
    fname = inpfld->fileName();
    FilePath fp( fname );
    newdirnmfld->setText( fp.fileName() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool acceptOK( CallBacker* )
{
    fname = inpfld->fileName();
    if ( !File_exists(fname) )
	mErrRet( "Selected directory does not exist" );
    if ( !File_isDirectory(fname) )
	mErrRet( "Please select a valid directory" );
    FilePath fp( fname );
    fp.add( ".survey" );
    if ( !File_exists( fp.fullPath() ) )
	mErrRet( "This is not an OpendTect survey directory" );

    newdirnm = newdirnmfld->text();
    if ( newdirnm.isEmpty() )
	{ inpSel(0); newdirnm = newdirnmfld->text(); }
    cleanupString( newdirnm.buf(), NO, NO, YES );

    return true;
}

    BufferString	fname;
    BufferString	newdirnm;
    uiFileInput*	inpfld;
    uiGenInput*		newdirnmfld;

};


void uiSurvey::copyButPushed( CallBacker* )
{
    uiSurveyGetCopyDir dlg( this, listbox->getText() );
    if ( !dlg.go() )
	return;

    if ( !copySurv( dlg.fname, dlg.newdirnm, -1 ) )
	return;

    updateSvyList();
    listbox->setCurrentItem( dlg.newdirnm );
    updateSvyFile();
}


void uiSurvey::dataRootPushed( CallBacker* )
{
    uiSetDataDir dlg( this );
    if ( dlg.go() )
    {
	mkDirList();
	updateSvyList();
    }
}


void uiSurvey::mkDirList()
{
    dirlist.deepErase();
    getSurveyList( dirlist );
}


bool uiSurvey::survInfoDialog()
{
    uiSurveyInfoEditor dlg( this, *survinfo );
    dlg.survparchanged.notify( mCB(this,uiSurvey,updateInfo) );
    if ( !dlg.go() )
	return false;

    updateSvyList();
    listbox->setCurrentItem( dlg.dirName() );
//    updateSvyFile();
    return true;
}


void uiSurvey::rmButPushed( CallBacker* )
{
    BufferString selnm( listbox->getText() );
    const BufferString seldirnm = FilePath(GetBaseDataDir())
					    .add(selnm).fullPath();
    const BufferString truedirnm = getTrueDir( seldirnm );

    BufferString msg( "This will remove the entire survey:\n\t" );
    msg += selnm;
    msg += "\nFull path: "; msg += truedirnm;
    msg += "\nAre you sure you wish to continue?";
    if ( !uiMSG().askGoOn( msg ) ) return;


    uiCursor::setOverride( uiCursor::Wait );
    bool rmres = File_remove( truedirnm, YES );
    uiCursor::restoreOverride();
    if ( !rmres )
    {
        msg = truedirnm; msg += "\nnot removed properly";
        return;
    }

    if ( seldirnm != truedirnm ) // must have been a link
	File_remove( seldirnm, NO );

    updateSvyList();
    const char* ptr = GetSurveyName();
    if ( ptr && selnm == ptr )
    {
        BufferString newsel( listbox->getText() );
        writeSurveyName( newsel );
	if ( button(CANCEL) ) button(CANCEL)->setSensitive( false );
    }
}


void uiSurvey::convButPushed( CallBacker* )
{
    uiConvertPos dlg( this, survinfo );
    dlg.go();
}


void uiSurvey::tutButPushed( CallBacker* )
{
    if ( !TutHandling::copy() )
	return;
    updateSvyList();
    TutHandling::fill();

    rmbut->setSensitive(true);
    editbut->setSensitive(true);
    convbut->setSensitive(true);
}


void uiSurvey::updateSvyList()
{
    mkDirList();
    if ( dirlist.isEmpty() ) updateInfo(0);
    listbox->empty();
    listbox->addItems( dirlist );
}


bool uiSurvey::updateSvyFile()
{
    BufferString seltxt( listbox->getText() );
    if ( seltxt.isEmpty() ) return true;

    if ( !writeSurveyName( seltxt ) )
    {
        ErrMsg( "Cannot update the 'survey' file in $HOME/.od/" );
        return false;
    }
    if ( !File_exists( FilePath(GetBaseDataDir()).add(seltxt).fullPath() ) )
    {
        ErrMsg( "Survey directory does not exist anymore" );
        return false;
    }

    newSurvey();
    SetSurveyName( seltxt );

    return true;
}


bool uiSurvey::writeSurveyName( const char* nm )
{
    const char* ptr = GetSurveyFileName();
    if ( !ptr )
    {
        ErrMsg( "Error in survey system. Please check $HOME/.od/" );
        return false;
    }

    StreamData sd = StreamProvider( ptr ).makeOStream();
    if ( !sd.usable() )
    {
        BufferString errmsg = "Cannot write to ";
        errmsg += ptr;
        ErrMsg( errmsg );
        return false;
    }

    *sd.ostrm << nm;

    sd.close();
    return true;
}


void uiSurvey::mkInfo()
{
    const SurveyInfo& si = *survinfo;
    BufferString inlinfo( "In-line range: " );
    BufferString crlinfo( "Cross-line range: " );
    BufferString zinfo( "Z range " );
    zinfo += si.getZUnit(); zinfo += ": ";
    BufferString bininfo( "Bin size (m/line): " );

    if ( si.sampling(false).hrg.totalNr() )
    {
	inlinfo += si.sampling(false).hrg.start.inl;
	inlinfo += " - "; inlinfo += si.sampling(false).hrg.stop.inl;
	inlinfo += "  step: "; inlinfo += si.inlStep();
	
	crlinfo += si.sampling(false).hrg.start.crl;
	crlinfo += " - "; crlinfo += si.sampling(false).hrg.stop.crl;
	crlinfo += "  step: "; crlinfo += si.crlStep();

	float inldist = si.inlDistance();
	float crldist = si.crlDistance();

#define mkString(dist) \
nr = (int)(dist*100+.5); bininfo += nr/100; \
rest = nr%100; bininfo += rest < 10 ? ".0" : "."; bininfo += rest; \

	int nr, rest;    
	bininfo += "inl: "; mkString(inldist);
	bininfo += "  crl: "; mkString(crldist);
    }

#define mkZString(nr) \
    zinfo += istime ? mNINT(1000*nr) : nr;

    const bool istime = si.zIsTime(); 
    mkZString( si.zRange(false).start );
    zinfo += " - "; mkZString( si.zRange(false).stop );
    zinfo += "  step: "; mkZString( si.zRange(false).step );

    inllbl->setText( inlinfo );
    crllbl->setText( crlinfo );
    binlbl->setText( bininfo );
    zlbl->setText( zinfo );
    notes->setText( si.comment() );

    bool anysvy = dirlist.size();
    rmbut->setSensitive( anysvy );
    editbut->setSensitive( anysvy );
    convbut->setSensitive( anysvy );
}


void uiSurvey::selChange()
{
    writeComments();
    updateInfo(0);
}


void uiSurvey::updateInfo( CallBacker* cb )
{
    mDynamicCastGet(uiSurveyInfoEditor*,dlg,cb);
    if ( !dlg )
	getSurvInfo();

    mkInfo();
    mapcanvas->update();
}


void uiSurvey::writeComments()
{
    BufferString txt = notes->text();
    if ( txt == survinfo->comment() ) return;

    survinfo->setComment( txt );
    if ( !survinfo->write( GetBaseDataDir() ) )
        ErrMsg( "Failed to write survey info.\nNo changes committed." );
}


void uiSurvey::doCanvas( CallBacker* c )
{
    if ( !mapcanvas ) return;
    if ( !survmap ) survmap = new uiSurveyMap( mapcanvas );
    survmap->drawMap( survinfo );
}


bool uiSurvey::rejectOK( CallBacker* )
{
    if ( initialdatadir != GetBaseDataDir() )
    {
	if ( !uiSetDataDir::setRootDataDir( initialdatadir ) )
	{
	    uiMSG().error( "As we cannot reset to the old Data Root,\n"
		    	   "You *have to* select a survey now!" );
	    return false;
	}
    }

    SurveyInfo::produceWarnings( true );
    return true;
}



bool uiSurvey::acceptOK( CallBacker* )
{
    writeComments();
    SurveyInfo::produceWarnings( true );
    if ( !updateSvyFile() || !IOMan::newSurvey() )
    { SurveyInfo::produceWarnings( false ); return false; }

    newSurvey();
    updateViewsGlobal();
    return true;
}


void uiSurvey::updateViewsGlobal()
{
    BufferString capt( "OpendTect V" );
    capt += GetFullODVersion();
    capt += "/"; capt += __plfsubdir__;

    const char* usr = GetSoftwareUser();
    if ( usr && *usr )
	{ capt += " ["; capt += usr; capt += "]"; }

    if ( !SI().name().isEmpty() )
    {
	capt += ": ";
	capt += SI().name();
    }

    uiMain::setTopLevelCaption( capt );
}


void uiSurvey::getSurvInfo()
{
    BufferString fname = FilePath( GetBaseDataDir() )
	    		.add( listbox->getText() ).fullPath();
    delete survinfo;
    survinfo = SurveyInfo::read( fname );
}


void uiSurvey::newSurvey()
{
    delete SurveyInfo::theinst_;
    SurveyInfo::theinst_ = 0;
}


void uiSurvey::getSurveyList( BufferStringSet& list )
{
    BufferString basedir = GetBaseDataDir();
    DirList dl( basedir, DirList::DirsOnly );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const char* dirnm = dl.get( idx );
	if ( matchString("_New_Survey_",dirnm) )
	    continue;

	FilePath fp( basedir );
	fp.add( dirnm ).add( ".survey" );
	BufferString survfnm = fp.fullPath();
	if ( File_exists(survfnm) )
	    list.add( dirnm );
    }

    list.sort();
}


bool uiSurvey::survTypeOKForUser( bool is2d )
{
    const bool dowarn = (is2d && !SI().has2D()) || (!is2d && !SI().has3D());
    if ( !dowarn ) return true;

    BufferString warnmsg( "Your survey is set up as '" );
    warnmsg += is2d ? "3-D only'.\nTo be able to actually use 2-D"
		    : "2-D only'.\nTo be able to actually use 3-D";
    warnmsg += " data\nyou will have to change the survey setup.";
    warnmsg += "\n\nDo you wish to continue?";

    return uiMSG().askGoOn( warnmsg );
}
