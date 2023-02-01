/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uilatlonginp.h"

#include "latlong.h"
#include "od_iostream.h"
#include "od_helpids.h"
#include "survinfo.h"

#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilatlong2coord.h"
#include "uilineedit.h"
#include "uimsg.h"
#include "uispinbox.h"
#include "uitoolbutton.h"


#define mErrRet(msg) { uiMSG().error( msg ); return false; }

class uiLatLongDMSInp : public uiGroup
{ mODTextTranslationClass(uiLatLongDMSInp);
public:
			uiLatLongDMSInp(uiParent*,bool lat);
			~uiLatLongDMSInp();

    double		value() const;
    void		set(double);
    void		setReadOnly(bool);

    Notifier<uiLatLongDMSInp> valueChanged;

protected:

    bool		islat_;

    uiSpinBox*		degfld_;
    uiSpinBox*		minfld_;
    uiLineEdit*		secfld_;
    uiComboBox*		hfld_;

    void		valChgCB(CallBacker*);
};


uiLatLongDMSInp::uiLatLongDMSInp( uiParent* p, bool lat )
    : uiGroup(p,BufferString(lat?"lat":"long"," DMS group"))
    , valueChanged(this)
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

    secfld_ = new uiLineEdit( this, BufferString("DMS ",nm," sec") );
    secfld_->setHSzPol( uiObject::Small );
    secfld_->attach( rightOf, minfld_ );
    secfld_->setValue( 0 );
    uiFloatValidator fv( 0, 59.99f );
    fv.nrdecimals_ = 4;
    secfld_->setValidator( fv );

    hfld_ = new uiComboBox( this, "Hemisphere" );
    hfld_->setHSzPol( uiObject::SmallVar );
    hfld_->attach( rightOf, secfld_ );
    hfld_->addItem( lat ? uiStrings::sNorth(true) : uiStrings::sEast(true) );
    hfld_->addItem( lat ? uiStrings::sSouth(true) : uiStrings::sWest(true) );

    mAttachCB( degfld_->valueChanged, uiLatLongDMSInp::valChgCB );
    mAttachCB( minfld_->valueChanged, uiLatLongDMSInp::valChgCB );
    mAttachCB( secfld_->editingFinished, uiLatLongDMSInp::valChgCB );
    mAttachCB( hfld_->selectionChanged, uiLatLongDMSInp::valChgCB );
}


uiLatLongDMSInp::~uiLatLongDMSInp()
{
    detachAllNotifiers();
}


void uiLatLongDMSInp::valChgCB( CallBacker* )
{
    valueChanged.trigger();
}


void uiLatLongDMSInp::setReadOnly( bool yn )
{
    degfld_->setReadOnly( yn );
    minfld_->setReadOnly( yn );
    secfld_->setReadOnly( yn );
    hfld_->setSensitive( !yn );
}


double uiLatLongDMSInp::value() const
{
    const int sign = hfld_->currentItem()==0 ? 1 : -1;
    const int d = sign * degfld_->getIntValue();
    const int m = sign * minfld_->getIntValue();
    const float s = sign * secfld_->getFValue();
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
    hfld_->setCurrentItem( issw ? 1 : 0 );
    const int sign = issw ? -1 : 1;
    degfld_->setValue( sign * d );
    minfld_->setValue( sign * m );
    secfld_->setValue( sign * s );
}


// uiLatLongInp
uiLatLongInp::uiLatLongInp( uiParent* p )
    : uiGroup(p,"Lat/Long inp group")
    , valueChanged(this)
{
    auto* bgrp = new uiButtonGroup( this, "Dec/DMS sel grp", OD::Horizontal );
    bgrp->setExclusive( true );
    isdecbut_ = new uiRadioButton( bgrp, uiStrings::sDecimal() );
    isdecbut_->setChecked( true );
    mAttachCB( isdecbut_->activated, uiLatLongInp::typSel );
    auto* isdmsbut = new uiRadioButton( bgrp, tr("DMS") );
    mAttachCB( isdmsbut->activated, uiLatLongInp::typSel );

    auto* lblgrp = new uiGroup( this, "Lat/Long Label grp" );
    auto* lnglbl = new uiLabel( lblgrp, uiStrings::sLongitude() );
    auto* latlbl = new uiLabel( lblgrp, uiStrings::sLat() );
    latlbl->attach( alignedBelow, lnglbl );
    lblgrp->setHAlignObj( lnglbl );

    auto* inpgrp = new uiGroup( this, "Lat/Long inp grp" );
    uiFloatValidator validator( -180.f, 180.f );
    validator.nrdecimals_ = 5;
    validator.scnotation_ = false;
    lngdecfld_ = new uiLineEdit( inpgrp, "Dec Longitude" );
    lngdecfld_->setValidator( validator );
    inpgrp->setHAlignObj( lngdecfld_ );

    latdecfld_ = new uiLineEdit( inpgrp, "Dec Latitude" );
    validator.bottom_ = -90.f;
    validator.top_ = 90.f;
    latdecfld_->setValidator( validator );
    latdecfld_->attach( alignedBelow, lngdecfld_ );

    lngdmsfld_ = new uiLatLongDMSInp( inpgrp, false );
    latdmsfld_ = new uiLatLongDMSInp( inpgrp, true );
    latdmsfld_->attach( alignedBelow, lngdmsfld_ );

    mAttachCB( lngdecfld_->editingFinished, uiLatLongInp::llchgCB );
    mAttachCB( latdecfld_->editingFinished, uiLatLongInp::llchgCB );
    mAttachCB( lngdmsfld_->valueChanged, uiLatLongInp::llchgCB );
    mAttachCB( latdmsfld_->valueChanged, uiLatLongInp::llchgCB );

    lblgrp->attach( leftTo, inpgrp );
    inpgrp->attach( alignedBelow, bgrp );
    setHAlignObj( inpgrp );
    mAttachCB( postFinalize(), uiLatLongInp::typSel );
}


