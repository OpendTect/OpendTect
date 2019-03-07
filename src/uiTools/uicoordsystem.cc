/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/

#include "uicoordsystem.h"

#include "latlong.h"
#include "manobjectset.h"
#include "survinfo.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilatlonginp.h"
#include "uitoolbutton.h"
#include "uibuttongroup.h"
#include "uilineedit.h"
#include "uispinbox.h"
#include "uimsg.h"
#include "od_iostream.h"
#include "od_helpids.h"

using namespace Coords;

mImplClassFactory( uiCoordSystem, factory );


uiCoordSystem::uiCoordSystem( uiParent* p, const uiString& caption )
    : uiDlgGroup( p, caption )
    , si_( 0 )
{}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }


class uiLatLongDMSInp : public uiGroup
{ mODTextTranslationClass(uiLatLongDMSInp);
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
    secfld_ = new uiLineEdit( this, BufferString("DMS ",nm," sec") );
    uiFloatValidator fv( 0, 59.99f ); fv.nrdecimals_ = 2;
    secfld_->setValidator( fv );
    secfld_->setHSzPol( uiObject::Small );
    secfld_->attach( rightOf, minfld_ );
    secfld_->setValue( 0 );

    swfld_ = new uiCheckBox( this, islat_ ? uiStrings::sSouth(true) :
					    uiStrings::sWest(true) );
    swfld_->attach( rightOf, secfld_ );
    swfld_->setHSzPol( uiObject::Small );
}


