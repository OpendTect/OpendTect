/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvey.cc,v 1.40 2003-10-17 14:19:03 bert Exp $
________________________________________________________________________

-*/

#include "uisurvey.h"
#include "survinfoimpl.h"
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
#include "uitextedit.h"
#include "uimain.h"
#include "uifont.h"
#include "iodrawtool.h"
#include "strmprov.h"
#include "dirlist.h"
#include "ioman.h"
#include "ctxtioobj.h"
#include "filegen.h"
#include "iostrm.h"

#include <math.h>
#include <fstream>

extern "C" const char* GetSurveyName();
extern "C" const char* GetSurveyFileName();
extern "C" const char* GetBaseDataDir();
extern "C" void SetSurveyName(const char*);


class TutHandling
{
public:

static bool copy()
{
    BufferString from = GetDataFileName( "Tutorial" );
    if ( !File_exists(from) )
    {
        uiMSG().error( "Tutorial not installed" );
        return false;
    }

    BufferString to( GetBaseDataDir() );
    to = File_getFullPath( to, "Tutorial" );
    if ( File_exists(to) )
    {
        uiMSG().error( "A survey 'Tutorial' already exists.\n"
                        "Please rename or remove.");
        return false;
    }

    if ( !File_copy( from, to, YES ) )
    {
        uiMSG().error( "Cannot create new survey directory for Tutorial" );
        return false;
    }

    File_makeWritable( to, YES, YES );

    return true;
}


static void fill()
{
    BufferString dirnm = File_getFullPath(GetBaseDataDir(),"Tutorial");
    dirnm = File_getFullPath( dirnm, "Seismics" );
    IOM().setRootDir( dirnm );
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
    fname = GetDataFileName( fname );
    iostrm->setFileName( fname );
    IOM().commitChanges( *iostrm );
}

};


uiSurvey::uiSurvey( uiParent* p, bool isgdi )
	: uiDialog(p,uiDialog::Setup("Survey selection",
		   "Select and setup survey","0.3.1"))
{
    SurveyInfo::produceWarnings( false );
    const int lbwidth = 250;
    const int mapwdth = 300;
    const int maphght = 300;
    const int noteshght = 5;
    const int totwdth = lbwidth + mapwdth + 10;

    const char* ptr = GetBaseDataDir();
    if ( !ptr ) return;

    dirlist = new DirList( ptr, 1 );
    dirlist->sort();

    uiGroup* rightgrp = new uiGroup( this, "Survey selection right" );
    mapcanvas = new uiCanvas( rightgrp, "Survey map" );
    mapcanvas->setPrefHeight( maphght );
    mapcanvas->setPrefWidth( mapwdth );
    mapcanvas->setStretch( 0, 0 );
    mapcanvas->preDraw.notify( mCB(this,uiSurvey,doCanvas) );

    uiGroup* leftgrp = new uiGroup( this, "Survey selection left" );
    listbox = new uiListBox( leftgrp, *dirlist, "Surveys" );
    listbox->setCurrentItem( GetSurveyName() );
    listbox->selectionChanged.notify( mCB(this,uiSurvey,selChange) );
    listbox->doubleClicked.notify( mCB(this,uiSurvey,accept) );
    listbox->setPrefWidth( lbwidth );
    listbox->setStretch( 2, 2 );
    leftgrp->attach( leftOf, rightgrp );

    newbut = new uiPushButton( leftgrp, "New ..." );
    newbut->activated.notify( mCB(this,uiSurvey,newButPushed) );
    newbut->attach( alignedBelow, listbox );
    rmbut = new uiPushButton( leftgrp, "Remove ..." );
    rmbut->activated.notify( mCB(this,uiSurvey,rmButPushed) );
    rmbut->attach( rightAlignedBelow, listbox );

    editbut = new uiPushButton( rightgrp, "Edit ..." );
    editbut->activated.notify( mCB(this,uiSurvey,editButPushed) );
    editbut->attach( alignedBelow, mapcanvas );
    convbut = new uiPushButton( rightgrp, "X/Y <-> I/C ..." );
    convbut->activated.notify( mCB(this,uiSurvey,convButPushed) );
    convbut->attach( rightAlignedBelow, mapcanvas );
    uiButton* tutbut = 0;
    if ( isgdi )
    {
	static const char* tutdirnm = "Tutorial";
	const bool direxists = dirlist->indexOf(tutdirnm) >= 0;
	BufferString dirnm( GetDataFileName(tutdirnm) );
	const bool tutinst = File_exists( dirnm );
	if ( tutinst && !direxists )
	{
	    tutbut = new uiPushButton( leftgrp, "Get Tutorial" );
	    tutbut->attach( centeredBelow, listbox );
	    tutbut->activated.notify( mCB(this,uiSurvey,tutButPushed) );
	}
    }

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
    setOkText( "Select" );
}