uiLatLongInp::~uiLatLongInp()
{
    detachAllNotifiers();
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


void uiLatLongInp::llchgCB( CallBacker* )
{
    valueChanged.trigger();
}


void uiLatLongInp::get( LatLong& ll ) const
{
    return get( ll, isdecbut_->isChecked() );
}


void uiLatLongInp::get( LatLong& ll, bool isdec ) const
{
    if ( isdec )
    {
	ll.lng_ = lngdecfld_->getDValue();
	ll.lat_ = latdecfld_->getDValue();
	return;
    }

    ll.lat_ = latdmsfld_->value();
    ll.lng_ = lngdmsfld_->value();
}


void uiLatLongInp::set( const LatLong& ll )
{
    set( ll, 0 );
}


void uiLatLongInp::set( const LatLong& ll, int opt )
{
    if ( ll.isUdf() )
    {
	latdecfld_->setValue( 0 );
	lngdecfld_->setValue( 0 );
	latdmsfld_->set( 0 );
	lngdmsfld_->set( 0 );
	return;
    }

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


void uiLatLongInp::setReadOnly( bool yn )
{
    lngdecfld_->setReadOnly( yn );
    latdecfld_->setReadOnly( yn );
    lngdmsfld_->setReadOnly( yn );
    latdmsfld_->setReadOnly( yn );
}


namespace Coords
{

uiUnlocatedXYSystem::uiUnlocatedXYSystem( uiParent* p )
    : uiCoordSystem(p,sFactoryDisplayName())
{
    xyunitfld_ = new uiGenInput( this, tr("Coordinates are in"),
	    BoolInpSpec(true,uiStrings::sMeter(),uiStrings::sFeet()) );
    setHAlignObj( xyunitfld_ );
}


uiUnlocatedXYSystem::~uiUnlocatedXYSystem()
{}


bool uiUnlocatedXYSystem::initFields( const Coords::CoordSystem* sys )
{
    mDynamicCastGet( const Coords::UnlocatedXY*, from,	sys );
    if ( !from )
	return false;

    xyunitfld_->setValue( !from->isFeet() );
    return true;
}


bool uiUnlocatedXYSystem::acceptOK()
{
    RefMan<UnlocatedXY> res = new UnlocatedXY;
    res->setIsFeet( !xyunitfld_->getBoolValue() );
    outputsystem_ = res;
    return true;
}


uiAnchorBasedXYSystem::uiAnchorBasedXYSystem( uiParent* p )
    : uiCoordSystem(p,sFactoryDisplayName())
{
    helpkey_ = mODHelpKey(mLatLong2CoordDlgHelpID);

    coordfld_ = new uiGenInput( this, tr("Coordinate in or near survey"),
				DoubleInpSpec(), DoubleInpSpec() );
    coordfld_->setElemSzPol( uiObject::SmallVar );

    latlngfld_ = new uiLatLongInp( this );
    latlngfld_->attach( alignedBelow, coordfld_ );
    new uiLabel( this, tr("Corresponds to"), latlngfld_ );

    xyunitfld_ = new uiGenInput( this, uiStrings::sEmptyString(),
	    BoolInpSpec(true,uiStrings::sMeter(),uiStrings::sFeet()) );
    xyunitfld_->attach( rightOf, coordfld_ );

    setHAlignObj( coordfld_ );
}


uiAnchorBasedXYSystem::~uiAnchorBasedXYSystem()
{}


bool uiAnchorBasedXYSystem::initFields( const Coords::CoordSystem* sys )
{
    mDynamicCastGet( const Coords::AnchorBasedXY*, from, sys );
    if ( !from || !from->geographicTransformOK() )
	return false;

    coordfld_->setValue( from->refCoord() );
    latlngfld_->set( from->refLatLong() );

    xyunitfld_->setValue( !from->isFeet() );
    return true;
}


bool uiAnchorBasedXYSystem::acceptOK()
{
    LatLong ll;
    latlngfld_->get( ll );
    const Coord crd = coordfld_->getCoord();
    if ( mIsUdf(ll.lat_) || mIsUdf(ll.lng_) ||
	 mIsUdf(crd.x) || mIsUdf(crd.y) )
	mErrRet(tr("Please fill all fields"))
    if (ll.lat_ > 90 || ll.lat_ < -90)
	mErrRet(tr("Latitude must be between -90 and 90"))
    if (ll.lng_ > 180 || ll.lng_ < -180)
	mErrRet(tr("Longitude must be between -180 and 180"))
    if ( !si_->isReasonable(crd) )
    {
	if ( !uiMSG().askContinue(
	    tr("The coordinate seems to be far away from the survey."
	       "\nContinue?")))
	    return false;
    }

    RefMan<AnchorBasedXY> res = new AnchorBasedXY( ll, crd );
    if ( !res->geographicTransformOK() )
    {
	uiMSG().error(tr("Sorry, your Lat/Long definition has a problem"));
	return false;
    }

    res->setIsFeet( !xyunitfld_->getBoolValue() );

    outputsystem_ = res;

    return true;
}

} // namespace Coords
