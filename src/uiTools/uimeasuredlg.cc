/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimeasuredlg.h"

#include "bufstring.h"
#include "draw.h"
#include "position.h"
#include "settings.h"
#include "survinfo.h"
#include "unitofmeasure.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uiconstvel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uisellinest.h"
#include "uispinbox.h"
#include "od_helpids.h"

#include <math.h>


static const char* sKeyLineStyle = "Measure LineStyle";

uiMeasureDlg::uiMeasureDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Measure Distance"),mNoDlgTitle,
		       mODHelpKey(mMeasureDlgHelpID)).modal(false))
    , ls_(*new OD::LineStyle(OD::LineStyle::Solid,3))
    , appvelfld_(0)
    , zdist2fld_(0)
    , dist2fld_(0)
    , velocity_(Vel::getGUIDefaultVelocity())
    , lineStyleChange(this)
    , clearPressed(this)
    , velocityChange(this)
    , dipUnitChange(this)
{
    setCtrlStyle( CloseOnly );
    showAlwaysOnTop();

    BufferString str;
    mSettUse(get,"dTect",sKeyLineStyle,str);
    if ( !str.isEmpty() )
	ls_.fromString( str.buf() );

    uiGroup* topgrp = new uiGroup( this, "Info fields" );
    uiString hdistlbl = uiStrings::phrJoinStrings(uiStrings::sHorizontal(),
			uiStrings::phrJoinStrings(uiStrings::sDistance(),
			SI().getUiXYUnitString()) );
    hdistfld_ = new uiGenInput( topgrp, hdistlbl, FloatInpSpec(0) );
    hdistfld_->setReadOnly( true );

    uiString zdistlbl = uiStrings::phrJoinStrings(uiStrings::sVertical(),
			uiStrings::phrJoinStrings(uiStrings::sDistance(),
			SI().getUiZUnitString()) );
    zdistfld_ = new uiGenInput( topgrp, zdistlbl, FloatInpSpec(0) );
    zdistfld_->setReadOnly( true );
    zdistfld_->attach( alignedBelow, hdistfld_ );

    uiString zintimelbl = uiStrings::phrJoinStrings(uiStrings::sVertical(),
			  uiStrings::phrJoinStrings(uiStrings::sDistance(),
			  SI().getUiXYUnitString()) );
    if ( SI().zIsTime() )
    {
	zdist2fld_ = new uiGenInput( topgrp, zintimelbl, FloatInpSpec(0) );
	zdist2fld_->attach( alignedBelow, zdistfld_ );

	zdistlbl = uiStrings::phrJoinStrings(uiStrings::sVelocity(),
		   UnitOfMeasure::surveyDefVelUnitAnnot(true,true));
	appvelfld_ = new uiGenInput(topgrp, zdistlbl, FloatInpSpec(velocity_));
	mAttachCB( appvelfld_->valuechanged, uiMeasureDlg::velocityChgd );
	appvelfld_->attach( alignedBelow, zdist2fld_ );
    }

    uiString distlbl = uiStrings::phrJoinStrings(uiStrings::sDistance(),
		       SI().getUiXYUnitString());
    distfld_ = new uiGenInput( topgrp, distlbl, FloatInpSpec(0) );
    distfld_->setReadOnly( true );
    distfld_->attach( alignedBelow, appvelfld_ ? appvelfld_ : zdistfld_ );

    if ( !SI().zIsTime() && SI().xyInFeet() != SI().zInFeet() )
    {
	uiString lbl = uiStrings::phrJoinStrings( uiStrings::sDistance(),
		uiStrings::sDistUnitString(!SI().xyInFeet(),true,true) );
	dist2fld_ = new uiGenInput( topgrp, lbl, FloatInpSpec(0) );
	dist2fld_->setReadOnly( true );
	dist2fld_->attach( alignedBelow, distfld_ );
    }

    inlcrldistfld_ = new uiGenInput( topgrp, tr("Inl/Crl Distance"),
				     FloatInpIntervalSpec(Interval<float>(0,0))
				     .setName("InlDist",0)
				     .setName("CrlDist",1) );
    inlcrldistfld_->setReadOnly( true, -1 );
    inlcrldistfld_->attach( alignedBelow, dist2fld_ ? dist2fld_ : distfld_ );

    dipfld_ = new uiGenInput( topgrp, uiStrings::sDip(), FloatInpSpec(0) );
    dipfld_->setReadOnly( true );
    dipfld_->attach( alignedBelow, inlcrldistfld_ );

    BufferStringSet unitstrs;
    unitstrs.add( "degrees" ).add( SI().zIsTime() ? "us/m" : "mm/m" );
    dipunitfld_ = new uiComboBox( topgrp, unitstrs, "Dip Units" );
    mAttachCB( dipunitfld_->selectionChanged, uiMeasureDlg::dipUnitSel );
    dipunitfld_->attach( rightOf, dipfld_ );

    uiGroup* botgrp = new uiGroup( this, "Button group" );
    uiPushButton* clearbut = new uiPushButton( botgrp, tr("Clear"),
				mCB(this,uiMeasureDlg,clearCB), true );
    uiPushButton* stylebut = new uiPushButton( botgrp, tr("Line style"),
				mCB(this,uiMeasureDlg,stylebutCB), false );
    stylebut->attach( rightTo, clearbut );

    clearchkbut_ = new uiCheckBox( botgrp, tr("Clear line on Close") );
    clearchkbut_->attach( alignedBelow, clearbut );
    clearchkbut_->setChecked( true );
    clearchkbut_->setSensitive( true );

    botgrp->attach( centeredBelow, topgrp );

    mAttachCB( postFinalize(), uiMeasureDlg::finalizeCB );
}


