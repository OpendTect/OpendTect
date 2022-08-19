/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistrateditlayer.h"
#include "stratlayersequence.h"
#include "stratlayer.h"
#include "stratlith.h"
#include "stratunitref.h"
#include "propertyref.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "uipropvalfld.h"
#include "uistratlaycontent.h"
#include "uigeninput.h"
#include "uiseparator.h"
#include "uimsg.h"
#include "od_helpids.h"


uiStratEditLayer::uiStratEditLayer( uiParent* p, Strat::Layer& lay,
			const Strat::LayerSequence& ls, bool editable )
    : uiDialog(p,Setup(tr("Layer properties"),
		       tr("Layer: '%1'").arg(lay.name()),
                       mODHelpKey(mStratEditLayerHelpID) ))
    , editable_(editable)
    , lay_(lay)
    , worklay_(*new Strat::Layer(lay))
    , chgd_(false)
{
    if ( !editable_ )
	setCtrlStyle( CloseOnly );

    lithfld_ = new uiGenInput( this, uiStrings::sLithology(),
                               lay_.lithology().name() );
    lithfld_->setReadOnly();
    const bool depthinft = SI().depthsInFeet();
    float dpth = lay_.zTop(); if ( depthinft ) dpth *= mToFeetFactorF;
    const uiString thtxt(tr("Top depth (%1").arg( depthinft ? tr("ft)")
							    : tr("m)")));
    topfld_ = new uiGenInput( this, thtxt, FloatInpSpec(dpth) );
    topfld_->attach( alignedBelow, lithfld_ );
    topfld_->setReadOnly();

    auto* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, topfld_ );

    uiGroup* algrp = topfld_;
    for ( int ival=0; ival<lay_.nrValues(); ival++ )
    {
	if ( ival >= ls.propertyRefs().size() )
	    break;

	const float val = lay_.value( ival );
	const PropertyRef& pr = *ls.propertyRefs()[ival];

	auto* valfld = new uiPropertyValFld( this, pr, val );
	if ( ival == 0 )
	{
	    if ( depthinft )
		valfld->setUnit( UnitOfMeasure::surveyDefDepthUnit() );
	    valfld->attach( ensureBelow, sep );
	}
	valfld->attach( alignedBelow, algrp );
	algrp = valfld;
	valflds_ += valfld;
	if ( editable_ && !lay_.isMath(ival) )
	    valfld->valueChanged.notify( mCB(this,uiStratEditLayer,valChg) );
	else
	    valfld->setReadOnly( true );
    }

    contfld_ = new uiStratLayerContent( this, true, lay.unitRef().refTree() );
    contfld_->set( lay.content() );
    contfld_->attach( alignedBelow, algrp );
}


uiStratEditLayer::~uiStratEditLayer()
{
    delete &worklay_;
}


bool uiStratEditLayer::getFromScreen( bool emituierrs )
{
    chgd_ = false;
    if ( !editable_ )
	return true;

    TypeSet<float> oldvals, newvals;
    for ( int ival=0; ival<lay_.nrValues(); ival++ )
    {
	if ( ival >= valflds_.size() )
	    break;
	const float val = valflds_[ival]->getValue();
	uiString msg;
	if ( mIsUdf(val) )
	    msg =  tr("Please enter a value for %1")
		 .arg( valflds_[ival]->propName() );
	else if ( ival == 0 && val <= 0 )
	    msg = tr("Please set the thickness to a positive number");
	if ( !msg.isEmpty() )
	    { if ( emituierrs ) uiMSG().error( msg ); return false; }

	oldvals += lay_.value( ival );
	newvals += val;
    }

    for ( int ival=0; ival<newvals.size(); ival++ )
    {
	const float valdiff = oldvals[ival] - newvals[ival];
	const Strat::LayerValue* lv = lay_.getLayerValue( ival );
	const bool mustclone = lv && !lv->isSimple();
	const bool issame = mIsZero(valdiff,1e-8);
	if ( !issame )
	    chgd_ = true;
	if ( mustclone || issame )
	    worklay_.setValue( ival, lv->clone(&worklay_) );
	else
	    worklay_.setValue( ival, newvals[ival] );
    }

    const Strat::Content& newcontent = contfld_->get();
    if ( &lay_.content() != &newcontent )
	chgd_ = true;
    worklay_.setContent( newcontent );

    return true;
}


void uiStratEditLayer::valChg( CallBacker* )
{
    if ( !getFromScreen(false) )
	return;

    for ( int ival=0; ival<valflds_.size(); ival++ )
    {
	if ( worklay_.isMath(ival) )
	    valflds_[ival]->setValue( worklay_.value(ival) );
    }
}


bool uiStratEditLayer::acceptOK( CallBacker* )
{
    if ( !editable_ )
	return true;
    else if ( !getFromScreen(true) )
	return false;

    if ( chgd_ )
	const_cast<Strat::Layer&>(lay_) = worklay_;

    return true;
}
