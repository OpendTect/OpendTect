/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          July 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigmtoverlay.cc,v 1.3 2009/07/22 16:01:28 cvsbert Exp $";


#include "uigmtoverlay.h"
#include "uibutton.h"


const char* uiGMTOverlayGrp::sKeyProgName()	{ return "Program Name"; }
const char* uiGMTOverlayGrp::sKeyUserRef()	{ return "User Ref"; }


uiGMTOverlayGrp::uiGMTOverlayGrp( uiParent* p, const char* nm )
    : uiDlgGroup(p,nm)
{}


uiGMTOverlayGrpFactory& uiGMTOF()
{
    static PtrMan<uiGMTOverlayGrpFactory> inst = 0;
    if ( !inst )
	inst = new uiGMTOverlayGrpFactory;

    return *inst;
}


int uiGMTOverlayGrpFactory::add( const char* nm, uiGMTOverlayGrpCreateFunc fn )
{
    Entry* entry = getEntry( nm );
    if ( entry )
	entry->crfn_ = fn;
    else
    {
	entry = new Entry( nm, fn );
	entries_ += entry;
    }

    return entries_.size() - 1;
}


uiGMTOverlayGrp* uiGMTOverlayGrpFactory::create( uiParent* p,
						 const char* nm ) const
{
    Entry* entry = getEntry( nm );
    if ( !entry ) return 0;

    uiGMTOverlayGrp* grp = entry->crfn_( p );
    return grp;
}


const char* uiGMTOverlayGrpFactory::name( int idx ) const
{
    if ( idx < 0 || idx >= entries_.size() )
	return 0;

    return entries_[idx]->name_.buf();
}


uiGMTOverlayGrpFactory::Entry* uiGMTOverlayGrpFactory::getEntry(
							const char* nm ) const
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( entries_[idx]->name_ == nm )
	    return const_cast<uiGMTOverlayGrpFactory*>( this )->entries_[idx];
    }

    return 0;
}

