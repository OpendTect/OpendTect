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
#include "attribdescsetsholder.h"
#include "attribprovider.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uilistbox.h"
#include "uimultcomputils.h"
#include "uitoolbutton.h"
#include "od_helpids.h"

using namespace Attrib;

void uiMultOutSel::fillInAvailOutNames( const Desc& desc,
					BufferStringSet& outnames )
{
    uiString errmsg;
    Desc& ds = const_cast<Desc&>(desc);
    RefMan<Provider> tmpprov = Provider::create( ds, errmsg );
    if ( !tmpprov ) return;

    //compute and set refstep, needed to get nr outputs for some attribs
    //( SpecDecomp for ex )
    tmpprov->computeRefStep();
    tmpprov->getCompNames( outnames );
}


uiMultOutSel::uiMultOutSel( uiParent* p, const Desc& desc )
	: uiDialog(p,Setup(tr("Multiple components selection"),
			   tr("Select the outputs to compute"),
                           mODHelpKey(mMultOutSelHelpID) ))
	, outlistfld_(0)
	, outallfld_(0)
{
    BufferStringSet outnames;
    Desc* tmpdesc = new Desc( desc );
    tmpdesc->ref();
    fillInAvailOutNames( *tmpdesc, outnames );
    const bool dodlg = outnames.size() > 1;
    if ( dodlg )
	createMultOutDlg( outnames );

    tmpdesc->unRef();
}


void uiMultOutSel::createMultOutDlg( const BufferStringSet& outnames )
{
    outlistfld_ = new uiListBox( this, "Outputs", OD::ChooseAtLeastOne );
    outlistfld_->addItems( outnames );

    outallfld_ = new uiCheckBox( this, tr("Output all"));
    outallfld_->activated.notify( mCB(this,uiMultOutSel,allSel) );
    outallfld_->attach( alignedBelow, outlistfld_ );
}


void uiMultOutSel::getSelectedOutputs( TypeSet<int>& selouts ) const
{
    if ( outlistfld_ )
	outlistfld_->getChosen( selouts );
}


void uiMultOutSel::getSelectedOutNames( BufferStringSet& seloutnms ) const
{
    if ( outlistfld_ )
	outlistfld_->getChosen( seloutnms );
}


bool uiMultOutSel::doDisp() const
{
    return outlistfld_;
}


void uiMultOutSel::allSel( CallBacker* c )
{
    outlistfld_->chooseAll( outallfld_->isChecked() );
}


bool uiMultOutSel::handleMultiCompChain( Attrib::DescID& attribid,
					const Attrib::DescID& multicompinpid,
					bool is2d, const SelInfo& attrinf,
					Attrib::DescSet* curdescset,
					uiParent* parent,
					TypeSet<Attrib::SelSpec>& targetspecs)
{
    if ( !curdescset ) return false;
    Desc* seldesc = curdescset->getDesc( attribid );
    if ( !seldesc )
	return false;

    Desc* inpdesc = curdescset->getDesc( multicompinpid );
    if ( !inpdesc ) return false;

    BufferStringSet complist;
    uiMultOutSel::fillInAvailOutNames( *inpdesc, complist );
    uiMultCompDlg compdlg( parent, complist );
    if ( compdlg.go() )
    {
	LineKey lk;
	BufferString userrefstr ( inpdesc->userRef() );
	userrefstr.trimBlanks();
	if ( stringEndsWith( "|ALL", userrefstr.buf() ))
	    userrefstr[ userrefstr.size()-4 ] = '\0';

	if ( is2d )
	{
	    const MultiID mid( attrinf.ioobjids_.get(0) );
	    lk = LineKey( mid, userrefstr );
	}
	else
	{
	    const int inpidx = attrinf.ioobjnms_.indexOf( userrefstr.buf() );
	    if ( inpidx<0 ) return false;

	    const char* objidstr = attrinf.ioobjids_.get(inpidx);
	    lk = LineKey( objidstr );
	}

	TypeSet<int> selectedcomps;
	compdlg.getCompNrs( selectedcomps );
	const int selcompssz = selectedcomps.size();
	if ( selcompssz )
	    targetspecs.erase();

	for ( int idx=0; idx<selcompssz; idx++ )
	{
	    const int compidx = selectedcomps[idx];
	    const DescID newinpid = curdescset->getStoredID( lk, compidx,
					    true, true, complist.get(compidx) );
	    Desc* newdesc = seldesc->cloneDescAndPropagateInput( newinpid,
							complist.get(compidx) );
	    if ( !newdesc ) continue;

	    DescID newdid = curdescset->getID( *newdesc );
	    SelSpec as( 0, newdid );
	    BufferString bfs;
	    newdesc->getDefStr( bfs );
	    as.setDefString( bfs.buf() );
	    as.setRefFromID( *curdescset );
	    as.set2DFlag( is2d );
	    targetspecs += as;
	}
    }

    return true;
}



