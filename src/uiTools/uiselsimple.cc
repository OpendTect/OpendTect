/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2001
 RCS:           $Id: uiselsimple.cc,v 1.6 2003-11-07 12:22:02 bert Exp $
________________________________________________________________________

-*/

#include "uilabel.h"
#include "uilistbox.h"
#include "uiselsimple.h"
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
