/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uilatlong2coord.h"
#include "uilatlonginp.h"
#include "latlong.h"
#include "survinfo.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uilineedit.h"
#include "uispinbox.h"
#include "uimsg.h"


class uiLatLongDMSInp : public uiGroup
{
public:
    			uiLatLongDMSInp(uiParent*,bool lat);

    double		value() const;
    void		set(double);

protected:

    bool	islat_;

    uiSpinBox*	degfld_;
    uiSpinBox*	minfld_;
    uiLineEdit*	secfld_;
    uiCheckBox*	swfld_;

};


uiLatLongDMSInp::uiLatLongDMSInp( uiParent* p, bool lat )
    : uiGroup(p,BufferString(lat?"lat":"long"," DMS group"))
    , islat_(lat)
{
    const char* nm = islat_ ? "Latitude" : "Longitude";
    degfld_ = new uiSpinBox( this, 0, BufferString("DMS ",nm," deg") );
    if ( islat_ )
	degfld_->setInterval( 0, 90, 1 );
    else
	degfld_->setInterval( 0, 180, 1 );
    degfld_->setValue( 0 );
    minfld_ = new uiSpinBox( this, 0, BufferString("DMS ",nm," min") );
    minfld_->setInterval( 0, 59, 1 );
    minfld_->setValue( 0 );
    minfld_->attach( rightOf, degfld_ );
    secfld_ = new uiLineEdit( this, FloatInpSpec(),
	    		      BufferString("DMS ",nm," sec") );
    secfld_->setHSzPol( uiObject::Small );
    secfld_->attach( rightOf, minfld_ );
    secfld_->setValue( 0 );

    swfld_ = new uiCheckBox( this, islat_ ? "S" : "W" );
    swfld_->attach( rightOf, secfld_ );
    swfld_->setHSzPol( uiObject::Small );
}


double uiLatLongDMSInp::value() const
{
    int sign = swfld_->isChecked() ? -1 : 1;
    const int d = sign * degfld_->getValue();
    const int m = sign * minfld_->getValue();
    const float s = sign * secfld_->getfValue();
    LatLong ll; ll.setDMS( islat_, d, m, s );
    return islat_ ? ll.lat_ : ll.lng_;
}


void uiLatLongDMSInp::set( double val )
{
    LatLong ll;
    (islat_ ? ll.lat_ : ll.lng_) = val;
    int d, m; float s; 
    ll.getDMS( islat_, d, m, s );
    const bool issw = val < 0;
    swfld_->setChecked( issw );
    const int sign = issw ? -1 : 1;
    degfld_->setValue( sign * d );
    minfld_->setValue( sign * m );
    secfld_->setValue( sign * s );
}


uiLatLongInp::uiLatLongInp( uiParent* p )
    : uiGroup(p,"Lat/Long inp group")
{
    const CallBack tscb( mCB(this,uiLatLongInp,typSel) );
    uiButtonGroup* bgrp = new uiButtonGroup( this, "Dec/DMS sel grp" );
    bgrp->setExclusive( true );
    isdecbut_ = new uiRadioButton( bgrp, "Decimal" );
    isdecbut_->setChecked( true );
    isdecbut_->activated.notify( tscb );
    uiRadioButton* isdmsbut = new uiRadioButton( bgrp, "DMS" );
    isdmsbut->activated.notify( tscb );

    uiGroup* lblgrp = new uiGroup( this, "Lat/Long Label grp" );
    uiLabel* latlbl = new uiLabel( lblgrp, "Latitude" );
    uiLabel* lnglbl = new uiLabel( lblgrp, "Longitude" );
    lnglbl->attach( alignedBelow, latlbl );
    lblgrp->setHAlignObj( latlbl );

    uiGroup* inpgrp = new uiGroup( this, "Lat/Long inp grp" );
    latdecfld_ = new uiLineEdit( inpgrp, DoubleInpSpec(0), "Dec Latitude" );
    lngdecfld_ = new uiLineEdit( inpgrp, DoubleInpSpec(0), "Dec Longitude" );
    lngdecfld_->attach( alignedBelow, latdecfld_ );
    latdmsfld_ = new uiLatLongDMSInp( inpgrp, true );
    lngdmsfld_ = new uiLatLongDMSInp( inpgrp, false );
    lngdmsfld_->attach( alignedBelow, latdmsfld_ );
    inpgrp->setHAlignObj( latdecfld_ );

    lblgrp->attach( rightOf, bgrp );
    inpgrp->attach( rightOf, lblgrp );
    setHAlignObj( inpgrp );
    postFinalise().notify( tscb );
}


