/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          Jan 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimultoutsel.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribprovider.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uilistbox.h"
#include "uitoolbutton.h"

using namespace Attrib;

uiMultOutSel::uiMultOutSel( uiParent* p, const Desc& desc )
	: uiDialog(p,Setup("Multiple components selection",
		    	   "Select the outputs to compute", "101.2.3"))
	, outlistfld_(0)
	, outallfld_(0)
{
    BufferStringSet outnames;
    Desc* tmpdesc = new Desc( desc );
    tmpdesc->ref();
    fillInAvailOutNames( tmpdesc, outnames );
    const bool dodlg = outnames.size() > 1;
    if ( dodlg )
	createMultOutDlg( outnames );

    tmpdesc->unRef();
}


void uiMultOutSel::fillInAvailOutNames( Desc* desc,
					BufferStringSet& outnames ) const
{
    BufferString errmsg;
    Provider* tmpprov = Provider::create( *desc, errmsg );
    if ( !tmpprov ) return;
    tmpprov->ref();

    //compute and set refstep, needed to get nr outputs for some attribs
    //( SpecDecomp for ex )
    tmpprov->computeRefStep();

    tmpprov->getCompNames( outnames );
    tmpprov->unRef();
}


void uiMultOutSel::createMultOutDlg( const BufferStringSet& outnames )
{
    outlistfld_ = new uiListBox( this );
    outlistfld_->setMultiSelect();
    outlistfld_->addItems( outnames );

    outallfld_ = new uiCheckBox( this, "Output all");
    outallfld_->activated.notify( mCB(this,uiMultOutSel,allSel) );
    outallfld_->attach( alignedBelow, outlistfld_ );
}


void uiMultOutSel::getSelectedOutputs( TypeSet<int>& selouts ) const
{
    if ( outlistfld_ )
	outlistfld_->getSelectedItems( selouts );
}


void uiMultOutSel::getSelectedOutNames( BufferStringSet& seloutnms ) const
{
    if ( outlistfld_ )
	outlistfld_->getSelectedItems( seloutnms );
}


bool uiMultOutSel::doDisp() const
{
    return outlistfld_;
}


void uiMultOutSel::allSel( CallBacker* c )
{
    outlistfld_->selectAll( outallfld_->isChecked() );
}



// uiMultiAttribSel

uiMultiAttribSel::uiMultiAttribSel( uiParent* p, const Attrib::DescSet& ds )
    : uiGroup(p,"MultiAttrib group")
    , descset_(ds)
{
#define mLblPos uiLabeledListBox::AboveLeft
    uiLabeledListBox* attrllb =
	new uiLabeledListBox( this, "Available attributes", true, mLblPos );
    attribfld_ = attrllb->box();
    attribfld_->setHSzPol( uiObject::Wide );
    fillAttribFld();

    uiButtonGroup* bgrp = new uiButtonGroup( this, "", true );
    new uiToolButton( bgrp, uiToolButton::RightArrow,"Add",
		      mCB(this,uiMultiAttribSel,doAdd) );
    new uiToolButton( bgrp, uiToolButton::LeftArrow, "Don't use",
		      mCB(this,uiMultiAttribSel,doRemove) );
    bgrp->attach( centeredRightOf, attrllb );

    uiLabeledListBox* selllb =
	new uiLabeledListBox( this, "Selected attributes", false, mLblPos );
    selfld_ = selllb->box();
    selfld_->setHSzPol( uiObject::Wide );
    selllb->attach( rightTo, attrllb );
    selllb->attach( ensureRightOf, bgrp );

    uiButtonGroup* sortgrp = new uiButtonGroup( this, "", true );
    new uiToolButton( sortgrp, uiToolButton::UpArrow,"Move up",
		      mCB(this,uiMultiAttribSel,moveUp) );
    new uiToolButton( sortgrp, uiToolButton::DownArrow, "Move down",
		      mCB(this,uiMultiAttribSel,moveDown) );
    sortgrp->attach( centeredRightOf, selllb );

    setHAlignObj( attrllb );
}


uiMultiAttribSel::~uiMultiAttribSel()
{}


bool uiMultiAttribSel::is2D() const
{ return descset_.is2D(); }


void uiMultiAttribSel::fillAttribFld()
{
    attribfld_->setEmpty();
    for ( int idx=0; idx<descset_.size(); idx++ )
    {
	const Attrib::Desc& desc = descset_[idx];
	if ( desc.isHidden() || desc.isStored() )
	    continue;

	allids_ += desc.id();
	attribfld_->addItem( desc.userRef() );
    }
}


void uiMultiAttribSel::updateSelFld()
{
    selfld_->setEmpty();
    for ( int idx=0; idx<selids_.size(); idx++ )
    {
	const Attrib::Desc* desc = descset_.getDesc( selids_[idx] );
	if (!desc ) continue;

	selfld_->addItem( desc->userRef() );
    }
}


void uiMultiAttribSel::doAdd( CallBacker* )
{
    TypeSet<int> selidxs;
    attribfld_->getSelectedItems( selidxs );
    if ( selidxs.isEmpty() )
	return;

    for ( int idx=0; idx<selidxs.size(); idx++ )
	selids_ += allids_[ selidxs[idx] ];
    updateSelFld();
}


void uiMultiAttribSel::doRemove( CallBacker* )
{
    const int idx = selfld_->currentItem();
    if ( !selids_.validIdx(idx) )
	return;

    selids_.removeSingle( idx );
    updateSelFld();
}


void uiMultiAttribSel::moveUp( CallBacker* )
{
    const int idx = selfld_->currentItem();
    if ( idx==0 || !selids_.validIdx(idx) )
	return;

    selids_.swap( idx, idx-1);
    updateSelFld();
    selfld_->setCurrentItem( idx-1 );
}


void uiMultiAttribSel::moveDown( CallBacker* )
{
    const int idx = selfld_->currentItem();
    if ( idx==selids_.size()-1 || !selids_.validIdx(idx) )
	return;

    selids_.swap( idx, idx+1 );
    updateSelFld();
    selfld_->setCurrentItem( idx+1 );

}


void uiMultiAttribSel::getSelIds( TypeSet<Attrib::DescID>& ids ) const
{ ids = selids_; }