double uiLatLongDMSInp::value() const
{
    int sign = swfld_->isChecked() ? -1 : 1;
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
    uiButtonGroup* bgrp = new uiButtonGroup( this, "Dec/DMS sel grp",
					     OD::Horizontal );
    bgrp->setExclusive( true );
    isdecbut_ = new uiRadioButton( bgrp, uiStrings::sDecimal() );
    isdecbut_->setChecked( true );
    isdecbut_->activated.notify( tscb );
    uiRadioButton* isdmsbut = new uiRadioButton( bgrp, tr("DMS") );
    isdmsbut->activated.notify( tscb );

    uiGroup* lblgrp = new uiGroup( this, "Lat/Long Label grp" );
    uiLabel* lnglbl = new uiLabel( lblgrp, uiStrings::sLongitude(false) );
    uiLabel* latlbl = new uiLabel( lblgrp, uiStrings::sLatitude(false) );
    latlbl->attach( alignedBelow, lnglbl );
    lblgrp->setHAlignObj( lnglbl );

    uiGroup* inpgrp = new uiGroup( this, "Lat/Long inp grp" );
    lngdecfld_ = new uiLineEdit( inpgrp, DoubleInpSpec(0), "Dec Longitude" );
    latdecfld_ = new uiLineEdit( inpgrp, DoubleInpSpec(0), "Dec Latitude" );
    latdecfld_->attach( alignedBelow, lngdecfld_ );
    lngdmsfld_ = new uiLatLongDMSInp( inpgrp, false );
    latdmsfld_ = new uiLatLongDMSInp( inpgrp, true );
    latdmsfld_->attach( alignedBelow, lngdmsfld_ );
    inpgrp->setHAlignObj( lngdecfld_ );

    inpgrp->attach( alignedBelow, bgrp );
    lblgrp->attach( leftOf, inpgrp );
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
	ll.lat_ = latdecfld_->getDValue();
	ll.lng_ = lngdecfld_->getDValue();
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


uiUnlocatedXYSystem::uiUnlocatedXYSystem( uiParent* p )
    : uiCoordSystem( p,sFactoryDisplayName() )
{
    xyunitfld_ = new uiGenInput( this, tr("Coordinates are in"),
	BoolInpSpec(true,uiStrings::sMeter(false),uiStrings::sFeet(false)) );
    setHAlignObj( xyunitfld_ );
}


bool uiUnlocatedXYSystem::initFields( const Coords::CoordSystem* sys )
{
    mDynamicCastGet(const Coords::UnlocatedXY*,from,sys);
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
    : uiCoordSystem( p,sFactoryDisplayName() )
{
    helpkey_ = mTODOHelpKey;

    coordfld_ = new uiGenInput( this, tr("Coordinate in or near survey"),
				DoubleInpSpec(), DoubleInpSpec() );

    xyunitfld_ = new uiGenInput( this, uiString::empty(),
	BoolInpSpec(true,uiStrings::sMeter(false),uiStrings::sFeet(false)) );
    xyunitfld_->attach( rightOf, coordfld_ );

    uiLabel* dummylbl = new uiLabel( this, uiString::empty() );
    dummylbl->attach( alignedBelow, coordfld_ );
    dummylbl->display( false );
    new uiLabel( this, tr("Corresponds to"), dummylbl );

    latlngfld_ = new uiLatLongInp( this );
    latlngfld_->attach( alignedBelow, coordfld_ );

    setHAlignObj( coordfld_ );
}


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
	 mIsUdf(crd.x_) || mIsUdf(crd.y_) )
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
uiCoordSystemSelGrp::uiCoordSystemSelGrp( uiParent* p,
					      bool onlyorthogonal,
					      bool projectiononly,
					      const SurveyInfo* si,
					      const CoordSystem* fillfrom )
    : uiDlgGroup( p, tr("Coordinate system properties") )
    , si_(si ? si : &SI())
{
    uiStringSet names;
    CoordSystem::getSystemNames( onlyorthogonal, projectiononly,
				    names, coordsystempars_ );

    for ( int idx=0; idx<coordsystempars_.size(); idx++ )
    {
	BufferString key;
	uiCoordSystem* systemui = 0;
	if ( coordsystempars_[idx]->get( CoordSystem::sKeyFactoryKey(), key ) )
	    systemui = uiCoordSystem::factory().create( key, this );

	if ( !systemui )
	{
	    coordsystempars_.removeSingle( idx );
	    names.removeSingle( idx );
	    idx--;
	    continue;
	}


	coordsystemsuis_ += systemui;
	systemui->setSurveyInfo( si_ );
	if ( fillfrom && key==systemui->factoryKeyword() )
	    systemui->initFields( fillfrom );
	systemui->display( false );
    }

    if ( names.isEmpty() )
	coordsystemsel_ = 0;
    else
    {
	coordsystemsel_ = new uiGenInput( this, uiStrings::sCoordSys(),
				      StringListInpSpec(names) );
	coordsystemsel_->attach( leftBorder );
	mAttachCB( coordsystemsel_->valuechanged,
	       uiCoordSystemSelGrp::systemChangedCB);
    }

    if ( coordsystemsel_ )
    {
	const BufferString selname = fillfrom ? fillfrom->factoryKeyword() : "";
	for ( int idx=0; idx<coordsystemsuis_.size(); idx++ )
	{
	    uiCoordSystem& uics  = *coordsystemsuis_[idx];
	    uics.attach( alignedBelow, coordsystemsel_ );
	    if ( selname == uics.factoryKeyword() )
		coordsystemsel_->setValue( idx );
	}
    }

    systemChangedCB( 0 );
}


uiCoordSystemSelGrp::~uiCoordSystemSelGrp()
{

}

void uiCoordSystemSelGrp::systemChangedCB(CallBacker *)
{
    const int selidx = coordsystemsel_ ? coordsystemsel_->getIntValue() : 0;

    for ( int idx=0; idx<coordsystemsuis_.size(); idx++ )
	coordsystemsuis_[idx]->display( idx==selidx );
}


bool uiCoordSystemSelGrp::acceptOK()
{
    outputsystem_ = 0;

    const int selidx = coordsystemsel_ ? coordsystemsel_->getIntValue() : 0;

    if ( !coordsystemsuis_[selidx]->acceptOK() )
	return false;

    outputsystem_ = coordsystemsuis_[selidx]->outputSystem();
    return outputsystem_;
}


uiCoordSystemDlg::uiCoordSystemDlg( uiParent* p, bool orthogonalonly,
			bool projectiononly, const CoordSystem* coordsys )
    : uiDialog(p,uiDialog::Setup(tr("Coordinate Reference System"),mNoDlgTitle,
				 mTODOHelpKey ))
{
    coordsysselfld_ = new Coords::uiCoordSystemSelGrp( this, orthogonalonly,
						projectiononly, 0, coordsys );
}


uiCoordSystemDlg::~uiCoordSystemDlg()
{
}


RefMan<CoordSystem> uiCoordSystemDlg::getCoordSystem()
{
    if ( !coordsysselfld_->outputSystem() )
	coordsysselfld_->acceptOK();

    return coordsysselfld_->outputSystem();
}


bool uiCoordSystemDlg::acceptOK()
{
    return coordsysselfld_->acceptOK();
}


bool uiCoordSystemDlg::ensureGeographicTransformOK( uiParent* p,
						    SurveyInfo* si )
{
    if ( !si ) si = const_cast<SurveyInfo*>( &SI() );
    if ( si->getCoordSystem() && si->getCoordSystem()->geographicTransformOK() )
	return true;

    uiCoordSystemDlg dlg( p, true, false, si->getCoordSystem() );
    if ( !dlg.go() || !dlg.getCoordSystem()
	    || !dlg.getCoordSystem()->geographicTransformOK() )
	return false;

    si->setCoordSystem( dlg.getCoordSystem() );
    return true;
}


uiCoordSystemSel::uiCoordSystemSel( uiParent* p, bool orthogonalonly,
			    bool projectiononly, const CoordSystem* coordsys,
			    const uiString& seltxt )
    : uiCompoundParSel(p,seltxt)
    , orthogonalonly_(orthogonalonly), projectiononly_(projectiononly)
    , dlg_(0)
{
    if ( coordsys )
	coordsystem_ = coordsys->clone();

    txtfld_->setElemSzPol( uiObject::Wide );
    butPush.notify( mCB(this,uiCoordSystemSel,selCB) );
}


void uiCoordSystemSel::selCB( CallBacker* )
{
    if ( !dlg_ )
	dlg_ = new uiCoordSystemDlg( this, orthogonalonly_, projectiononly_,
					coordsystem_ );

    if ( dlg_->go() )
    {
	coordsystem_ = dlg_->getCoordSystem();
	updateSummary();
    }
}


uiString uiCoordSystemSel::getSummary() const
{
    if ( !coordsystem_ )
	return uiString::empty();

    return coordsystem_->summary();
}

