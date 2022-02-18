/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          Jan 2009
________________________________________________________________________

-*/

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


static void getOutputIDs( const Desc& desc, TypeSet<int>& ids )
{
    ids.erase();

    uiString errmsg;
    Desc& ds = const_cast<Desc&>(desc);
    RefMan<Provider> tmpprov = Provider::create( ds, errmsg );
    if ( !tmpprov )
	return;

    tmpprov->getCompOutputIDs( ids );
}


uiMultOutSel::uiMultOutSel( uiParent* p, const Desc& desc )
	: uiDialog(p,Setup(tr("Multiple attributes selection"),
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

    getOutputIDs( *tmpdesc, outputids_ );

    tmpdesc->unRef();
}


void uiMultOutSel::createMultOutDlg( const BufferStringSet& outnames )
{
    outlistfld_ = new uiListBox( this, "Outputs", OD::ChooseAtLeastOne );
    outlistfld_->addItems( outnames );

    outallfld_ = new uiCheckBox( this, uiStrings::phrOutput(uiStrings::sAll()));
    outallfld_->activated.notify( mCB(this,uiMultOutSel,allSel) );
    outallfld_->attach( alignedBelow, outlistfld_ );
    outallfld_->display( false, true );
}


void uiMultOutSel::getSelectedOutputs( TypeSet<int>& selouts ) const
{
    if ( !outlistfld_ )
	return;

    TypeSet<int> chosen;
    outlistfld_->getChosen( chosen );

    for ( int idx=0; idx<chosen.size(); idx++ )
    {
	const int chidx = chosen[idx];
	if ( outputids_.validIdx(chidx) )
	    selouts += outputids_[chidx];
    }

    if ( selouts.isEmpty() )
	selouts = chosen;
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


void uiMultOutSel::allSel( CallBacker* )
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
    if ( !compdlg.go() )
	return false;

    MultiID mid;
    BufferString userrefstr ( inpdesc->userRef() );
    userrefstr.trimBlanks();
    if ( stringEndsWith( "|ALL", userrefstr.buf() ))
	userrefstr[ userrefstr.size()-4 ] = '\0';

    if ( is2d )
	mid = attrinf.ioobjids_.get( 0 );
    else
    {
	const int inpidx = attrinf.ioobjnms_.indexOf( userrefstr.buf() );
	if ( inpidx<0 )
	    return false;

	mid = attrinf.ioobjids_.get( inpidx );
    }

    TypeSet<int> selectedcomps;
    compdlg.getCompNrs( selectedcomps );
    const int selcompssz = selectedcomps.size();
    if ( selcompssz )
	targetspecs.erase();

    TypeSet<int> outputids;
    getOutputIDs( *inpdesc, outputids );
    for ( int idx=0; idx<selcompssz; idx++ )
    {
	const int compidx = selectedcomps[idx];
	const DescID newinpid = curdescset->getStoredID( mid, compidx,
					true, true, complist.get(compidx) );
	Desc* newdesc = seldesc->cloneDescAndPropagateInput( newinpid,
						    complist.get(compidx) );
	if ( !newdesc ) continue;

	if ( !outputids.isEmpty() )
	    newdesc->selectOutput( outputids[compidx] );

	DescID newdid = curdescset->getID( *newdesc );
	SelSpec as( 0, newdid );
	BufferString bfs;
	newdesc->getDefStr( bfs );
	as.setDefString( bfs.buf() );
	as.setRefFromID( *curdescset );
	as.set2DFlag( is2d );
	targetspecs += as;
    }

    return true;
}



// uiMultiAttribSel
uiMultiAttribSel::uiMultiAttribSel( uiParent* p, const Attrib::DescSet* ds )
    : uiGroup(p,"MultiAttrib group")
    , descset_(ds)
{
    uiListBox::Setup asu( OD::ChooseAtLeastOne, tr("Available Attributes"),
			  uiListBox::AboveMid );
    attribfld_ = new uiListBox( this, asu );
    attribfld_->setHSzPol( uiObject::Wide );

    uiButtonGroup* bgrp = new uiButtonGroup( this, "", OD::Vertical );
    new uiToolButton( bgrp, uiToolButton::RightArrow, uiStrings::sAdd(),
		      mCB(this,uiMultiAttribSel,doAdd) );
    new uiToolButton( bgrp, uiToolButton::LeftArrow, tr("Don't use"),
		      mCB(this,uiMultiAttribSel,doRemove) );
    bgrp->attach( centeredRightOf, attribfld_ );

    uiListBox::Setup ssu( OD::ChooseAtLeastOne, tr("Selected Attributes"),
			  uiListBox::AboveMid );
    selfld_ = new uiListBox( this, ssu );
    selfld_->setHSzPol( uiObject::Wide );
    selfld_->attach( rightTo, attribfld_ );
    selfld_->attach( ensureRightOf, bgrp );

    uiButtonGroup* sortgrp = new uiButtonGroup( this, "", OD::Vertical );
    new uiToolButton( sortgrp, uiToolButton::UpArrow,uiStrings::sMoveUp(),
		      mCB(this,uiMultiAttribSel,moveUp) );
    new uiToolButton( sortgrp, uiToolButton::DownArrow, uiStrings::sMoveDown(),
		      mCB(this,uiMultiAttribSel,moveDown) );
    sortgrp->attach( centeredRightOf, selfld_ );

    fillAttribFld();
    setHAlignObj( attribfld_ );
}


uiMultiAttribSel::~uiMultiAttribSel()
{}


bool uiMultiAttribSel::is2D() const
{ return descset_ && descset_->is2D(); }


void uiMultiAttribSel::fillAttribFld()
{
    attribfld_->setEmpty();
    allids_.setEmpty();
    if ( !descset_ )
	return;

    const int nrdescs = descset_->size();
    for ( int didx=0; didx<nrdescs; didx++ )
    {
	const Attrib::Desc* desc = descset_->desc( didx );
	if ( desc->isHidden() || desc->isStored() )
	    continue;

	const int seldescouputidx = desc->selectedOutput();
	BufferStringSet alluserrefs;
	uiMultOutSel::fillInAvailOutNames( *desc, alluserrefs );
	const BufferString baseusrref = desc->userRef();
	for ( int idx=0; idx<alluserrefs.size(); idx++ )
	{
	    const BufferString usrref( baseusrref, "_", alluserrefs.get(idx) );
	    if ( idx == seldescouputidx )
	    {
		if ( alluserrefs.size() > 1 )
		    const_cast<Desc*>(desc)->setUserRef( usrref );
		allids_ += desc->id();
		attribfld_->addItem( toUiString(desc->userRef()) );
		continue;
	    }

	    Desc* tmpdesc = new Desc( *desc );
	    tmpdesc->ref();
	    tmpdesc->selectOutput( idx );
	    tmpdesc->setUserRef( usrref );
	    const DescID newid =
		const_cast<Attrib::DescSet*>(descset_)->addDesc( tmpdesc );
	    allids_ += newid;
	    attribfld_->addItem( toUiString(tmpdesc->userRef()) );
	}
    }
}


void uiMultiAttribSel::setDescSet( const Attrib::DescSet* descset )
{
    descset_ = descset;
    fillAttribFld();
    updateSelFld();
}


void uiMultiAttribSel::updateSelFld()
{
    selfld_->setEmpty();
    if ( !descset_ )
	return;

    for ( int idx=0; idx<selids_.size(); idx++ )
    {
	const Attrib::Desc* desc = descset_->getDesc( selids_[idx] );
	if (!desc )
	    continue;

	selfld_->addItem( toUiString(desc->userRef()) );
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