uiMeasureDlg::~uiMeasureDlg()
{
    detachAllNotifiers();
    delete &ls_;
}


void uiMeasureDlg::finalizeCB( CallBacker* )
{
    hdistfld_->setNrDecimals( 2 );
    zdistfld_->setNrDecimals( 2 );
    if ( zdist2fld_ )
	zdist2fld_->setNrDecimals( 2 );
    distfld_->setNrDecimals( 2 );
    if ( dist2fld_ )
	dist2fld_->setNrDecimals( 2 );
    dipfld_->setNrDecimals( 2 );
}


bool uiMeasureDlg::doClear() const
{
   return clearchkbut_->isChecked();
}


void uiMeasureDlg::lsChangeCB( CallBacker* cb )
{
    mDynamicCastGet(uiColorInput*,uicol,cb)
    mDynamicCastGet(uiSpinBox*,uisb,cb)
    if ( !uicol && !uisb ) return;

    if ( uicol )
	ls_.color_ = uicol->color();
    else if ( uisb )
	ls_.width_ = uisb->getIntValue();
    lineStyleChange.trigger( cb );
}


void uiMeasureDlg::clearCB( CallBacker* cb )
{ clearPressed.trigger( cb ); }


void uiMeasureDlg::stylebutCB( CallBacker* )
{
    uiDialog dlg( this, uiDialog::Setup(tr("Line Style"),mNoDlgTitle,
					mNoHelpKey) );
    dlg.setCtrlStyle( uiDialog::CloseOnly );
    auto* linestylefld = new uiSelLineStyle( &dlg, ls_,
				uiSelLineStyle::Setup().drawstyle(false) );
    mAttachCB( linestylefld->changed, uiMeasureDlg::lsChangeCB );
    dlg.go();

    mDetachCB( linestylefld->changed, uiMeasureDlg::lsChangeCB );
    BufferString str;
    ls_.toString( str );
    mSettUse(set,"dTect",sKeyLineStyle,str);
    Settings::common().write();
}


