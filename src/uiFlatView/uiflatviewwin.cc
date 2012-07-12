/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          Sep 2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiflatviewwin.cc,v 1.33 2012-07-12 15:04:44 cvsbruno Exp $";

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
	uiFlatViewer* vwr = new uiFlatViewer( dockParent() );
	vwrs_ += vwr;
	vwr->setStretch( 2, 2 );
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

    dockParent()->setBackgroundColor( yn ? Color::Black() : Color::White() );
    viewerParent()->setBackgroundColor( yn ? Color::Black() : Color::White() );
}


void uiFlatViewWin::makeInfoMsg( BufferString& mesg, IOPar& pars ) const
{
    int nrinfos = 0;
#define mAddSep() if ( nrinfos++ ) mesg += ";\t";

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
	float val = *vdvalstr ? toFloat( vdvalstr ) : mUdf(float);
	mesg += "Val="; mesg += mIsUdf(val) ? "undef" : vdvalstr;
	mesg += " ("; mesg += vdstr; mesg += ")";
    }
    if ( wvavalstr && !issame )
    {
	mAddSep();
	float val = *wvavalstr ? toFloat( wvavalstr ) : mUdf(float);
	mesg += "Val="; mesg += mIsUdf(val) ? "undef" : wvavalstr;
	if ( !wvastr || !*wvastr ) wvastr = "WVA Val";
	mesg += " ("; mesg += wvastr; mesg += ")";
    }

    const char* valstr = pars.find( sKey::Offset() );
    if ( valstr && *valstr )
	{ mAddSep(); mesg += "Offs="; mesg += valstr; }
    valstr = pars.find( sKey::Azimuth() );
    if ( valstr && *valstr && strcmp(valstr,"0") )
	{ mAddSep(); mesg += "Azim="; mesg += valstr; }

    valstr = pars.find( "Z" );
    if ( valstr && *valstr )
	{ mAddSep(); mesg += "Z="; mesg += valstr; }
    valstr = pars.find( sKey::Position() );
    if ( valstr && *valstr )
	{ mAddSep(); mesg += "Pos="; mesg += valstr; }
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

	valstr = pars.find( sKey::TraceNr() );
	if ( valstr && *valstr )
	{
	    mAddSep(); mesg += "TrcNr="; mesg += valstr;
	    valstr = pars.find( "Reference position" );
	    if ( valstr && *valstr )
		{ mAddSep(); mesg += "Ref/SP="; mesg += valstr; }
	    valstr = pars.find( "Z-Coord" );
	    if ( valstr && *valstr )
		{ mAddSep(); mesg += "Z="; mesg += valstr; }
	}
	else
	{
	    valstr = pars.find( "Inline" );
	    if ( !valstr ) valstr = pars.find( "In-line" );
	    if ( valstr && *valstr )
		{ mAddSep(); mesg += "Inline="; mesg += valstr; }
	    valstr = pars.find( "Crossline" );
	    if ( !valstr ) valstr = pars.find( "Cross-line" );
	    if ( valstr && *valstr )
		{ mAddSep(); mesg += "Crossline="; mesg += valstr; }
	}
    }
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


void uiFlatViewWin::setInitialSize( int w, int h )
{
    int vwrw = w / vwrs_.size(); int vwrh = h / vwrs_.size();
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	vwrs_[idx]->setInitialSize( uiSize(vwrw,vwrh) );
}


void uiFlatViewMainWin::setInitialSize( int w, int h )
{
    uiFlatViewWin::setInitialSize( w, h );
    setPrefWidth( w ); setPrefHeight( h );
}


void uiFlatViewMainWin::displayInfo( CallBacker* cb )
{
    mCBCapsuleUnpack(IOPar,pars,cb);
    BufferString mesg; 
    makeInfoMsg( mesg, pars );
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
