/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2012
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uistrateditlayer.cc,v 1.2 2012-08-30 14:54:23 cvsbert Exp $";

#include "uistrateditlayer.h"
#include "stratlayersequence.h"
#include "stratlayer.h"
#include "stratlith.h"
#include "propertyref.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "uipropvalfld.h"
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

    for ( int ival=0; ival<lay_.nrValues(); ival++ )
    {
	if ( ival >= ls.propertyRefs().size() )
	    break;

	const float val = lay_.value( ival );
	const PropertyRef& pr = *ls.propertyRefs()[ival];

	uiPropertyValFld* valfld = new uiPropertyValFld( this, pr, val );
	if ( ival )
	    valfld->attach( alignedBelow, valflds_[ival-1] );
	else
	{
	    if ( depthinft )
		valfld->setUnit( UnitOfMeasure::surveyDefZUnit() );
	    valfld->attach( alignedBelow, topfld_ );
	    valfld->attach( ensureBelow, sep );
	}
	valflds_ += valfld;
    }
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

    return true;
}
