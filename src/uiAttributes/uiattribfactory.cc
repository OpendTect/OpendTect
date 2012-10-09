/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uiattribfactory.h"
#include "uiattrdesced.h"
#include "ptrman.h"


uiAttributeFactory::~uiAttributeFactory()
{
    deepErase( entries_ );
}


uiAttributeFactory& uiAF()
{
    static PtrMan<uiAttributeFactory> inst = 0;
    if ( !inst )
    {
	inst = new uiAttributeFactory;
    }
    return *inst;
}


int uiAttributeFactory::add( const char* dispnm, const char* attrnm,
			     const char* grpnm, uiAttrDescEdCreateFunc fn,
       			     int domtyp, int dimtyp )
{
    Entry* entry = getEntry( dispnm, true );
    if ( !entry )
	entry = getEntry( attrnm, false );

    if ( entry )
    {
	entry->dispnm_ = dispnm;
	entry->attrnm_ = attrnm;
	entry->grpnm_ = grpnm;
	entry->crfn_ = fn;
	entry->domtyp_ = domtyp;
	entry->dimtyp_ = dimtyp;
    }
    else
    {
	entry = new Entry( dispnm, attrnm, grpnm, fn, domtyp, dimtyp );
	entries_ += entry;
    }

    return entries_.size() - 1;
}


uiAttrDescEd* uiAttributeFactory::create( uiParent* p, const char* nm,
					  bool is2d, bool isdisp ) const
{
    Entry* entry = getEntry( nm, isdisp );
    if ( !entry ) return 0;

    uiAttrDescEd* ed = entry->crfn_( p, is2d );
    if ( ed )
    {
	ed->setDisplayName( entry->dispnm_ );
	ed->setDomainType( (uiAttrDescEd::DomainType)entry->domtyp_ );
	ed->setDimensionType( (uiAttrDescEd::DimensionType)entry->dimtyp_ );
    }
    return ed;
}


uiAttributeFactory::Entry* uiAttributeFactory::getEntry( const char* nm,
							 bool isdisp ) const
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( (isdisp && entries_[idx]->dispnm_ == nm)
	  || (!isdisp && entries_[idx]->attrnm_ == nm) )
	    return const_cast<uiAttributeFactory*>( this )->entries_[idx];
    }

    return 0;
}


const char* uiAttributeFactory::dispNameOf( const char* attrnm ) const
{
    Entry* entry = getEntry( attrnm, false );
    return entry ? ((const char*)entry->dispnm_) : 0;
}


const char* uiAttributeFactory::attrNameOf( const char* attrnm ) const
{
    const Entry* entry = getEntry( attrnm, true );
    return entry ? ((const char*)entry->attrnm_) : 0;
}