uiSurvey::~uiSurvey()
{
    delete dirlist;
    delete mapcanvas;
    delete survinfo;
    delete survmap;
}


void uiSurvey::newButPushed( CallBacker* )
{
    ioDrawTool& dt = *mapcanvas->drawTool();
    dt.beginDraw(); dt.clear(); dt.endDraw();

    BufferString oldnm = listbox->getText();
  
    BufferString from( GetSoftwareDir() );
    from = File_getFullPath( GetSoftwareDir(), "data" );
    from = File_getFullPath( from, "BasicSurvey" );
    delete survinfo;
    survinfo = SurveyInfo::read( from );
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


bool uiSurvey::survInfoDialog()
{
    BufferString selnm( listbox->getText() );
    BufferString dgbdata( GetBaseDataDir() );

    uiSurveyInfoEditor dlg( this, survinfo, mCB(this,uiSurvey,updateInfo) );
    if ( !dlg.go() ) return false;

    bool doupd = true;
    if ( dlg.dirnmChanged() )
    {
        const char* newnm = dlg.dirName();
        if ( *newnm )
        {
            BufferString newfname(File_getFullPath( dgbdata, newnm ));
            if ( File_exists(newfname) )
            {
                BufferString errmsg( "Cannot rename directory:\n" );
                errmsg += newfname;
                errmsg += "\nexists";
                ErrMsg( errmsg );
            }
            else
            {
                BufferString fname(File_getFullPath( dgbdata, selnm ));
                File_rename( fname, newfname );
                updateSvyList(); doupd = false;
                updateSvyFile();
                selnm = newnm;
            }
        }
    }
    else
	selnm = dlg.dirName();

    if ( doupd ) updateSvyList();
    listbox->setCurrentItem( selnm );

    return true;
}


void uiSurvey::rmButPushed( CallBacker* )
{
    BufferString selnm( listbox->getText() );
    BufferString msg( "This will remove the entire survey:\n\t" );
    msg += selnm;
    msg += "\nAre you sure you wish to continue?";
    if ( !uiMSG().askGoOn( msg ) ) return;

    msg = File_getFullPath( GetBaseDataDir(), selnm );
    if ( !File_remove( msg, YES ) )
    {
        msg += "\nnot removed properly";
        return;
    }

    updateSvyList();
    const char* ptr = GetSurveyName();
    if ( ptr && selnm == ptr )
    {
        BufferString newsel( listbox->getText() );
        writeSurveyName( newsel );
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
    dirlist->update();
    dirlist->sort();
    if ( !dirlist->size() ) updateInfo(0);
    listbox->empty();
    listbox->addItems( dirlist );
}


bool uiSurvey::updateSvyFile()
{
    const char* ptr = GetSurveyName();
    if ( ptr ) ptr = File_getFileName( ptr );
    BufferString seltxt( listbox->getText() );
    if ( seltxt == "" ) return true;

    if ( (!ptr || seltxt != ptr ) && !writeSurveyName( seltxt ) )
    {
        ErrMsg( "Cannot update the .dgbSurvey file in your $HOME" );
        return false;
    }
    FileNameString fname( File_getFullPath(GetBaseDataDir(),seltxt) );
    if ( !File_exists(fname) )
    {
        ErrMsg( "Survey directory does not exist anymore" );
        return false;
    }
    delete SurveyInfo::theinst_;
    SurveyInfo::theinst_ = SurveyInfo::read( fname );
    SetSurveyName( seltxt );

    return true;
}


bool uiSurvey::writeSurveyName( const char* nm )
{
    const char* ptr = GetSurveyFileName();
    if ( !ptr )
    {
        ErrMsg( "Error in survey system. Please check $HOME." );
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
    BufferString inlinfo( "In-line range: " );
    BufferString crlinfo( "Cross-line range: " );
    BufferString zinfo( "Z range (" );
    zinfo += survinfo->getZUnit(); zinfo += "): ";
    BufferString bininfo( "Bin size (m/line): " );

    if ( survinfo->rangeUsable() )
    {
	inlinfo += survinfo->range().start.inl;
	inlinfo += " - "; inlinfo += survinfo->range().stop.inl;
	inlinfo += "  step: "; inlinfo += survinfo->inlStep();
	
	crlinfo += survinfo->range().start.crl;
	crlinfo += " - "; crlinfo += survinfo->range().stop.crl;
	crlinfo += "  step: "; crlinfo += survinfo->crlStep();

	if ( survinfo->is3D() )
	{
	    const SurveyInfo3D& si = *(SurveyInfo3D*)survinfo;
	    float inldist = si.transform( BinID(0,0) ).distance(
		    si.transform( BinID(1,0) ) );
	    float crldist = si.transform( BinID(0,0) ).distance(
		    si.transform( BinID(0,1) ) );

#define mkString(dist) \
    nr = (int)(dist*100+.5); bininfo += nr/100; \
    rest = nr%100; bininfo += rest < 10 ? ".0" : "."; bininfo += rest; \

	    int nr, rest;    
	    bininfo += "inl: "; mkString(inldist);
	    bininfo += "  crl: "; mkString(crldist);
	}
    }

    if ( survinfo->zRangeUsable() )
    {
#define mkZString(nr) \
    zinfo += istime ? mNINT(1000*nr) : nr;

	bool istime = survinfo->zIsTime(); 
        mkZString( survinfo->zRange().start );
	zinfo += " - "; mkZString( survinfo->zRange().stop );
	zinfo += "  step: "; mkZString( survinfo->zRange().step );
    }


    inllbl->setText( inlinfo );
    crllbl->setText( crlinfo );
    binlbl->setText( bininfo );
    zlbl->setText( zinfo );
    notes->setText( survinfo->comment() );

    bool anysvy = dirlist->size();
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
    if ( dlg )
	survinfo = dlg->getSurvInfo();
    else
	getSurvInfo();

    mkInfo();
    survmap->drawMap( survinfo );
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
    mDynamicCastGet(uiCanvas*,mapcanvas,c)
    if (!mapcanvas) return;
    survmap = new uiSurveyMap( mapcanvas );
    survmap->drawMap( survinfo );
}


bool uiSurvey::rejectOK( CallBacker* )
{
    SurveyInfo::produceWarnings( true );
    return true;
}


bool uiSurvey::acceptOK( CallBacker* )
{
    writeComments();
    SurveyInfo::produceWarnings( true );
    if ( !updateSvyFile() || !IOMan::newSurvey() )
    {
	SurveyInfo::produceWarnings( false );
	return false;
    }

    updateViewsGlobal();
    return true;
}


void uiSurvey::updateViewsGlobal()
{
    BufferString capt( GetProjectVersionName() );
    BufferString fnm( GetSoftwareDir() );
    fnm = File_getFullPath( fnm, ".rel" );
    if ( File_exists(fnm) )
    {
	char* ptr = strrchr( capt.buf(), 'V' );
	if ( ptr )
	{
	    char vstr[80];
	    ifstream strm( fnm );
	    strm.getline( vstr, 80 );
	    if ( vstr[0] )
	    {
		*ptr++ = '\0';
		capt += vstr;
	    }
	}
    }

    const char* usr = GetSoftwareUser();
    if ( usr && *usr )
	{ capt += " ["; capt += usr; capt += "]"; }

    if ( SI().name() != "" )
    {
	capt += ": ";
	capt += SI().name();
    }

    uiMain::setTopLevelCaption( capt );
}


void uiSurvey::getSurvInfo()
{
    BufferString fname( File_getFullPath(GetBaseDataDir(), listbox->getText()));
    survinfo = SurveyInfo::read( fname );
}
