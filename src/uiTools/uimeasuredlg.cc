/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimeasuredlg.h"

#include "bufstring.h"
#include "draw.h"
#include "hiddenparam.h"
#include "settings.h"
#include "survinfo.h"
#include "unitofmeasure.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uiconstvel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uisellinest.h"
#include "uispinbox.h"
#include "od_helpids.h"

#include <math.h>

static HiddenParam<uiMeasureDlg,uiGenInput*> hp_unitflds( nullptr );

static const char* sKeyLineStyle = "Measure LineStyle";

uiMeasureDlg::uiMeasureDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Measure Distance"),mODHelpKey(mMeasureDlgHelpID))
		    .modal(false))
    , lineStyleChange(this)
    , clearPressed(this)
    , velocityChange(this)
    , dipUnitChange(this)
    , velocity_(Vel::getGUIDefaultVelocity())
    , ls_(*new OD::LineStyle(OD::LineStyle::Solid,3))
    , zdist2fld_(0)
    , appvelfld_(0)
    , dist2fld_(0) // not used anymore
{
    setCtrlStyle( CloseOnly );
    showAlwaysOnTop();

    BufferString str;
    mSettUse(get,"dTect",sKeyLineStyle,str);
    if ( !str.isEmpty() )
	ls_.fromString( str.buf() );

    auto* topgrp = new uiGroup( this, "Info fields" );
    auto* unitfld = new uiGenInput( topgrp, tr("Show distances in"),
	BoolInpSpec(!SI().xyInFeet(),uiStrings::sMeter(),uiStrings::sFeet()) );
    mAttachCB( unitfld->valueChanged, uiMeasureDlg::unitSelCB );
    hp_unitflds.setParam( this, unitfld );

    uiString hdistlbl = uiStrings::phrJoinStrings( uiStrings::sHorizontal(),
						   uiStrings::sDistance() );
    hdistfld_ = new uiGenInput( topgrp, hdistlbl, FloatInpSpec(0) );
    hdistfld_->attach( alignedBelow, unitfld );
    hdistfld_->setReadOnly( true );
    uiObject* attachobj = hdistfld_->attachObj();

    if ( SI().zIsTime() )
    {
	uiString zdisttimelbl = uiStrings::phrJoinStrings(uiStrings::sTime(),
			uiStrings::phrJoinStrings(uiStrings::sDistance(),
			SI().getUiZUnitString()) );
	auto* zdisttimefld = new uiGenInput( topgrp, zdisttimelbl,
					     FloatInpSpec(0) );
	zdisttimefld->setReadOnly( true );
	zdisttimefld->attach( alignedBelow, hdistfld_ );
	zdist2fld_ = zdisttimefld;

	uiString vellbl = uiStrings::phrJoinStrings( uiStrings::sVelocity(),
		   UnitOfMeasure::surveyDefVelUnitAnnot(true,true) );
	appvelfld_ = new uiGenInput( topgrp, vellbl, FloatInpSpec(velocity_) );
	mAttachCB( appvelfld_->valueChanged, uiMeasureDlg::velocityChgd );
	appvelfld_->attach( alignedBelow, zdisttimefld );
	attachobj = appvelfld_->attachObj();
    }

    uiString zdistlbl = uiStrings::phrJoinStrings(uiStrings::sVertical(),
			  uiStrings::sDistance() );
    zdistfld_ = new uiGenInput( topgrp, zdistlbl, FloatInpSpec(0) );
    zdistfld_->setReadOnly( true );
    zdistfld_->attach( alignedBelow, attachobj );

    auto* totaldistfld = new uiGenInput( topgrp, tr("Total Distance"),
				    FloatInpSpec(0) );
    totaldistfld->setReadOnly( true );
    totaldistfld->attach( alignedBelow, zdistfld_ );
    distfld_ = totaldistfld;

    inlcrldistfld_ = new uiGenInput( topgrp, tr("Inl/Crl Distance"),
				     FloatInpIntervalSpec(Interval<float>(0,0))
				     .setName("InlDist",0)
				     .setName("CrlDist",1) );
    inlcrldistfld_->setReadOnly( true, -1 );
    inlcrldistfld_->attach( alignedBelow, totaldistfld );

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
    hp_unitflds.removeParam( this );
}


uiGenInput* uiMeasureDlg::zDistTimeField()
{
    return zdist2fld_;
}


uiGenInput* uiMeasureDlg::totalDistField()
{
    return distfld_;
}


