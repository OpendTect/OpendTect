/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvey.cc,v 1.10 2001-10-05 10:10:12 nanne Exp $
________________________________________________________________________

-*/

#include "uisurvey.h"
#include "dirlist.h"
#include "filegen.h"
#include "genc.h"
#include "ioman.h"
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
#include "uitextedit.h"

#include <fstream>
#include <math.h>

extern "C" const char* GetSurveyName();
extern "C" const char* GetSurveyFileName();
extern "C" int GetSurveyName_reRead;

uiSurvey::uiSurvey( uiParent* p )
	: uiDialog(p,"Survey setup")

{
    setTitleText( "Setup and select the survey" );

    const int lbwidth = 250;
    const int mapwdth = 300;
    const int maphght = 300;
    const int noteshght = 130;
    const int totwdth = lbwidth + mapwdth + 10;

    const char* ptr = getenv( "dGB_DATA" );
    if ( !ptr ) return;

    dirlist = new DirList( ptr, 1 );
    dirlist->sort();
    ptr = GetSurveyName();
    if ( ptr )
    {
        UserIDObject* uidobj = (*dirlist)[ptr];
        if ( uidobj ) dirlist->setCurrent( uidobj );
    }

    uiGroup* selgrp = new uiGroup( this, "Survey selection" );
    mapcanvas = new uiCanvas( selgrp, "Survey map" );
    mapcanvas->setPrefHeight( maphght );
    mapcanvas->setPrefWidth( mapwdth );
    mapcanvas->setStretch( 0, 0 );
    mapcanvas->preDraw.notify( mCB(this,uiSurvey,doCanvas) );
    listbox = new uiListBox( selgrp, dirlist );
    listbox->selectionChanged.notify( mCB(this,uiSurvey,selChange) );
    listbox->doubleClicked.notify( mCB(this,uiSurvey,editButPushed) );
    listbox->attach( leftOf, mapcanvas );
    listbox->attach( heightSameAs, mapcanvas );
    listbox->setPrefWidth( lbwidth );
    listbox->setStretch( 0, 0 );

    newbut = new uiPushButton( selgrp, "New ..." );
    newbut->activated.notify( mCB(this,uiSurvey,newButPushed) );
    newbut->attach( alignedBelow, listbox );
    rmbut = new uiPushButton( selgrp, "Remove ..." );
    rmbut->activated.notify( mCB(this,uiSurvey,rmButPushed) );
    rmbut->attach( rightAlignedBelow, listbox );
    editbut = new uiPushButton( selgrp, "Edit ..." );
    editbut->activated.notify( mCB(this,uiSurvey,editButPushed) );
    editbut->attach( alignedBelow, mapcanvas );
    convbut = new uiPushButton( selgrp, "X/Y <-> I/C ..." );
    convbut->activated.notify( mCB(this,uiSurvey,convButPushed) );
    convbut->attach( rightAlignedBelow, mapcanvas );

    uiSeparator* horsep1 = new uiSeparator( this );
    horsep1->setPrefWidth( totwdth );
    horsep1->attach( ensureBelow, selgrp );

    uiGroup* infogrp = new uiGroup( this, "Survey information" );
    infogrp->setFont( uiFontList::get(FontData::defaultKeys()[2]) ); 
    infogrp->attach( alignedBelow, selgrp );
    infogrp->attach( ensureBelow, horsep1 );
    infogrp->setPrefWidth( totwdth );
    uiLabel* irange1 = new uiLabel( infogrp, "In-line range:" );
    uiLabel* xrange1 = new uiLabel( infogrp, "Cross-line range:" );
    uiLabel* zrange1 = new uiLabel( infogrp, "Time range (s):" );
    uiLabel* binsize1 = new uiLabel( infogrp, "Bin size (m/line):" );
    xrange1->attach( alignedBelow, irange1 );
    zrange1->attach( rightOf, irange1, 225 );
    binsize1->attach( alignedBelow, zrange1 );

    irange2 = new uiLabel( infogrp, "" );
    xrange2 = new uiLabel( infogrp, "" );
    zrange2 = new uiLabel( infogrp, "" );
    binsize2 = new uiLabel( infogrp, "" );
    irange2->attach( rightOf, irange1 );
    xrange2->attach( rightOf, xrange1 );
    zrange2->attach( rightOf, zrange1 );
    binsize2->attach( rightOf, binsize1 );
   
    uiSeparator* horsep2 = new uiSeparator( this );
    horsep2->attach( ensureBelow, infogrp );
    horsep2->setPrefWidth( totwdth );

    uiLabel* notelbl = new uiLabel( this, "Notes:" );
    notelbl->attach( alignedBelow, horsep2 );
    notes = new uiTextEdit( this, "Notes" );
    notes->attach( alignedBelow, notelbl);
    notes->setPrefHeight( noteshght );
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
    survinfo = new SurveyInfo( from );
    survinfo->dirname = "";
    mkInfo();
    if ( !survInfoDialog() )
    {
	getSurvInfo();
	mkInfo();
	survmap->drawMap( survinfo );
    }
}

