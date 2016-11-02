/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          Sep 2006
________________________________________________________________________

-*/

#include "uiflatviewmainwin.h"
#include "uiflatviewdockwin.h"
#include "uiflatviewcontrol.h"
#include "uiflatviewer.h"
#include "uistatusbar.h"
#include "unitofmeasure.h"
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


void uiFlatViewWin::makeInfoMsg( uiString& mesg, IOPar& pars )
{
    FixedString valstr = pars.find( sKey::Position() );
    mesg.setEmpty();
    if ( valstr && *valstr )
	mesg.addSpace().append( valstr ).addTab();
    else
    {
	valstr = pars.find( sKey::TraceNr() );
	if ( valstr && *valstr )
	    mesg.addSpace().append(tr("Trace Number=")).append( valstr );
	else
	{
	    valstr = pars.find( "Inline" );
	    if ( !valstr ) valstr = pars.find( "In-line" );
	    if ( valstr && *valstr )
		mesg.addSpace().append( valstr );
	    valstr = pars.find( "Crossline" );
	    if ( !valstr ) valstr = pars.find( "Cross-line" );
	    if ( valstr && *valstr )
		mesg.append( toUiString("/") ).append( valstr );
	}
    }

    Coord3 crd( Coord3::udf() );
    valstr = pars.find( "X" );
    if ( !valstr ) valstr = pars.find( "X-coordinate" );
    if ( valstr && *valstr ) crd.x_ = toDouble( valstr );

    valstr = pars.find( "Y" );
    if ( !valstr ) valstr = pars.find( "Y-coordinate" );
    if ( valstr && *valstr ) crd.y_ = toDouble( valstr );

    valstr = pars.find( "Z" );
    if ( !valstr ) valstr = pars.find( "Z-Coord" );
    if ( valstr && *valstr ) crd.z_ = toDouble( valstr );

    if ( !crd.getXY().isUdf() )
    {
	mesg.addTab().append( toUiString("(" )).append( toString(crd.x_,0) );
	mesg.append( toUiString(", ") ).append( toString(crd.y_,0) );
    }

    if ( !mIsUdf(crd.z_) )
	mesg.append( toUiString(", ") ).append( toString(crd.z_,0) )
						    .append( toUiString(")") );
    else if ( !crd.getXY().isUdf() )
	mesg.append( ")" );
    //<-- MapDataPack has valid crd.coord() but invalid crd.z.

    mesg.addTab();

    int nrinfos = 0;
#define mAddSep() if ( nrinfos++ ) mesg.append( toUiString(";\t") );

    FixedString vdstr = pars.find( "Variable density data" );
    FixedString wvastr = pars.find( "Wiggle/VA data" );
    FixedString vdvalstr = pars.find( "VD Value" );
    FixedString wvavalstr = pars.find( "WVA Value" );
    const bool issame = vdstr && wvastr && vdstr==wvastr;
    if ( vdvalstr )
    {
	mAddSep();
	if ( issame )
	    { if ( !vdstr || !*vdstr ) vdstr = wvastr; }
	else
	    { if ( !vdstr || !*vdstr ) vdstr = "VD Val"; }
	float val = *vdvalstr ? vdvalstr.toFloat() : mUdf(float);
	mesg.append( toUiString("Val=") ); 
	mesg.append( mIsUdf(val) ? toUiString("undef") : toUiString(vdvalstr) );
	mesg.append(toUiString("(%1)").arg(toUiString(vdstr)));
    }
    if ( wvavalstr && !issame )
    {
	mAddSep();
	float val = *wvavalstr ? wvavalstr.toFloat() : mUdf(float);
	mesg.append( toUiString("Val=") ); 
	mesg.append( mIsUdf(val) ? toUiString("undef") : toUiString(wvavalstr));
	if ( !wvastr || !*wvastr ) wvastr = "WVA Val";
	mesg.append( toUiString("(%1)").arg(toUiString(wvastr)) );
    }

    float val;
    if ( pars.get(sKey::Offset(),val) )
    {
	if ( SI().xyInFeet() )
	    val *= mToFeetFactorF;

	mAddSep(); mesg.append(toUiString("Offs=%1").arg(toUiString(val)));
	mesg.addSpace().append(toUiString(SI().xyUnitString()));
    }

    valstr = pars.find( sKey::Azimuth() );
    if ( valstr && valstr!="0" )
	{ mAddSep(); mesg.append(tr("Azimuth=%1").arg(toUiString(valstr))); }

    valstr = pars.find( "Ref/SP number" );
    if ( valstr && *valstr )
	{ mAddSep(); mesg.append(toUiString("Ref/SP=%1")
						    .arg(toUiString(valstr))); }
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
    uiString mesg;
    makeInfoMsg( mesg, pars );
    statusBar()->message( mesg );
}


uiFlatViewDockWin::uiFlatViewDockWin( uiParent* p,
				      const uiFlatViewDockWin::Setup& setup )
    : uiDockWin(p,setup.name_)
{
    setDockName( toUiString(setup.name_) );
    createViewers( setup.nrviewers_ );
}


uiFlatViewDockWin::~uiFlatViewDockWin()
{
}
