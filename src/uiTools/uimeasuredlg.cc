/*+
________________________________________________________________________

    (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
    Author: Nageswara
    Date:   May 2008
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
#include "uiconstvel.h"
#include "uilabel.h"
#include "uisellinest.h"
#include "uispinbox.h"
#include "od_helpids.h"

#include <math.h>


static const char* sKeyLineStyle = "Measure LineStyle";

uiMeasureDlg::uiMeasureDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Measure Distance"),mNoDlgTitle,
	mODHelpKey(mMeasureDlgHelpID) ).modal(false))
    , ls_(*new OD::LineStyle(OD::LineStyle::Solid,3))
    , appvelfld_(0)
    , zdist2fld_(0)
    , dist2fld_(0)
    , velocity_(Vel::getGUIDefaultVelocity())
    , lineStyleChange(this)
    , clearPressed(this)
    , velocityChange(this)
{
    setCtrlStyle( CloseOnly );
    showAlwaysOnTop();

    BufferString str;
    mSettUse(get,"dTect",sKeyLineStyle,str);
    if ( !str.isEmpty() )
	ls_.fromString( str.buf() );

    uiGroup* topgrp = new uiGroup( this, "Info fields" );
    uiString hdistlbl = tr("Horizontal Distance").withSurvXYUnit();
    hdistfld_ = new uiGenInput( topgrp, hdistlbl, FloatInpSpec(0) );
    hdistfld_->setReadOnly( true );

    uiString vertdiststr = tr("Vertical Distance");

    uiString zdistlbl = vertdiststr.withSurvZUnit();
    zdistfld_ = new uiGenInput( topgrp, zdistlbl, FloatInpSpec(0) );
    zdistfld_->setReadOnly( true );
    zdistfld_->attach( alignedBelow, hdistfld_ );

    const uiString zintimelbl = vertdiststr.withSurvXYUnit();
    if ( SI().zIsTime() )
    {
	zdist2fld_ = new uiGenInput( topgrp, zintimelbl, FloatInpSpec(0) );
	zdist2fld_->attach( alignedBelow, zdistfld_ );

	appvelfld_ = new uiConstantVel( topgrp );
	velocity_ = appvelfld_->getInternalVelocity();
	appvelfld_->valuechanged.notify( mCB(this,uiMeasureDlg,velocityChgd) );
	appvelfld_->attach( alignedBelow, zdist2fld_ );
    }

    uiString distlbl = uiStrings::sDistance().withSurvXYUnit();
    distfld_ = new uiGenInput( topgrp, distlbl, FloatInpSpec(0) );
    distfld_->setReadOnly( true );
    distfld_->attach( alignedBelow, appvelfld_ ? appvelfld_ : zdistfld_ );

    if ( !SI().zIsTime() && SI().xyInFeet() != SI().zInFeet() )
    {
	uiString lbl = uiStrings::sDistance()
		.withUnit(uiStrings::sDistUnitString(!SI().xyInFeet(),true));
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

    uiGroup* botgrp = new uiGroup( this, "Button group" );
    uiPushButton* clearbut = new uiPushButton( botgrp, uiStrings::sClear(),
				mCB(this,uiMeasureDlg,clearCB), true );
    uiPushButton* stylebut = new uiPushButton( botgrp, uiStrings::sLineStyle(),
				mCB(this,uiMeasureDlg,stylebutCB), false );
    stylebut->attach( rightTo, clearbut );

    clearchkbut_ = new uiCheckBox( botgrp, tr("Clear line on Close") );
    clearchkbut_->attach( alignedBelow, clearbut );
    clearchkbut_->setChecked( true );
    clearchkbut_->setSensitive( true );

    botgrp->attach( centeredBelow, topgrp );
}


uiMeasureDlg::~uiMeasureDlg()
{
    delete &ls_;
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
    uiSelLineStyle* linestylefld = new uiSelLineStyle( &dlg, ls_,
				   uiSelLineStyle::Setup().drawstyle(false) );
    linestylefld->changed.notify( mCB(this,uiMeasureDlg,lsChangeCB) );
    dlg.go();

    BufferString str;
    ls_.toString( str );
    mSettUse(set,"dTect",sKeyLineStyle,str);
    Settings::common().write();
}


void uiMeasureDlg::velocityChgd( CallBacker* )
{
    if ( !appvelfld_ ) return;

    const float newvel = appvelfld_->getInternalVelocity();
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
    if ( appvelfld_ ) appvelfld_->setInternalVelocity( velocity_ );
    distfld_->setValue( 0 );
    if ( dist2fld_ ) dist2fld_->setValue( 0 );
    distfld_->setValue( 0 );
    inlcrldistfld_->setValue( Interval<int>(0,0) );
}


void uiMeasureDlg::fill( const TypeSet<Coord3>& points )
{
    const float velocity = appvelfld_ ? appvelfld_->getInternalVelocity() : 0 ;
    const int size = points.size();
    if ( size<2 )
	{ reset(); return; }

    int totinldist = 0, totcrldist = 0;
    double tothdist = 0, totzdist = 0;
    double totrealdist = 0; // in xy unit
    const UnitOfMeasure* ftuom = UnitOfMeasure::feetUnit();
    for ( int idx=1; idx<size; idx++ )
    {
	const Coord xy = points[idx].getXY();
	const Coord prevxy = points[idx-1].getXY();
	const BinID bid = SI().transform( xy );
	const BinID prevbid = SI().transform( prevxy );
	double zdist = fabs( points[idx-1].z_ - points[idx].z_ );

	totinldist += abs( bid.inl() - prevbid.inl() );
	totcrldist += abs( bid.crl() - prevbid.crl() );
	const double hdist = xy.distTo<double>( prevxy );
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
		zdist = ftuom->getUserValueFromSI( zdist );

	    if ( !SI().zInMeter() && !SI().xyInFeet() )
		zdist = ftuom->getSIValue( zdist );

	    totrealdist += Math::Sqrt( hdist*hdist + zdist*zdist );
	}
    }

    double convdist = SI().xyInFeet() ? ftuom->getSIValue( totrealdist )
				      : ftuom->getUserValueFromSI( totrealdist);

    hdistfld_->setValue( tothdist );
    zdistfld_->setValue( totzdist*SI().zDomain().userFactor() );
    if ( zdist2fld_ ) zdist2fld_->setValue( totzdist*velocity/2 );
    distfld_->setValue( totrealdist );
    if ( dist2fld_ ) dist2fld_->setValue( convdist );
    inlcrldistfld_->setValue( Interval<int>(totinldist,totcrldist) );
    raise();
}
