/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2001
 RCS:           $Id: uiselsimple.cc,v 1.8 2007-01-09 13:21:06 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiselsimple.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uigeninput.h"
#include "bufstringset.h"

uiSelectFromList::Setup::Setup( const char* titl, const BufferStringSet& bss )
    : uiDialog::Setup(titl)
    , current_(0)
    , items_(bss)
{
}


uiSelectFromList::uiSelectFromList( uiParent* p, const Setup& setup )
	: uiDialog(p,setup)
	, selfld_(0)
	, sel_(-1)
{
    const int sz = setup.items().size();
    if ( sz < 1 )
	{ new uiLabel(this,"No items available for selection"); return; }

    selfld_ = new uiListBox( this );
    selfld_->addItems( setup.items() );
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


uiGetObjectName::uiGetObjectName( uiParent* p, const char* txt,
				  const BufferStringSet& nms,
				  const char* cur, const char* captn )
	: uiDialog(p,Setup(captn))
	, listfld_(0)
{
    if ( nms.size() > 0 )
    {
	listfld_ = new uiListBox( this );
	for ( int idx=0; idx<nms.size(); idx++ )
	    listfld_->addItem( nms.get(idx) );
	if ( cur && *cur )
	    listfld_->setCurrentItem( cur );
	else
	    listfld_->setCurrentItem( 0 );
	listfld_->selectionChanged.notify( mCB(this,uiGetObjectName,selChg) );
	listfld_->doubleClicked.notify( mCB(this,uiDialog,accept) );
    }

    if ( !cur && listfld_ )
	cur = nms.get(0);
    inpfld_ = new uiGenInput( this, txt, cur );
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
