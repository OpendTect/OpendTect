/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2001
 RCS:           $Id: uiselsimple.cc,v 1.9 2007-01-09 16:36:11 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiselsimple.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uigeninput.h"
#include "bufstringset.h"


uiSelectFromList::uiSelectFromList( uiParent* p, const Setup& setup )
	: uiDialog(p,setup)
	, selfld_(0)
	, sel_(-1)
{
    const int sz = setup.items_.size();
    if ( sz < 1 )
	{ new uiLabel(this,"No items available for selection"); return; }

    selfld_ = new uiListBox( this );
    selfld_->addItems( setup.items_ );
    if ( setup.current_.isEmpty() )
	selfld_->setCurrentItem( 0 );
    else
	selfld_->setCurrentItem( setup.current_ );

    selfld_->doubleClicked.notify( mCB(this,uiDialog,accept) );
}


bool uiSelectFromList::acceptOK( CallBacker* )
{
    sel_ = selfld_ ? selfld_->currentItem() : -1;
    return true;
}


uiGetObjectName::uiGetObjectName( uiParent* p, const Setup& setup )
	: uiDialog(p,setup)
	, listfld_(0)
{
    if ( setup.items_.size() > 0 )
    {
	listfld_ = new uiListBox( this );
	for ( int idx=0; idx<setup.items_.size(); idx++ )
	    listfld_->addItem( setup.items_.get(idx) );
	if ( !setup.deflt_.isEmpty() )
	    listfld_->setCurrentItem( setup.deflt_ );
	else
	    listfld_->setCurrentItem( 0 );
	listfld_->selectionChanged.notify( mCB(this,uiGetObjectName,selChg) );
	listfld_->doubleClicked.notify( mCB(this,uiDialog,accept) );
    }

    BufferString defnm( setup.deflt_ );
    if ( defnm.isEmpty() && listfld_ )
	defnm = setup.items_.get(0);
    inpfld_ = new uiGenInput( this, setup.inptxt_, defnm );
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
