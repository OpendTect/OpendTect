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
#include "uistrings.h"


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
    BufferString valstr = pars.find( sKey::Position() );
    mesg.setEmpty();
    if ( !valstr.isEmpty() )
	mesg.appendPlainText( valstr );
    else
    {
	valstr = pars.find( sKey::TraceNr() );
	if ( !valstr.isEmpty() )
	    mesg.constructWordWith( toUiString("%1=%2")
		    .arg( uiStrings::sTraceNumber() ).arg( valstr ) );
	else
	{
	    valstr = pars.find( "Inline" );
	    if ( valstr.isEmpty() )
		valstr = pars.find( "In-line" );
	    BufferString xlvalstr = pars.find( "Crossline" );
	    if ( xlvalstr.isEmpty() )
		xlvalstr = pars.find( "Cross-line" );
	    if ( !valstr.isEmpty() && !xlvalstr.isEmpty() )
		mesg.constructWordWith( toUiString("%1/%2").arg( valstr )
						 .arg( xlvalstr ) );
	}
    }

    Coord3 crd( Coord3::udf() );
    valstr = pars.find( sKey::X() );
    if ( valstr.isEmpty() )
	valstr = pars.find( "X-coordinate" );
    if ( !valstr.isEmpty() )
	crd.x_ = toDouble( valstr );
    valstr = pars.find( sKey::Y() );
    if ( valstr.isEmpty() )
	valstr = pars.find( "Y-coordinate" );
    if ( !valstr.isEmpty() )
	crd.y_ = toDouble( valstr );
    valstr = pars.find( sKey::Z() );
    if ( valstr.isEmpty() )
	valstr = pars.find( "Z-Coord" );
    if ( !valstr.isEmpty() )
	crd.z_ = toDouble( valstr );

    const bool havecoord = !mIsUdf(crd.x_) && !mIsUdf(crd.y_);
    const bool havez = !mIsUdf(crd.z_);
    if ( havecoord && havez )
	mesg.constructWordWith( toUiString(crd.toPrettyString()), true );
    else if ( havecoord )
	mesg.constructWordWith( toUiString(crd.getXY().toPrettyString()), true);
    else if ( havez )
	mesg.constructWordWith( toUiString("Z=%1").arg(crd.z_), true );

    mesg.appendPlainText( "\t" );

    int nrinfos = 0;
#define mAddSep() if ( nrinfos++ ) mesg.appendPlainText( ";\t" );

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
	mesg.constructWordWith( toUiString("%1=%2").arg( uiStrings::sValue() )
				.arg( mIsUdf(val) ? uiStrings::sUndef()
						  : toUiString(vdvalstr) ) );
	mesg.constructWordWith( toUiString(" (%1)").arg( vdstr ) );
    }
    if ( wvavalstr && !issame )
    {
	mAddSep();
	float val = *wvavalstr ? wvavalstr.toFloat() : mUdf(float);
	mesg.constructWordWith( toUiString("%1=%2").arg( uiStrings::sValue() )
			.arg( mIsUdf(val) ? uiStrings::sUndef()
					  : toUiString(wvavalstr) ) );
	if ( !wvastr || !*wvastr ) wvastr = "WVA Val";
	mesg.constructWordWith( toUiString(" (%1)").arg( wvastr ) );
    }

    float val;
    if ( pars.get(sKey::Offset(),val) && !mIsUdf(val) )
    {
	if ( SI().xyInFeet() )
	    val *= mToFeetFactorF;
	mAddSep();
	mesg.constructWordWith( toUiString("%1=%2")
				.arg( uiStrings::sOffset() ).arg( val )
				.withSurvXYUnit() );
    }

    valstr = pars.find( sKey::Azimuth() );
    if ( !valstr.isEmpty() && valstr != "0" )
    {
	mAddSep();
	mesg.constructWordWith( toUiString("%1=%2")
				.arg( uiStrings::sAzimuth() ).arg( valstr ) );
    }

    valstr = pars.find( "Ref/SP number" );
    if ( !valstr.isEmpty() )
    {
	mAddSep();
	mesg.constructWordWith( toUiString("%1=%2")
				.arg( uiStrings::sSPNumber() ).arg( valstr ) );
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
