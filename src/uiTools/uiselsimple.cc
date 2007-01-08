/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2001
 RCS:           $Id: uiselsimple.cc,v 1.7 2007-01-08 17:07:26 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiselsimple.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uigeninput.h"
#include "bufstringset.h"


uiSelectFromList::uiSelectFromList( uiParent* p,
				    const BufferStringSet& strs,
				    const char* cur, const char* captn )
	: uiDialog(p,Setup(captn))
	, selfld(0)
	, sel_(-1)
{
    const int sz = strs.size();
    const char** s = new const char* [sz];
    for ( int idx=0; idx<sz; idx++ )
	s[idx] = strs.get(idx).buf();
    init( s, sz, cur );
    delete [] s;
}


uiSelectFromList::uiSelectFromList( uiParent* p,
				    const char** strs, int sz,
				    const char* cur, const char* captn )
	: uiDialog(p,captn)
	, selfld(0)
	, sel_(-1)
{
    init( strs, sz, cur );
}


void uiSelectFromList::init( const char** strs, int nr, const char* cur )
{
    if ( nr == 0 || (nr < 0 && (!strs || !strs[0])) )
	{ new uiLabel(this,"No items available for selection"); return; }
    selfld = new uiListBox( this );
    for ( int idx=0; nr < 0 || idx<nr; idx++ )
    {
	if ( strs[idx] )
	    selfld->addItem(strs[idx]);
	else if ( nr < 0 )
	    break;
    }
    if ( cur && *cur ) selfld->setCurrentItem( cur );
    else	       selfld->setCurrentItem( 0 );

    selfld->doubleClicked.notify( mCB(this,uiDialog,accept) );
}


bool uiSelectFromList::acceptOK( CallBacker* )
{
    sel_ = selfld ? selfld->currentItem() : -1;
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