void uiMeasureDlg::finalizeCB( CallBacker* )
{
    hdistfld_->setNrDecimals( 2 );
    zdistfld_->setNrDecimals( 2 );
    totalDistField()->setNrDecimals( 2 );
    if ( zDistTimeField() )
	zDistTimeField()->setNrDecimals( 2 );
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
    uiDialog dlg( this, Setup(tr("Line Style"),mNoHelpKey) );
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
    if ( !appvelfld_ )
	return;

    const float newvel = appvelfld_->getFValue();
    if ( mIsEqual(velocity_,newvel,mDefEps) )
	return;

    if ( newvel<=10 || newvel>1e6 )
    {
	uiMSG().message( tr("Please enter a valid velocity value") );
	return;
    }

    velocity_ = newvel;
    velocityChange.trigger();
}


void uiMeasureDlg::reset()
{
    hdistfld_->setValue( 0 );
    zdistfld_->setValue( 0 );
    if ( zDistTimeField() )
	zDistTimeField()->setValue( 0 );

    if ( appvelfld_ )
	appvelfld_->setValue( velocity_ );

    totalDistField()->setValue( 0 );
    inlcrldistfld_->setValue( Interval<int>(0,0) );
    dipfld_->setEmpty();
    dipfld_->setSensitive( false );
}


void uiMeasureDlg::fill( const TypeSet<Coord3>& points )
{
    const int size = points.size();
    if ( size<2 )
    {
	reset();
	return;
    }

    // All values will be converted to SI here
    const UnitOfMeasure* xyuom = UnitOfMeasure::surveyDefXYUnit();
    const UnitOfMeasure* zuom = UnitOfMeasure::surveyDefZStorageUnit();
    const UnitOfMeasure* veluom = UnitOfMeasure::surveyDefVelUnit();

    int totalinldist = 0, totalcrldist = 0;
    double totalxydist = 0, totalzdist = 0, totaltimedist = 0;
    double totalrealdist = 0;
    double velocity = 0;
    if ( appvelfld_ )
    {
	velocity = appvelfld_->getDValue();
	if ( !veluom->isSI() )
	    velocity = veluom->getSIValue( velocity );
    }

    for ( int idx=1; idx<size; idx++ )
    {
	const Coord xy = points[idx].coord();
	const Coord prevxy = points[idx-1].coord();
	const BinID bid = SI().transform( xy );
	const BinID prevbid = SI().transform( prevxy );
	totalinldist += abs( bid.inl() - prevbid.inl() );
	totalcrldist += abs( bid.crl() - prevbid.crl() );

	double xydist = xy.distTo( prevxy );
	if ( !xyuom->isSI() )
	    xydist = xyuom->getSIValue( xydist );
	totalxydist += xydist;

	double zdiff = fabs( points[idx-1].z_ - points[idx].z_ );
	double zdist = 0;
	if ( SI().zIsTime() )
	{
	    totaltimedist += zdiff;
	    zdist = zdiff * velocity / 2;
	}
	else
	{
	    if ( zuom->isSI() )
		zdist = zdiff;
	    else
		zdist = zuom->getSIValue( zdiff );
	}

	totalzdist += zdist;
	totalrealdist += Math::Sqrt( xydist*xydist + zdist*zdist );
    }

    const bool showdip = points.size() == 2;
    if ( !showdip )
	dipfld_->setEmpty();
    else
    {
	float dipval;
	if ( dipunitfld_->currentItem() == 0 ) // Degrees
	{
	    dipval = mIsZero(totalxydist,1e-3) ? 90.f
		: Math::toDegrees( Math::Atan2(totalzdist,totalxydist) );
	}
	else
	{
	    if ( SI().zIsTime() )
		dipval = totaltimedist * 1e6 / totalxydist;
	    else
		dipval = totalzdist * 1e3 / totalxydist;
	}

	dipfld_->setValue( dipval );
    }

    dipfld_->setSensitive( showdip );

    uiGenInput* unitfld = hp_unitflds.getParam( this );
    const bool doconv = !unitfld->getBoolValue();
    if ( doconv )
    {
	const UnitOfMeasure* meteruom = UnitOfMeasure::meterUnit();
	const UnitOfMeasure* feetuom = UnitOfMeasure::feetUnit();
	totalxydist = getConvertedValue( totalxydist, meteruom, feetuom );
	totalzdist = getConvertedValue( totalzdist, meteruom, feetuom );
	totalrealdist = getConvertedValue( totalrealdist, meteruom, feetuom );
    }

    hdistfld_->setValue( totalxydist );
    if ( zDistTimeField() )
	zDistTimeField()->setValue( totaltimedist*SI().zDomain().userFactor() );

    zdistfld_->setValue( totalzdist );
    totalDistField()->setValue( totalrealdist );
    inlcrldistfld_->setValue( Interval<int>(totalinldist,totalcrldist) );

    raise();
}


void uiMeasureDlg::dipUnitSel( CallBacker* )
{
    dipUnitChange.trigger();
}


void uiMeasureDlg::unitSelCB( CallBacker* )
{
    dipUnitChange.trigger();
}
