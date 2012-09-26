/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          November 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "attribdescsetman.h"
#include "attribdescset.h"
#include "attribdesc.h"
#include "iopar.h"
#include "keystrs.h"

namespace Attrib
{

DescSetMan::DescSetMan( bool is2d, DescSet* ads, bool destr )
    : inpselhist_(*new IOPar( "Input Attribute history" ))
    , steerselhist_(*new IOPar( "Steering selection history" ))
    , unsaved_(false)
    , ads_(ads)
    , is2d_(is2d)
    , attrsetid_("") 
    , destrondel_(destr)
{
    if ( !ads_ )
	ads_ = new DescSet(is2d_);
    else
	fillHist();
}


DescSetMan::~DescSetMan()
{
    if ( destrondel_ ) delete ads_;
    delete &inpselhist_;
    delete &steerselhist_;
}


void DescSetMan::setDescSet( DescSet* newads )
{
    if ( newads == ads_ )
	return;
    if ( !ads_ )
	{ ads_ = newads; return; }
    if ( !newads )
    	{ inpselhist_.setEmpty(); ads_ = newads; return; }

    // Remove invalid entries
    fillHist();
    cleanHist( inpselhist_, *newads );
    cleanHist( steerselhist_, *newads );

    ads_ = newads;
}


void DescSetMan::cleanHist( IOPar& selhist, const DescSet& newads )
{
    for ( int ikey=0; ikey<selhist.size(); ikey++ )
    {
	const int id = toInt( selhist.getValue(ikey) );
	if ( id < 0 ) continue;

	const Desc* desc = ads_->getDesc( DescID(id,false) );
	bool keep = false;
	if ( desc )
	{
	    if ( newads.getID(desc->userRef(),true).asInt() >= 0 )
		keep = true;
	}

	if ( !keep )
	{
	    selhist.remove( ikey );
	    ikey--;
	}
    }
}


void DescSetMan::fillHist()
{
    inpselhist_.setEmpty();

    // First add one empty
    inpselhist_.set( IOPar::compKey(sKey::IOSelection(),1), -1 );

    int nr = 1;
    TypeSet<DescID> attribids;
    ads_->getIds( attribids );
    for ( int idx=0; idx<attribids.size(); idx++ )
    {
	Desc* ad = ads_->getDesc( attribids[idx] );
	if ( !ad || ad->isHidden() || ad->isStored() ) continue;

	const BufferString key( "", attribids[idx].asInt() );
	if ( inpselhist_.findKeyFor(key) ) continue;

	nr++;
	inpselhist_.set( IOPar::compKey(sKey::IOSelection(),nr), key );
    }
}

}; // namespace Attrib
