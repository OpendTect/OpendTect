/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Dec 2001
 RCS:           $Id: uiselsimple.cc,v 1.3 2003-04-29 15:45:53 bert Exp $
________________________________________________________________________

-*/

#include "uiselsimple.h"
#include "uilistbox.h"
#include "uilabel.h"
#include "uidobjset.h"


uiSelectFromList::uiSelectFromList( uiParent* p,
				    const ObjectSet<BufferString>& strs,
				    const char* cur, const char* captn )
	: uiDialog(p,Setup(captn))
	, selfld(0)
	, sel_(-1)
{
    const int sz = strs.size();
    const char** s = new const char* [sz];
    for ( int idx=0; idx<sz; idx++ )
	s[idx] = (const char*)(*strs[idx]);
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


uiSelectFromList::uiSelectFromList( uiParent* p,
				    const PtrUserIDObjectSet& strs,
				    const char* cur, const char* captn )
	: uiDialog(p,captn)
	, selfld(0)
	, sel_(-1)
{
    const int sz = strs.size();
    const char** s = new const char* [sz];
    for ( int idx=0; idx<sz; idx++ )
	s[idx] = strs[idx]->name();
    init( s, sz, cur );
    delete [] s;
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

    selfld->doubleClicked.notify( mCB(this,uiSelectFromList,acceptOK) );
}


bool uiSelectFromList::acceptOK( CallBacker* )
{
    sel_ = selfld ? selfld->currentItem() : -1;
    return true;
}
