/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#define mNrXYDec 2

#include "uiflatviewmainwin.h"
#include "uiflatviewdockwin.h"
#include "uiflatviewcontrol.h"
#include "uiflatviewer.h"
#include "uistatusbar.h"
#include "unitofmeasure.h"
#include "keystrs.h"


uiFlatViewWin::uiFlatViewWin()
{}


uiFlatViewWin::~uiFlatViewWin()
{}


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

    const OD::Color clr = yn ? OD::Color::Black() : OD::Color::White();
    dockParent()->setBackgroundColor( clr );
    viewerParent()->setBackgroundColor( clr );
}


void uiFlatViewWin::makeInfoMsg( BufferString& mesg, IOPar& pars )
{
    BufferString valstr = pars.find( sKey::Position() );
    mesg.setEmpty();
    if ( !valstr.isEmpty() )
	mesg.add( valstr ).addTab();
    else
    {
	valstr = pars.find( sKey::TraceNr() );
	if ( !valstr.isEmpty() )
	    mesg.add("TrcNr=").add( valstr );
	else
	{
	    valstr = pars.find( "Inline" );
	    if ( valstr.isEmpty() )
		valstr = pars.find( "In-line" );

	    if ( !valstr.isEmpty() )
		mesg.add( valstr );

	    valstr = pars.find( "Crossline" );
	    if ( valstr.isEmpty() )
		valstr = pars.find( "Cross-line" );

	    if ( !valstr.isEmpty() )
		mesg.add( "/" ).add( valstr );
	}
    }

    Coord3 crd( Coord3::udf() );
    valstr = pars.find( "X" );
    if ( valstr.isEmpty() )
	valstr = pars.find( "X-coordinate" );

    if ( !valstr.isEmpty() )
	crd.x = toDouble( valstr );

    valstr = pars.find( "Y" );
    if ( valstr.isEmpty() )
	valstr = pars.find( "Y-coordinate" );

    if ( !valstr.isEmpty() )
	crd.y = toDouble( valstr );

    valstr = pars.find( "Z" );
    if ( valstr.isEmpty() )
	valstr = pars.find( "Z-Coord" );

    if ( !valstr.isEmpty() )
	crd.z = toDouble( valstr );

    if ( !crd.coord().isUdf() )
    {
	mesg.addTab().add( "(" ).add( toString(crd.x,mNrXYDec) );
	mesg.add( ", " ).add( toString(crd.y,mNrXYDec) );
    }

    if ( !mIsUdf(crd.z) )
	mesg.add( ", " ).add( toString(crd.z) ).add( ")" );
    else if ( !crd.coord().isUdf() )
	mesg.add( ")" );
    //<-- MapDataPack has valid crd.coord() but invalid crd.z.

    mesg.addTab();

    int nrinfos = 0;
#define mAddSep() if ( nrinfos++ ) mesg += ";\t";

    BufferString vdstr = pars.find( FlatView::Viewer::sKeyVDData() );
    BufferString wvastr = pars.find( FlatView::Viewer::sKeyWVAData() );
    const BufferString vdvalstr = pars.find( FlatView::Viewer::sKeyVDVal() );
    const BufferString wvavalstr =
				pars.find( FlatView::Viewer::sKeyWVAVal() );
    const bool issame = vdstr.isEqual( wvastr );
    if ( vdvalstr.str() )
    {
	mAddSep();
	if ( issame && vdstr.isEmpty() )
		vdstr = wvastr;
	else if ( !issame && vdstr.isEmpty() )
	    vdstr = FlatView::Viewer::sKeyVDVal();

	float val = !vdvalstr.isEmpty() ? vdvalstr.toFloat() : mUdf(float);
	mesg += "Val="; mesg += mIsUdf(val) ? "undef" : vdvalstr.buf();
	mesg += " ("; mesg += vdstr; mesg += ")";
    }

    if ( wvavalstr.str() && !issame )
    {
	mAddSep();
	float val = wvavalstr.str() ? wvavalstr.toFloat() : mUdf(float);
	mesg += "Val="; mesg += mIsUdf(val) ? "undef" : wvavalstr.buf();
	if ( wvastr.isEmpty() )
	    wvastr = FlatView::Viewer::sKeyWVAVal();

	mesg += " ("; mesg += wvastr; mesg += ")";
    }

    float val;
    if ( pars.get(sKey::Offset(),val) )
    {
	mAddSep(); mesg += "Offs="; mesg += val;
	mesg += " "; mesg += SI().getXYUnitString();
    }

    valstr = pars.find( sKey::Azimuth() );
    if ( valstr.str() && valstr!="0" )
	{ mAddSep(); mesg += "Azim="; mesg += valstr; }

    valstr = pars.find( "Ref/SP number" );
    if ( !valstr.isEmpty() )
    {
	mAddSep();
	mesg += "Ref/SP=";
	mesg += valstr;
    }
}



uiFlatViewMainWin::uiFlatViewMainWin( uiParent* p,
				      const uiFlatViewMainWin::Setup& setup )
    : uiMainWin(p,setup.wintitle_,setup.nrstatusfields_,setup.menubar_)
{
    createViewers( setup.nrviewers_ );
    setDeleteOnClose( setup.deleteonclose_ );
}


uiFlatViewMainWin::~uiFlatViewMainWin()
{}


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
    statusBar()->message( mToUiStringTodo(mesg.buf()) );
}


uiFlatViewDockWin::uiFlatViewDockWin( uiParent* p,
				      const uiFlatViewDockWin::Setup& setup )
    : uiDockWin(p,setup.name_)
{
    setDockName( mToUiStringTodo(setup.name_) );
    createViewers( setup.nrviewers_ );
}


uiFlatViewDockWin::~uiFlatViewDockWin()
{
}
