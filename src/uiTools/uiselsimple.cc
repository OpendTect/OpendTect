/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiselsimple.cc,v 1.21 2012/03/14 11:09:32 cvsranojay Exp $";

#include "uiselsimple.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uigeninput.h"
#include "uichecklist.h"
#include "globexpr.h"
#include "bufstringset.h"


uiSelectFromList::uiSelectFromList( uiParent* p, const Setup& sup )
	: uiDialog(p,sup)
	, setup_(sup)
	, selfld_(0)
{
    const int sz = setup_.items_.size();
    if ( sz < 1 )
    {
	new uiLabel(this,"No items available for selection");
	setCtrlStyle( LeaveOnly );
	return;
    }

    filtfld_ = new uiGenInput( this, "Filter", "*" );
    filtfld_->valuechanged.notify( mCB(this,uiSelectFromList,filtChg) );

    selfld_ = new uiListBox( this );
    selfld_->setName("Select Data from List");
    selfld_->addItems( setup_.items_ );
    if ( setup_.current_ < 1 )
	selfld_->setCurrentItem( 0 );
    else
	selfld_->setCurrentItem( setup_.current_ );
    selfld_->attach( centeredBelow, filtfld_ );

    selfld_->setHSzPol( uiObject::Wide );
    selfld_->doubleClicked.notify( mCB(this,uiDialog,accept) );
}


void uiSelectFromList::filtChg( CallBacker* )
{
    const char* filt = filtfld_->text();
    if ( !filt || !*filt ) filt = "*";

    BufferString cursel( selfld_->getText() );
    selfld_->setEmpty();
    GlobExpr ge( filt );
    for ( int idx=0; idx<setup_.items_.size(); idx++ )
    {
	const char* itm = setup_.items_.get( idx );
	if ( ge.matches(itm) )
	    selfld_->addItem( itm );
    }

    if ( selfld_->isPresent(cursel) )
	selfld_->setCurrentItem( cursel );
    else
	selfld_->setCurrentItem( 0 );
}


bool uiSelectFromList::acceptOK( CallBacker* )
{
    if ( !selfld_ ) return false;

    const int selidx = selfld_->currentItem();
    if ( selidx < 0 ) return false;

    const char* seltxt = selfld_->textOfItem( selidx );
    setup_.current_ = setup_.items_.indexOf( seltxt );
    return true;
}


uiGetObjectName::uiGetObjectName( uiParent* p, const Setup& sup )
	: uiDialog(p,sup)
	, listfld_(0)
{
    if ( sup.items_.size() > 0 )
    {
	listfld_ = new uiListBox( this );
	for ( int idx=0; idx<sup.items_.size(); idx++ )
	    listfld_->addItem( sup.items_.get(idx) );
	if ( !sup.deflt_.isEmpty() )
	    listfld_->setCurrentItem( sup.deflt_ );
	else
	    listfld_->setCurrentItem( 0 );
	listfld_->selectionChanged.notify( mCB(this,uiGetObjectName,selChg) );
	listfld_->doubleClicked.notify( mCB(this,uiDialog,accept) );
    }

    BufferString defnm( sup.deflt_ );
    if ( defnm.isEmpty() && listfld_ )
	defnm = sup.items_.get(0);
    inpfld_ = new uiGenInput( this, sup.inptxt_, defnm );
    if ( listfld_ )
	inpfld_->attach( alignedBelow, listfld_ );
}


void uiGetObjectName::selChg( CallBacker* )
{
    if ( listfld_ )
	inpfld_->setText( listfld_->getText() );
}


const char* uiGetObjectName::text() const
{
    return inpfld_->text();
}


bool uiGetObjectName::acceptOK( CallBacker* )
{
    const char* txt = text();
    return *txt ? true : false;
}


uiGetChoice::uiGetChoice( uiParent* p, const BufferStringSet& opts,
			  const char* qn, bool wcncl, const char* hid )
    : uiDialog(p,uiDialog::Setup("Please specify",qn,hid))
    , allowcancel_(wcncl)
{
    inpfld_ = new uiCheckList( this, opts, uiCheckList::OneOnly );
    setDefaultChoice( 0 );
}


uiGetChoice::uiGetChoice( uiParent* p, uiDialog::Setup s,
			  const BufferStringSet& opts, bool wc )
    : uiDialog(p,s)
    , allowcancel_(wc)
{
    inpfld_ = new uiCheckList( this, opts, uiCheckList::OneOnly );
    setDefaultChoice( 0 );
}

void uiGetChoice::setDefaultChoice( int nr )
{
    inpfld_->setChecked( nr, true );
}


bool uiGetChoice::rejectOK( CallBacker* )
{
    if ( !allowcancel_ )
	return false;
    choice_ = -1;
    return true;
}


bool uiGetChoice::acceptOK( CallBacker* )
{
    choice_ = inpfld_->firstChecked();
    return true;
}
