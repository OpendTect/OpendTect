/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2001
 RCS:           $Id: uiselsimple.cc,v 1.10 2007-01-11 12:37:49 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiselsimple.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uigeninput.h"
#include "bufstringset.h"


uiSelectFromList::uiSelectFromList( uiParent* p, const Setup& mysetup )
	: uiDialog(p,mysetup)
	, selfld_(0)
	, sel_(-1)
{
    const int sz = mysetup.items_.size();
    if ( sz < 1 )
	{ new uiLabel(this,"No items available for selection"); return; }

    selfld_ = new uiListBox( this );
    selfld_->addItems( mysetup.items_ );
    if ( mysetup.current_.isEmpty() )
	selfld_->setCurrentItem( 0 );
    else
	selfld_->setCurrentItem( mysetup.current_ );

    selfld_->doubleClicked.notify( mCB(this,uiDialog,accept) );
}


bool uiSelectFromList::acceptOK( CallBacker* )
{
    sel_ = selfld_ ? selfld_->currentItem() : -1;
    return true;
}


uiGetObjectName::uiGetObjectName( uiParent* p, const Setup& mysetup )
	: uiDialog(p,mysetup)
	, listfld_(0)
{
    if ( mysetup.items_.size() > 0 )
    {
	listfld_ = new uiListBox( this );
	for ( int idx=0; idx<mysetup.items_.size(); idx++ )
	    listfld_->addItem( mysetup.items_.get(idx) );
	if ( !mysetup.deflt_.isEmpty() )
	    listfld_->setCurrentItem( mysetup.deflt_ );
	else
	    listfld_->setCurrentItem( 0 );
	listfld_->selectionChanged.notify( mCB(this,uiGetObjectName,selChg) );
	listfld_->doubleClicked.notify( mCB(this,uiDialog,accept) );
    }

    BufferString defnm( mysetup.deflt_ );
    if ( defnm.isEmpty() && listfld_ )
	defnm = mysetup.items_.get(0);
    inpfld_ = new uiGenInput( this, mysetup.inptxt_, defnm );
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
