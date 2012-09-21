/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2012
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

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


uiStratEditLayer::uiStratEditLayer( uiParent* p, Strat::Layer& lay,
			const Strat::LayerSequence& ls, bool editable )
    : uiDialog(p,Setup("Layer properties",
		    BufferString("Layer: '",lay.name(),"'"),mTODOHelpID))
    , editable_(editable)
    , lay_(lay)
{
    if ( !editable )
	setCtrlStyle( LeaveOnly );

    lithfld_ = new uiGenInput( this, "Lithology", lay_.lithology().name() );
    lithfld_->setReadOnly();
    const bool depthinft = SI().depthsInFeet();
    float dpth = lay_.zTop(); if ( depthinft ) dpth *= mToFeetFactorF;
    topfld_ = new uiGenInput( this, "Top depth", FloatInpSpec(dpth) );
    topfld_->attach( alignedBelow, lithfld_ );
    topfld_->setReadOnly();

    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, topfld_ );

    uiGroup* algrp = topfld_;
    for ( int ival=0; ival<lay_.nrValues(); ival++ )
    {
	if ( ival >= ls.propertyRefs().size() )
	    break;

	const float val = lay_.value( ival );
	const PropertyRef& pr = *ls.propertyRefs()[ival];

	uiPropertyValFld* valfld = new uiPropertyValFld( this, pr, val );
	if ( ival == 0 )
	{
	    if ( depthinft )
		valfld->setUnit( UnitOfMeasure::surveyDefDepthUnit() );
	    valfld->attach( ensureBelow, sep );
	}
	valfld->attach( alignedBelow, algrp );
	algrp = valfld;
	valflds_ += valfld;
    }

    contfld_ = new uiStratLayerContent( this, &lay.unitRef().refTree() ); 
    contfld_->set( lay.content() );
    contfld_->attach( alignedBelow, algrp );
}


void uiStratEditLayer::getUnits( ObjectSet<const UnitOfMeasure>& uns ) const
{
    uns.allowNull(true);
    for ( int idx=0; idx<valflds_.size(); idx++ )
	uns += valflds_[idx]->getUnit();
}


bool uiStratEditLayer::acceptOK( CallBacker* )
{
    if ( !editable_ )
	return true;

    for ( int ival=0; ival<lay_.nrValues(); ival++ )
    {
	if ( ival >= valflds_.size() )
	    break;
	const float val = valflds_[ival]->getValue();
	BufferString msg;
	if ( mIsUdf(val) )
	    msg.add( "Please enter a value for " )
		.add( valflds_[ival]->propName() );
	else if ( ival == 0 && val <= 0 )
	    msg = "Please set the thickness to a positive number";
	if ( !msg.isEmpty() )
	    { uiMSG().error( msg ); return false; }
    }

    for ( int ival=0; ival<lay_.nrValues(); ival++ )
    {
	if ( ival < valflds_.size() )
	    lay_.setValue( ival, valflds_[ival]->getValue() );
    }
    lay_.setContent( contfld_->get() );

    return true;
}
