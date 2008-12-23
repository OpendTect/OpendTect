/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiflatviewwin.cc,v 1.17 2008-12-23 12:51:22 cvsbert Exp $";

#include "uiflatviewmainwin.h"
#include "uiflatviewdockwin.h"
#include "uiflatviewcontrol.h"
#include "uiflatviewer.h"
#include "uistatusbar.h"
#include "keystrs.h"


void uiFlatViewWin::createViewers( int nr )
{
    for ( int idx=0; idx<nr; idx++ )
    {
	uiFlatViewer* vwr = new uiFlatViewer( viewerParent() );
	vwrs_ += vwr;
	handleNewViewer( vwr );
    }
}


void uiFlatViewWin::cleanUp()
{
    deepErase( vwrs_ );
}


void uiFlatViewWin::setDarkBG( bool yn )
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	vwrs_[idx]->appearance().setDarkBG( yn );
    }

    viewerParent()->setBackgroundColor( yn ? Color::Black() : Color::White() );
}


uiFlatViewMainWin::uiFlatViewMainWin( uiParent* p,
				      const uiFlatViewMainWin::Setup& setup )
    : uiMainWin(p,setup.wintitle_,setup.nrstatusfields_,setup.menubar_)
{
    createViewers( setup.nrviewers_ );
    setDeleteOnClose( setup.deleteonclose_ );
}


void uiFlatViewMainWin::addControl( uiFlatViewControl* fvc )
{
    if ( !fvc ) return;

    fvc->infoChanged.notify(mCB(this,uiFlatViewMainWin,displayInfo) );
}


void uiFlatViewMainWin::displayInfo( CallBacker* cb )
{
    mCBCapsuleUnpack(IOPar,pars,cb);
    BufferString mesg;
    int nrinfos = 0;
#define mAddSep() if ( nrinfos++ ) mesg += "; ";

    const char* vdstr = pars.find( "Variable density data" );
    const char* wvastr = pars.find( "Wiggle/VA data" );
    const char* vdvalstr = pars.find( "VD Value" );
    const char* wvavalstr = pars.find( "WVA Value" );
    const bool issame = vdstr && wvastr && !strcmp(vdstr,wvastr);
    if ( vdvalstr )
    {
	mAddSep();
	if ( issame )
	    { if ( !vdstr || !*vdstr ) vdstr = wvastr; }
	else
	    { if ( !vdstr || !*vdstr ) vdstr = "VD Val"; }
	mesg += "Val="; mesg += *vdvalstr ? vdvalstr : "undef";
	mesg += " ("; mesg += vdstr; mesg += ")";
    }
    if ( wvavalstr && !issame )
    {
	mAddSep();
	mesg += "Val="; mesg += *wvavalstr ? wvavalstr : "undef";
	if ( !wvastr || !*wvastr ) wvastr = "WVA Val";
	mesg += " ("; mesg += wvastr; mesg += ")";
    }

    const char* valstr = pars.find( sKey::Offset );
    if ( valstr && *valstr )
	{ mAddSep(); mesg += "Offs="; mesg += valstr; }
    valstr = pars.find( sKey::Azimuth );
    if ( valstr && *valstr && strcmp(valstr,"0") )
	{ mAddSep(); mesg += "Azim="; mesg += valstr; }

    valstr = pars.find( "Z" );
    if ( valstr && *valstr )
	{ mAddSep(); mesg += "Z="; mesg += valstr; }
    valstr = pars.find( sKey::Position );
    if ( valstr && *valstr )
	{ mAddSep(); mesg += "Pos="; mesg += valstr; }
    else
    {
	valstr = pars.find( sKey::TraceNr );
	if ( valstr && *valstr )
	    { mAddSep(); mesg += "TrcNr="; mesg += valstr; }
	else
	{
	    valstr = pars.find( "X" );
	    if ( !valstr ) valstr = pars.find( "X-coordinate" );
	    if ( valstr && *valstr )
		{ mAddSep(); mesg += "X="; mesg += valstr; }
	    valstr = pars.find( "Y" );
	    if ( !valstr ) valstr = pars.find( "Y-coordinate" );
	    if ( valstr && *valstr )
		{ mAddSep(); mesg += "Y="; mesg += valstr; }
	}
    }

    statusBar()->message( mesg.buf() );
}


uiFlatViewDockWin::uiFlatViewDockWin( uiParent* p,
				      const uiFlatViewDockWin::Setup& setup )
    : uiDockWin(p,setup.name_)
{
    setDockName( setup.name_ );
    createViewers( setup.nrviewers_ );
}


uiFlatViewDockWin::~uiFlatViewDockWin()
{
}
