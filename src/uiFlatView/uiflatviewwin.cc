/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiflatviewmainwin.h"

#include "keystrs.h"
#include "uistrings.h"
#include "uiflatviewcontrol.h"
#include "uiflatviewdockwin.h"
#include "uiflatviewer.h"
#include "uistatusbar.h"
#include "unitofmeasure.h"


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


bool uiFlatViewWin::validIdx( int idx ) const
{
    return vwrs_.validIdx( idx );
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


void uiFlatViewWin::makeInfoMsg( const IOPar& pars, uiString& msg )
{
    if ( vwrs_.isEmpty() || pars.isEmpty() )
	return;

    int vieweridx = 0;
    pars.get( uiFlatViewControl::sKeyViewerIdx(), vieweridx );
    if ( !validIdx(vieweridx) )
	vieweridx = 0;

    makeInfoMsg( viewer(vieweridx), pars, msg );
}


void uiFlatViewWin::makeInfoMsg( const FlatView::Viewer& vwr,
				 const IOPar& pars, uiString& msg )
{
    BinID bid = BinID::udf();
    int trcnr = mUdf(int);
    float val = mUdf(float);
    Coord3 crd( Coord3::udf() );
    pars.get( sKey::Coordinate(), crd );

    uiString posmsg, crdmsg;
    if ( pars.get(sKey::Position(),bid) )
    {
	posmsg = toUiString( bid );
    }
    else if ( pars.get(sKey::TraceNr(),trcnr) )
    {
	float spval = mUdf(float);
	const bool hassp = pars.get(sKey::Shotpoint(),spval) && !mIsUdf(spval);
	posmsg = toUiString( "TrcNr" );
	posmsg.addMoreInfo( trcnr );
	if ( hassp )
	{
	    uiString spmsg = toUiString( "SP" );
	    spmsg.addMoreInfo( toUiString(spval,0,'f',2) );
	    posmsg.appendPhrase( spmsg, uiString::Comma, uiString::OnSameLine );
	}
    }

    const bool withz = !mIsUdf(crd.z_);
    if ( withz )
	crd.z_ *= vwr.annotUserFactor();

    if ( !crd.coord().isUdf() )
    {
	crdmsg = withz ? toUiString( crd, vwr.nrXYDec(), vwr.nrZDec() )
		       : toUiString( crd.coord(), vwr.nrXYDec() );
    }
    else if ( !mIsUdf(crd.z_) )
    {
	const int width = 5 + (vwr.nrZDec() > 0 ? vwr.nrZDec()+1 : 0);
	crdmsg.set( vwr.zDomain(true)->getLabel() )
	      .addMoreInfo( toUiString(crd.z_,width,'f',vwr.nrZDec()) );
    }

    if ( !posmsg.isEmpty() || !crdmsg.isEmpty() )
    {
	msg = !posmsg.isEmpty() && !crdmsg.isEmpty()
	    ? toUiString("%1	%2").arg( posmsg ).arg( crdmsg )
	    : (posmsg.isEmpty() ? crdmsg : posmsg);
    }

    int nrinfos = 0;
#define mAddSep() if ( nrinfos++ ) msg.appendPhrase( uiString::empty(), \
				   uiString::SemiColon, uiString::OnSameLine );

    BufferString wvastr = pars.find( FlatView::Viewer::sKeyWVAData() );
    BufferString vdstr = pars.find( FlatView::Viewer::sKeyVDData() );
    const BufferString wvavalstr = pars.find( FlatView::Viewer::sKeyWVAVal() );
    const BufferString vdvalstr = pars.find( FlatView::Viewer::sKeyVDVal() );
    const bool issame = vdstr.isEqual( wvastr );
    if ( !wvavalstr.isEmpty() )
    {
	mAddSep();
	if ( issame && wvastr.isEmpty() )
	    wvastr = vdstr;
	else if ( !issame && wvastr.isEmpty() )
	    wvastr = FlatView::Viewer::sKeyWVAVal();

	val = wvavalstr.isEmpty() ? mUdf(float) : wvavalstr.toFloat();
	uiString valstr = toUiString( "%1 = %2" )
				.arg( uiStrings::sValue() )
				.arg( mIsUdf(val) ? "undef" : wvavalstr.buf() );
	msg.appendPhrase( valstr, uiString::Tab, uiString::OnSameLine );
	valstr = toUiString( wvastr );
	valstr.parenthesize();
	msg.appendPhrase( valstr, uiString::Space, uiString::OnSameLine );
    }

    if ( !vdvalstr.isEmpty() && !issame )
    {
	mAddSep();
	val = vdvalstr.isEmpty() ? mUdf(float) : vdvalstr.toFloat();
	uiString valstr = toUiString( "%1 = %2" )
				.arg( uiStrings::sValue() )
				.arg( mIsUdf(val) ? "undef" : vdvalstr.buf() );
	msg.appendPhrase( valstr, uiString::Tab, uiString::OnSameLine );
	if ( vdstr.isEmpty() )
	    vdstr = FlatView::Viewer::sKeyVDVal();

	valstr = toUiString( vdstr );
	valstr.parenthesize();
	msg.appendPhrase( valstr, uiString::Space, uiString::OnSameLine );
    }

    if ( pars.get(sKey::Offset(),val) && !mIsUdf(val) )
    {
	mAddSep();
	Seis::OffsetType offsettype = SI().xyInFeet()
				    ? Seis::OffsetType::OffsetFeet
				    : Seis::OffsetType::OffsetMeter;
	Seis::getOffsetType( pars, offsettype );
	const UnitOfMeasure* uom = UnitOfMeasure::offsetUnit( offsettype );
	uiString valstr = toUiString( "%1 %2 = %3" );
	valstr.arg( Seis::isOffsetDist(offsettype) ? uiStrings::sOffset()
						   : uiStrings::sAngle() )
	      .arg( uom->getUiLabel() )
	      .arg( toUiString(val,5,'f',0) );
	msg.appendPhrase( valstr, uiString::Tab, uiString::OnSameLine );
    }

    if ( pars.get(sKey::Azimuth(),val) && !mIsUdf(val) )
    {
	mAddSep();
	OD::AngleType azityp = OD::AngleType::Degrees;
	Seis::getAzimuthType( pars, azityp );
	const UnitOfMeasure* uom = UnitOfMeasure::angleUnit( azityp );
	const uiString valstr = toUiString( "%1 %2 = %3" )
				.arg( uiStrings::sAzimuth() )
				.arg( uom->getUiLabel() )
				.arg( toUiString(val,4,'f',0) );
	msg.appendPhrase( valstr, uiString::Tab, uiString::OnSameLine );
    }
}


// uiFlatViewMainWin

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
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack(const IOPar&,pars,cb);
    uiString msg;
    makeInfoMsg( pars, msg );
    statusBar()->message( msg );
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