// uiMultiAttribSel

uiMultiAttribSel::uiMultiAttribSel( uiParent* p, const Attrib::DescSet& ds )
    : uiGroup(p,"MultiAttrib group")
    , descset_(ds)
{
#define mLblPos uiLabeledListBox::AboveLeft
    uiLabeledListBox* attrllb = new uiLabeledListBox( this,
	    "Available attributes", OD::ChooseAtLeastOne, mLblPos );
    attribfld_ = attrllb->box();
    attribfld_->setHSzPol( uiObject::Wide );
    attribfld_->selectionChanged.notify( mCB(this,uiMultiAttribSel,entrySel) );

    uiButtonGroup* bgrp = new uiButtonGroup( this, "", OD::Vertical );
    new uiToolButton( bgrp, uiToolButton::RightArrow, uiStrings::sAdd(true),
		      mCB(this,uiMultiAttribSel,doAdd) );
    new uiToolButton( bgrp, uiToolButton::LeftArrow, "Don't use",
		      mCB(this,uiMultiAttribSel,doRemove) );
    bgrp->attach( centeredRightOf, attrllb );

    uiLabeledListBox* selllb = new uiLabeledListBox( this,
	    "Selected attributes", OD::ChooseAtLeastOne, mLblPos );
    selfld_ = selllb->box();
    selfld_->setHSzPol( uiObject::Wide );
    selllb->attach( rightTo, attrllb );
    selllb->attach( ensureRightOf, bgrp );

    uiButtonGroup* sortgrp = new uiButtonGroup( this, "", OD::Vertical );
    new uiToolButton( sortgrp, uiToolButton::UpArrow,"Move up",
		      mCB(this,uiMultiAttribSel,moveUp) );
    new uiToolButton( sortgrp, uiToolButton::DownArrow, "Move down",
		      mCB(this,uiMultiAttribSel,moveDown) );
    sortgrp->attach( centeredRightOf, selllb );

    allcompfld_ = new uiCheckBox( this, "Use all possible outputs" );
    allcompfld_->attach( alignedBelow, attrllb );
    allcompfld_->setSensitive( false );
    
    fillAttribFld();
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
    attribfld_->getChosen( selidxs );
    if ( selidxs.isEmpty() )
	return;

    for ( int idx=0; idx<selidxs.size(); idx++ )
	selids_ += allids_[ selidxs[idx] ];

    if ( allcompfld_->sensitive() && allcompfld_->isChecked() )
    {
	const Desc* seldesc =
	    descset_.getDesc( descset_.getID(attribfld_->getText(),true) );
	if ( !seldesc )
	{
	    updateSelFld();
	    return;
	}

	const int seldescouputidx = seldesc->selectedOutput();
	BufferStringSet alluserrefs;
	uiMultOutSel::fillInAvailOutNames( *seldesc, alluserrefs );
	const BufferString baseusrref = seldesc->userRef();
	for ( int idx=0; idx<alluserrefs.size(); idx++ )
	{
	    const BufferString usrref( baseusrref, "_", alluserrefs.get(idx) );
	    if ( idx == seldescouputidx )
	    {
		const_cast<Desc*>(seldesc)->setUserRef( usrref );
		continue;
	    }

	    Desc* tmpdesc = new Desc( *seldesc );
	    tmpdesc->ref();
	    tmpdesc->selectOutput( idx );
	    tmpdesc->setUserRef( usrref );
	    const DescID newid =
		const_cast<Attrib::DescSet*>(&descset_)->addDesc( tmpdesc );
	    allids_ += newid;
	    selids_ += newid;
	}
    }

    updateSelFld();
}


void uiMultiAttribSel::doRemove( CallBacker* )
{
    TypeSet<int> selidxs;
    selfld_->getChosen( selidxs );
    if ( selidxs.isEmpty() )
	return;

    TypeSet<DescID> ids2rem;
    for ( int idx=0; idx<selidxs.size(); idx++ )
	ids2rem += selids_[ selidxs[idx] ];

    selids_.createDifference( ids2rem, true );
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


void uiMultiAttribSel::entrySel( CallBacker* )
{
    BufferStringSet outnames;
    const Desc* seldesc =
	descset_.getDesc( descset_.getID(attribfld_->getText(),true) );
    if ( !seldesc ) return;

    uiMultOutSel::fillInAvailOutNames( *seldesc, outnames );
    allcompfld_->setSensitive( outnames.size()>1 );
}

