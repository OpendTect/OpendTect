/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          July 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: gmtpar.cc,v 1.5 2009-07-22 16:01:26 cvsbert Exp $";


#include "gmtpar.h"

#include "debug.h"
#include "keystrs.h"


DefineNameSpaceEnumNames(ODGMT,Shape,3,"Shapes")
{ "Star", "Circle", "Diamond", "Square", "Triangle", "Cross", "Polygon",
  "Line", 0 };


GMTParFactory& GMTPF()
{
    static PtrMan<GMTParFactory> inst = 0;
    if ( !inst )
	inst = new GMTParFactory;

    return *inst;
}


int GMTParFactory::add( const char* nm, GMTParCreateFunc fn )
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


GMTPar* GMTParFactory::create( const IOPar& iop ) const
{
    const char* grpname = iop.find( ODGMT::sKeyGroupName );
    if ( !grpname || !*grpname ) return 0;

    Entry* entry = getEntry( grpname );
    if ( !entry ) return 0;

    GMTPar* grp = entry->crfn_( iop );
    return grp;
}


const char* GMTParFactory::name( int idx ) const
{
    if ( idx < 0 || idx >= entries_.size() )
	return 0;

    return entries_[idx]->name_.buf();
}


GMTParFactory::Entry* GMTParFactory::getEntry( const char* nm ) const
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( entries_[idx]->name_ == nm )
	    return const_cast<GMTParFactory*>( this )->entries_[idx];
    }

    return 0;
}


BufferString GMTPar::fileName( const char* fnm ) const
{
    BufferString fnmchg;
    if ( __iswin__ ) fnmchg += "\"";
    fnmchg += fnm;
    if ( __iswin__ ) fnmchg += "\"";
    return fnmchg;
}


bool GMTPar::execCmd( const BufferString& comm )
{
    DBG::message( comm );
    return system( comm ) != -1;
}
