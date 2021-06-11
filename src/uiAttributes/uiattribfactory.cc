/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/

#include "uiattribfactory.h"
#include "uiattrdesced.h"
#include "ptrman.h"


uiAttributeFactory::~uiAttributeFactory()
{
    deepErase( entries_ );
}


uiAttributeFactory& uiAF()
{
    mDefineStaticLocalObject( PtrMan<uiAttributeFactory>, inst,
			      = new uiAttributeFactory );
    return *inst;
}


int uiAttributeFactory::add( const uiString& dispnm, const char* attrnm,
			 const uiString& grpnm, uiAttrDescEdCreateFunc fn,
			 int domtyp, int dimtyp, bool supportsynth, bool gd )
{
    Entry* entry = getEntry( attrnm );
    if ( !entry )
	entry = getEntry( dispnm );

    if ( entry )
    {
	entry->dispnm_ = dispnm;
	entry->attrnm_ = attrnm;
	entry->grpnm_ = grpnm;
	entry->crfn_ = fn;
	entry->domtyp_ = domtyp;
	entry->dimtyp_ = dimtyp;
	entry->supportsynthetic_ = supportsynth;
	entry->isgroupdef_ = gd;
    }
    else
    {
	entry = new Entry( dispnm, attrnm, grpnm, fn, domtyp, dimtyp,
			   supportsynth, gd );
	entries_ += entry;
    }

    return entries_.size() - 1;
}


bool uiAttributeFactory::enable( const char* attrnm, bool yn )
{
    for ( auto* entry : entries_ )
    {
	if ( entry->attrnm_ == attrnm )
	{
	    entry->enabled_ = yn;
	    return true;
	}
    }

    return false;
}


bool uiAttributeFactory::remove( const char* attrnm )
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( entries_[idx]->attrnm_ == attrnm )
	{
	    delete entries_.removeSingle(idx);
	    return true;
	}
    }

    return false;
}


uiAttrDescEd* uiAttributeFactory::create( uiParent* p, const uiString& nm,
					  bool is2d ) const
{
    Entry* entry = getEntry( nm );
    return entry ? entry->crfn_( p, is2d ) : 0;
}


uiAttrDescEd* uiAttributeFactory::create( uiParent* p, const char* nm,
					  bool is2d ) const
{
    Entry* entry = getEntry( nm );
    return entry ? entry->crfn_( p, is2d ) : 0;
}


int uiAttributeFactory::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( entries_[idx]->attrnm_ == nm )
	    return idx;
    }
    return -1;
}


int uiAttributeFactory::indexOf( const uiString& nm ) const
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( entries_[idx]->dispnm_ == nm )
	    return idx;
    }
    return -1;
}


uiAttributeFactory::Entry* uiAttributeFactory::getEntry( const char* nm ) const
{
    const int idx = indexOf( nm );
    return idx < 0 ? 0 : const_cast<uiAttributeFactory*>( this )->entries_[idx];
}


uiAttributeFactory::Entry* uiAttributeFactory::getEntry(
					const uiString& nm ) const
{
    const int idx = indexOf( nm );
    return idx < 0 ? 0 : const_cast<uiAttributeFactory*>( this )->entries_[idx];
}


const uiString& uiAttributeFactory::dispNameOf( const char* attrnm ) const
{
    Entry* entry = getEntry( attrnm );
    return entry ? entry->dispnm_ : uiString::empty();
}


const char* uiAttributeFactory::attrNameOf( const uiString& attrnm ) const
{
    const Entry* entry = getEntry( attrnm );
    return entry ? entry->attrnm_.str() : 0;
}


bool uiAttributeFactory::isEnabled( const char* attrnm ) const
{
    const Entry* entry = getEntry( attrnm );
    return entry ? entry->enabled_ : false;
}


bool uiAttributeFactory::hasSteering() const
{
    return isEnabled( "Curvature" );
}


#define mRetInfo(memb,notvalidval) \
    return entries_.validIdx(idx) ? entries_[idx]->memb : notvalidval

#define mDefInfoFn(rettyp,fnnm,memb,notvalidval) \
    rettyp uiAttributeFactory::fnnm( int idx ) const \
    { \
	return entries_.validIdx(idx) ? entries_[idx]->memb : notvalidval; \
    }

// Bert: changed 'attrnm_' to 'attrnm_.str()' because the compiler created
// a new BufferString and returned the const char* of that temporary object.
// compiler bug?
mDefInfoFn( const char*, getAttribName, attrnm_.str(), 0 )
mDefInfoFn( const uiString&, getDisplayName, dispnm_, uiString::empty() )
mDefInfoFn( const uiString&, getGroupName, grpnm_, uiString::empty() )
mDefInfoFn( int, domainType, domtyp_, 0 )
mDefInfoFn( int, dimensionType, dimtyp_, 0 )
mDefInfoFn( bool, isSyntheticSupported, supportsynthetic_, false )
mDefInfoFn( bool, isGroupDef, isgroupdef_, false )
