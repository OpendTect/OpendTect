/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2008
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uicalcpoly2horvol.cc,v 1.4 2009-08-11 14:49:31 cvsbert Exp $";

#include "uicalcpoly2horvol.h"

#include "emmanager.h"
#include "emhorizon3d.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "pickset.h"
#include "survinfo.h"
#include "polygon.h"
#include "gridder2d.h"

#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "uilabel.h"
#include "uimsg.h"

#include <math.h>


uiCalcPoly2HorVol::uiCalcPoly2HorVol( uiParent* p, const Pick::Set& ps )
	: uiDialog(p,Setup("Calculate volume",
		    "Volume estimation: polygon to horizon", "104.4.5"))
	, ps_(ps)
	, ctio_(*mMkCtxtIOObj(EMHorizon3D))
	, zinft_(SI().depthsInFeetByDefault())
	, curhor_(0)
	, velfld_(0)
{
    setCtrlStyle( LeaveOnly );
    ctio_.ctxt.forread = true;
    const CallBack chgcb( mCB(this,uiCalcPoly2HorVol,haveChg) );
    const CallBack calccb( mCB(this,uiCalcPoly2HorVol,doCalc) );

    if ( ps_.size() < 3 )
    {
	new uiLabel( this, "Invalid polygon" );
	return;
    }

    horsel_ = new uiIOObjSel( this, ctio_, "Calculate to" );
    horsel_->selectiondone.notify( mCB(this,uiCalcPoly2HorVol,horSel) );

    upwbox_ = new uiCheckBox( this, "Upward" );
    upwbox_->setChecked( true ); upwbox_->activated.notify( chgcb );
    ignnegbox_ = new uiCheckBox( this, "Ignore negative thicknesses" );
    ignnegbox_->setChecked( true ); ignnegbox_->activated.notify( chgcb );
    ignnegbox_->attach( alignedBelow, horsel_ );
    upwbox_->attach( leftOf, ignnegbox_ );

    uiObject* attobj = ignnegbox_;
    if ( SI().zIsTime() )
    {
	const char* txt = zinft_ ? "Velocity (ft/s)" : "Velocity (m/s)";
	velfld_ = new uiGenInput( this, txt, FloatInpSpec(zinft_?10000:3000) );
	velfld_->attach( alignedBelow, ignnegbox_ );
	velfld_->valuechanged.notify( calccb );
	attobj = velfld_->attachObj();
    }

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, attobj );

    uiPushButton* calcbut = new uiPushButton( this,
	    			"&Estimate volume", calccb, true);
    calcbut->attach( alignedBelow, attobj );
    calcbut->attach( ensureBelow, sep );

    valfld_ = new uiGenInput( this, "==> Volume" );
    valfld_->attach( alignedBelow, calcbut );
    valfld_->setReadOnly( true );
}


uiCalcPoly2HorVol::~uiCalcPoly2HorVol()
{
    delete ctio_.ioobj; delete &ctio_;
    if ( curhor_ )
	curhor_->unRef();
}


void uiCalcPoly2HorVol::horSel( CallBacker* cb )
{
    if ( curhor_ )
	{ curhor_->unRef(); curhor_ = 0; }

    horsel_->commitInput();
    const IOObj* ioobj = horsel_->ioobj();
    if ( !ioobj )
	uiMSG().error( "Please provide the horizon to calculate to" );
    else
    {
	uiTaskRunner tr( this );
	EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( ioobj->key(),
							      &tr );
	mDynamicCastGet(EM::Horizon3D*,hor,emobj)
	if ( hor )
	{
	    hor->ref();
	    curhor_ = hor;
	}
    }

    haveChg( cb );
}


void uiCalcPoly2HorVol::haveChg( CallBacker* )
{
    valfld_->clear();
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiCalcPoly2HorVol::doCalc( CallBacker* )
{
    float vel = 1;
    if ( velfld_ )
    {
	vel = velfld_->getfValue();
	if ( mIsUdf(vel) || vel < 0.1 )
	    mErrRet("Please provide the velocity")
	if ( zinft_ )
	    vel *= mFromFeetFactor;
    }

    dispVal( getM3(vel) );
}


#define mPolyLoc(b) \
	Geom::Point2D<float>( (float)b.inl, (float)(b.crl) )

float uiCalcPoly2HorVol::getM3( float vel )
{
    if ( !curhor_ )
    {
	horSel( 0 );
	if ( !curhor_ )
	    return mUdf(float);
    }

    ODPolygon<float> poly;
    HorSampling hs;
    const Pick::Location& pl0( ps_[0] );
    TypeSet<Coord> pts; TypeSet<float> zvals;
    for ( int idx=0; idx<ps_.size(); idx++ )
    {
	const Pick::Location& pl( ps_[idx] );
	pts += pl.pos; zvals += pl.pos.z;
	const BinID bid( SI().transform(pl.pos) );
	poly.add( mPolyLoc(bid) );
	if ( idx )
	    hs.include( bid );
	else
	    hs.start = hs.stop = bid;
    }

    InverseDistanceGridder2D grdr;
    grdr.setPoints( pts ); grdr.setValues( zvals, false );
    float avgz = 0;
    for ( int idx=0; idx<zvals.size(); idx++ )
	avgz += zvals[idx];
    avgz /= zvals.size();

    const bool upw = upwbox_->isChecked();
    const bool useneg = !ignnegbox_->isChecked();

    const int nrsect = curhor_->nrSections();
    HorSamplingIterator iter( hs );
    BinID bid; float totth = 0;
    while ( iter.next(bid) )
    {
	if ( !poly.isInside(mPolyLoc(bid),true,1e-6) )
	    continue;

	const EM::SubID subid = bid.getSerialized();
	const Coord coord( SI().transform(bid) );

	for ( int isect=0; isect<nrsect; isect++ )
	{
	    const float horz = curhor_->getPos( isect, subid ).z;
	    if ( mIsUdf(horz) )
		continue;

	    float polyz = avgz;
	    bool useavgz = true;
	    useavgz = !grdr.setGridPoint(coord) || !grdr.init();
	    if ( useavgz )
		polyz = avgz;
	    else
	    {
		polyz = grdr.getValue();
		if ( mIsUdf(polyz) )
		    polyz = avgz;
	    }

	    const float th = upw ? polyz - horz : horz - polyz;
	    if ( useneg || th > 0 )
		{ totth += th; break; }
	}
    }

    const float cellarea = SI().inlDistance() * hs.step.inl
			 * SI().crlDistance() * hs.step.crl;
    const float v = SI().zIsTime() ? vel * .5 : 1; // TWT
    return cellarea * v * totth;
}


void uiCalcPoly2HorVol::dispVal( float m3 )
{
    static const float bblconv = 6.2898108;
    static const float ft3conv = 35.314667;

    if ( mIsUdf(m3) )
	{ valfld_->clear(); return; }

    float dispval = m3;
    if ( zinft_ ) dispval *= ft3conv;
    bool mega = false;
    if ( dispval > 1e6 )
	{ mega = true; dispval /= 1e6; }

    BufferString txt( "", dispval, mega ? "M " : " " );
    txt += zinft_ ? "ft^3" : "m^3";
    txt += " (";
    dispval *= bblconv;
    if ( zinft_ ) dispval /= ft3conv;
    if ( dispval > 1e6 )
	{ mega = true; dispval /= 1e6; }
    txt += dispval; if ( mega ) txt += "M";
    txt += " bbl)";
    valfld_->setText( txt );
}