void uiLatLongInp::typSel( CallBacker* )
{
    const bool isdec = isdecbut_->isChecked();
    LatLong oth; get( oth, !isdec );
    set( oth, isdec ? 1 : -1 );
    latdecfld_->display( isdec );
    lngdecfld_->display( isdec );
    latdmsfld_->display( !isdec );
    lngdmsfld_->display( !isdec );
}


void uiLatLongInp::get( LatLong& ll ) const
{
    return get( ll, isdecbut_->isChecked() );
}


void uiLatLongInp::get( LatLong& ll, bool isdec ) const
{
    if ( isdec )
    {
	ll.lat_ = latdecfld_->getdValue();
	ll.lng_ = lngdecfld_->getdValue();
    }
    else
    {
	ll.lat_ = latdmsfld_->value();
	ll.lng_ = lngdmsfld_->value();
    }
}


void uiLatLongInp::set( const LatLong& ll )
{
    set( ll, 0 );
}


void uiLatLongInp::set( const LatLong& ll, int opt )
{
    if ( opt >= 0 )
    {
	latdecfld_->setValue( ll.lat_ );
	lngdecfld_->setValue( ll.lng_ );
    }
    if ( opt <= 0 )
    {
	latdmsfld_->set( ll.lat_ );
	lngdmsfld_->set( ll.lng_ );
    }
}


uiLatLong2CoordDlg::uiLatLong2CoordDlg( uiParent* p, const LatLong2Coord& l,
       					const SurveyInfo* si )
    : uiDialog(p,uiDialog::Setup("Lat/Long vs Coordinates",
	     "Estimation of geographical coordinates from/to "
	     "the rectangular survey coordinates",
				 "0.3.9"))
    , ll2c_(*new LatLong2Coord(l))
    , si_(si?si:&SI())
{
    coordfld_ = new uiGenInput( this, "Coordinate in or near survey",
	    			DoubleInpSpec(), DoubleInpSpec() );

    latlngfld_ = new uiLatLongInp( this );
    latlngfld_->attach( alignedBelow, coordfld_ );
    new uiLabel( this, "Corresponds to", latlngfld_ );

    if ( ll2c_.isOK() )
    {
	coordfld_->setValue( ll2c_.refCoord() );
	latlngfld_->set( ll2c_.refLatLong() );
    }
}


uiLatLong2CoordDlg::~uiLatLong2CoordDlg()
{
    delete &ll2c_;
}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiLatLong2CoordDlg::acceptOK( CallBacker* )
{
    LatLong ll; latlngfld_->get( ll );
    const Coord crd = coordfld_->getCoord();
    if ( mIsUdf(ll.lat_) || mIsUdf(ll.lng_) || mIsUdf(crd.x) || mIsUdf(crd.y) )
	mErrRet("Please fill all fields")
    if ( ll.lat_ > 90 || ll.lat_ < -90 )
	mErrRet("Latitude must be between -90 and 90")
    if ( ll.lng_ > 180 || ll.lng_ < -180 )
	mErrRet("Longitude must be between -180 and 180")
    if ( !si_->isReasonable(crd) )
    {
	if ( !uiMSG().askContinue(
		    "The coordinate seems to be far away from the survey."
		    "\nContinue?") )
	    return false;
    }

    ll2c_.set( ll, crd );
    if ( !ll2c_.isOK() )
    {
	uiMSG().error( "Sorry, your Lat/Long definition has a problem" );
	return false;
    }

    si_->getLatlong2Coord() = ll2c_;
    if ( !si_->write() )
    {
	uiMSG().error( "Could not write the definitions to your '.survey' file"
		    "\nThe definition will work this OpendTect session only" );
	return false;
    }

    return true;
}


bool uiLatLong2CoordDlg::ensureLatLongDefined( uiParent* p, SurveyInfo* si )
{
    if ( !si ) si = const_cast<SurveyInfo*>( &SI() );
    if ( si->latlong2Coord().isOK() ) return true;

    uiLatLong2CoordDlg dlg( p, si->latlong2Coord(), si );
    return dlg.go();
}