void uiMeasureDlg::velocityChgd( CallBacker* )
{
    if ( !appvelfld_ ) return;

    const float newvel = appvelfld_->getFValue();
    if ( mIsEqual(velocity_,newvel,mDefEps) )
	return;

    velocity_ = newvel;
    velocityChange.trigger();
}


void uiMeasureDlg::reset()
{
    hdistfld_->setValue( 0 );
    zdistfld_->setValue( 0 );
    if ( zdist2fld_ ) zdist2fld_->setValue( 0 );
    if ( appvelfld_ ) appvelfld_->setValue( velocity_ );
    distfld_->setValue( 0 );
    if ( dist2fld_ ) dist2fld_->setValue( 0 );
    distfld_->setValue( 0 );
    inlcrldistfld_->setValue( Interval<int>(0,0) );
    dipfld_->setEmpty();
    dipfld_->setSensitive( false );
}


void uiMeasureDlg::fill( const TypeSet<Coord3>& points )
{
    const double velocity = appvelfld_ ? appvelfld_->getDValue() : 0 ;
    const int size = points.size();
    if ( size<2 )
    {
	reset();
	return;
    }

    int totinldist = 0, totcrldist = 0;
    double tothdist = 0, totzdist = 0;
    double totrealdist = 0; // in xy unit
    const UnitOfMeasure* uom = UoMR().get( "Feet" );
    for ( int idx=1; idx<size; idx++ )
    {
	const Coord xy = points[idx].coord();
	const Coord prevxy = points[idx-1].coord();
	const BinID bid = SI().transform( xy );
	const BinID prevbid = SI().transform( prevxy );
	double zdist = fabs( points[idx-1].z - points[idx].z );

	totinldist += abs( bid.inl() - prevbid.inl() );
	totcrldist += abs( bid.crl() - prevbid.crl() );
	const double hdist = xy.distTo( prevxy );
	tothdist += hdist;
	totzdist += zdist;
	if ( SI().zIsTime() )
	{
	    totrealdist +=
		Math::Sqrt( hdist*hdist + velocity*velocity*zdist*zdist/4 );
	}
	else
	{
	    if ( SI().zInMeter() && SI().xyInFeet() )
		zdist = uom->getUserValueFromSI( zdist );

	    if ( !SI().zInMeter() && !SI().xyInFeet() )
		zdist = uom->getSIValue( zdist );

	    totrealdist += Math::Sqrt( hdist*hdist + zdist*zdist );
	}
    }

    double convdist = SI().xyInFeet() ? uom->getSIValue( totrealdist )
				      : uom->getUserValueFromSI( totrealdist );

    hdistfld_->setValue( tothdist );
    zdistfld_->setValue( totzdist*SI().zDomain().userFactor() );
    if ( zdist2fld_ ) zdist2fld_->setValue( totzdist*velocity/2 );
    distfld_->setValue( totrealdist );
    if ( dist2fld_ ) dist2fld_->setValue( convdist );
    inlcrldistfld_->setValue( Interval<int>(totinldist,totcrldist) );

    const bool showdip = points.size() == 2;
    if ( !showdip )
	dipfld_->setEmpty();
    else
    {
	if ( SI().xyInFeet() )
	    tothdist *= mFromFeetFactorD;
	if ( SI().zInFeet() )
	    totzdist *= mFromFeetFactorD;

	float dipval = mUdf( float );
	if ( dipunitfld_->currentItem() == 0 ) // Degrees
	{
	    if (SI().zIsTime())
		totzdist *= velocity / 2;

	    dipval = mIsZero(tothdist,1e-3) ? 90.f
			: Math::toDegrees( Math::Atan2(totzdist,tothdist) );
	}
	else
	{
	    const double zfac = SI().zIsTime() ? 1e6 : 1e3;
	    dipval = totzdist * zfac / tothdist;
	}

	dipfld_->setValue( dipval );
    }

    dipfld_->setSensitive( showdip );
    raise();
}


void uiMeasureDlg::dipUnitSel( CallBacker* )
{
    dipUnitChange.trigger();
}
