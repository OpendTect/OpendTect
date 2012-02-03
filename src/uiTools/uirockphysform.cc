/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert / Helene
 Date:          Feb 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uirockphysform.cc,v 1.1 2012-02-03 13:00:59 cvsbert Exp $";

#include "uirockphysform.h"
#include "rockphysics.h"
#include "mathproperty.h"

#include "uicombobox.h"
#include "uimsg.h"


uiRockPhysForm::uiRockPhysForm( uiParent* p )
    : uiGroup(p,"RockPhyics Formula Selector")
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "Property Type" );
    nmfld_ = lcb->box();
    nmfld_->selectionChanged.notify( mCB(this,uiRockPhysForm,nameSel) );

    setHAlignObj( lcb );
}


void uiRockPhysForm::setType( PropertyRef::StdType typ )
{
    nmfld_->setEmpty();
    BufferStringSet nms;
    ROCKPHYSFORMS().getRelevant( typ, nms );
    nmfld_->addItems( nms );
    nameSel( 0 );
}


void uiRockPhysForm::nameSel( CallBacker* cb )
{
    // clean up
    const char* txt = nmfld_->text();
    if ( !txt || !*txt ) return;

    const RockPhysics::Formula* fm = ROCKPHYSFORMS().get( txt );
    if ( !fm )
	{ uiMSG().error( "Internal [impossible?]: formula not found" ); return;}

    MathProperty* mp = fm->getProperty();
    if ( !mp )
	{ uiMSG().error( "No property defined for this type" ); return; }

    const int nrconsts = mp->nrConsts();
    if ( nrconsts != fm->constdefs_.size() )
	{ uiMSG().error( "Formula doesn't match repository [c]!" ); return; }
    BufferString msg( "TODO ...\nNr consts: ", nrconsts );
    for ( int idx=0; idx<nrconsts; idx++ )
	msg.add( "\n" ).add( fm->constdefs_[idx]->name() );
    const int nrvars = mp->nrInputs();
    if ( nrvars != fm->vardefs_.size() )
	{ uiMSG().error( "Formula doesn't match repository [v]!" ); return; }
    msg.add( "\nNr Vars:" ).add( nrvars );
    for ( int idx=0; idx<nrvars; idx++ )
	msg.add( "\n" ).add( fm->vardefs_.get(idx) );

    uiMSG().message( msg );
}


BufferString uiRockPhysForm::getText() const
{
    BufferString ret( "TODO * implement" );

    //TODO construct it, remember to replace ' ' with '_' in variable names

    return ret;
}
