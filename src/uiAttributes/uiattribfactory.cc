/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:		$Id: uiattribfactory.cc,v 1.1 2006-09-11 06:53:42 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattribfactory.h"
#include "uiattrdesced.h"
#include "ptrman.h"


void uiAttributeFactory::add( const char* displaynm, CreateFunc fc )
{
    if ( displaynames_.indexOf(displaynm) != -1 )
	return;

    funcs_ += fc;
    displaynames_.add( displaynm );
}


uiAttrDescEd* uiAttributeFactory::create( uiParent* p, const char* nm )
{
    const int idx = displaynames_.indexOf( nm );
    uiAttrDescEd* ed = funcs_.validIdx(idx) ? funcs_[idx](p) : 0;
    if ( ed ) ed->setDisplayName( nm );
    return ed;
}


int uiAttributeFactory::size() const
{ return displaynames_.size(); }

const char* uiAttributeFactory::getDisplayName( int idx ) const
{ return displaynames_.validIdx(idx) ? displaynames_.get( idx ).buf() : 0; }


bool uiAttributeFactory::hasAttribute( const char* dispnm ) const
{
    return displaynames_.indexOf( dispnm ) != -1;
}


uiAttributeFactory& uiAF()
{
    static PtrMan<uiAttributeFactory> inst = 0;
    if ( !inst ) inst = new uiAttributeFactory;
    return *inst;
}
