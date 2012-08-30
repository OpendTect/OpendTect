/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2012
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uistrateditlayer.cc,v 1.1 2012-08-30 13:11:22 cvsbert Exp $";

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
{
    if ( !editable )
	setCtrlStyle( LeaveOnly );

    lithfld_ = new uiGenInput( this, "Lithology", lay.lithology().name() );
    lithfld_->setReadOnly();
    const bool depthinft = SI().depthsInFeet();
    float dpth = lay.zTop(); if ( depthinft ) dpth *= mToFeetFactorF;
    topfld_ = new uiGenInput( this, "Top depth", FloatInpSpec(dpth) );
    topfld_->attach( alignedBelow, lithfld_ );
    topfld_->setReadOnly();

    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, topfld_ );

    for ( int ival=0; ival<lay.nrValues(); ival++ )
    {
	if ( ival >= ls.propertyRefs().size() )
	    break;

	const float val = lay.value( ival );
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


bool uiStratEditLayer::acceptOK( CallBacker* )
{
    if ( !editable_ )
	return true;

    //TODO support editing
    uiMSG().warning( "Editing not yet supported."
	    "\nAny changed value will be reset." );
    return true;
}


void uiStratEditLayer::getUnits( ObjectSet<const UnitOfMeasure>& uns ) const
{
    uns.allowNull(true);
    for ( int idx=0; idx<valflds_.size(); idx++ )
	uns += valflds_[idx]->getUnit();
}
