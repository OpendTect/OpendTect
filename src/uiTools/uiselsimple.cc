/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiselsimple.cc,v 1.14 2008-11-25 15:35:26 cvsbert Exp $";

#include "uiselsimple.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uigeninput.h"
#include "bufstringset.h"


uiSelectFromList::uiSelectFromList( uiParent* p, const Setup& sup )
	: uiDialog(p,sup)
	, selfld_(0)
	, sel_(-1)
{
    const int sz = sup.items_.size();
    if ( sz < 1 )
	{ new uiLabel(this,"No items available for selection"); return; }

    selfld_ = new uiListBox( this );
    selfld_->setName("Select Data from List");
    selfld_->addItems( sup.items_ );
    if ( sup.current_ < 1 )
	selfld_->setCurrentItem( 0 );
    else
	selfld_->setCurrentItem( sup.current_ );

    selfld_->setHSzPol( uiObject::Wide );
    selfld_->doubleClicked.notify( mCB(this,uiDialog,accept) );
}


bool uiSelectFromList::acceptOK( CallBacker* )
{
    sel_ = selfld_ ? selfld_->currentItem() : -1;
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