void uiSurvey::editButPushed( CallBacker* )
{
    if ( !survInfoDialog() )
    {
	getSurvInfo();
	mkInfo();
	survmap->drawMap( survinfo );
    }
}

bool uiSurvey::survInfoDialog()
{
    BufferString selnm( listbox->getText() );
    BufferString dgbdata( getenv("dGB_DATA") );

    uiSurveyInfoEditor dlg( this, survinfo, survmap );
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
                update(); doupd = false;
                updateSvyFile();
                selnm = newnm;
            }
        }
    }
    else
	selnm = dlg.dirName();

    if ( doupd ) update();
    listbox->setCurrentItem( selnm );

    return true;
}

void uiSurvey::rmButPushed( CallBacker* )
{

    BufferString selnm( listbox->getText() );
    BufferString msg( "This will remove the entire survey:\n\n" );
    msg += selnm;
    msg += "\n\nAre you sure you wish to continue?";
    if ( !uiMSG().askGoOn( msg ) ) return;

    msg = File_getFullPath( getenv("dGB_DATA"), selnm );
    if ( !File_remove( msg, YES, YES ) )
    {
        msg += "\nnot removed properly";
        return;
    }

    update();
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

void uiSurvey::update()
{
    dirlist->update();
    dirlist->sort();
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
    FileNameString fname( File_getFullPath(getenv("dGB_DATA"),seltxt) );
    if ( !File_exists(fname) )
    {
        ErrMsg( "Survey directory does not exist anymore" );
        return false;
    }
    delete SurveyInfo::theinst_;
    SurveyInfo::theinst_ = new SurveyInfo( fname );

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

    ofstream strm( ptr );
    if ( strm.fail() )
    {
        BufferString errmsg = "Cannot write to ";
        errmsg += ptr;
        ErrMsg( errmsg );
        return false;
    }
    strm << nm;

    return true;
}


void uiSurvey::mkInfo()
{
    if ( survinfo->rangeUsable() )
    {
	BufferString inlinfo = survinfo->range().start.inl;
	inlinfo += " - "; inlinfo += survinfo->range().stop.inl;
	inlinfo += "  step: "; inlinfo += survinfo->step().inl;
	
	BufferString crlinfo = survinfo->range().start.crl;
	crlinfo += " - "; crlinfo += survinfo->range().stop.crl;
	crlinfo += "  step: "; crlinfo += survinfo->step().crl;

	double xinl = survinfo->b2c_.getTransform(true).b;
	double yinl = survinfo->b2c_.getTransform(false).b;
	double xcrl = survinfo->b2c_.getTransform(true).c;
	double ycrl = survinfo->b2c_.getTransform(false).c;

	double ibsz = double( int( 100*sqrt(xinl*xinl + yinl*yinl)+.5 ) ) / 100;
	double cbsz = double( int( 100*sqrt(xcrl*xcrl + ycrl*ycrl)+.5 ) ) / 100;
	BufferString bininfo = "inline: "; bininfo += ibsz;
	bininfo += "  crossline: "; bininfo += cbsz;

	irange2->setText( inlinfo );
	xrange2->setText( crlinfo );
	binsize2->setText( bininfo );
    }
    else
    {
	irange2->setText( "" );
        xrange2->setText( "" );
        binsize2->setText( "" );
    }

    if ( survinfo->zRangeUsable() )
    {
	BufferString zinfo = survinfo->zRange().start;
	zinfo += " - "; zinfo += survinfo->zRange().stop;
	zinfo += "  step: "; zinfo += survinfo->zRange().step;
	zrange2->setText( zinfo );
    }
    else
	zrange2->setText( "" );

    notes->setText( survinfo->comment() );
}


void uiSurvey::selChange()
{
    writeComments();
    getSurvInfo();
    mkInfo();
    survmap->drawMap( survinfo );
}


void uiSurvey::writeComments()
{
    BufferString txt = notes->text();
    if ( txt == survinfo->comment() ) return;

    survinfo->setComment( txt );
    if ( !survinfo->write( getenv("dGB_DATA") ) )
        ErrMsg( "Failed to write survey info.\nNo changes committed." );
}


void uiSurvey::doCanvas( CallBacker* c )
{
    mDynamicCastGet(uiCanvas*,mapcanvas,c)
    if (!mapcanvas) return;
    survmap = new uiSurveyMap( mapcanvas, survinfo );
    survmap->drawMap( survinfo );
}


bool uiSurvey::acceptOK( CallBacker* )
{
    writeComments();
    GetSurveyName_reRead = true;
    if ( !updateSvyFile() || !IOMan::newSurvey() )
	return false;

    return true;
}

void uiSurvey::getSurvInfo()
{
    BufferString fname( File_getFullPath(getenv("dGB_DATA"),
			  listbox->getText()) );
    survinfo = new SurveyInfo( fname );
}
