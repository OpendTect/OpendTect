/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : April 2001
 * FUNCTION : IODir entries for selectors

-*/


#include "iodirentry.h"
#include "ioobjctxt.h"
#include "transl.h"
#include "iopar.h"
#include "strmprov.h"
#include "globexpr.h"



IODirEntryList::IODirEntryList( const IOObjContext& ct )
    : ctxt_(*new IOObjContext(ct))
{
}


IODirEntryList::IODirEntryList( const IODir& id, const IOObjContext& ct )
    : ctxt_(*new IOObjContext(ct))
{
    fill( id );
}


IODirEntryList::IODirEntryList( const IODir& id, const TranslatorGroup* tr,
				const char* allowedtransls )
    : ctxt_(*new IOObjContext(tr))
{
    ctxt_.toselect_.allowtransls_ = allowedtransls;
    fill( id );
}


IODirEntryList::~IODirEntryList()
{
    deepErase( entries_ );
    delete &ctxt_;
}


void IODirEntryList::fill( const IODir& iodir, const char* nmfilt )
{
    if ( iodir.isBad() )
	{ pErrMsg("Bad IODir" ); return; }

    deepErase( entries_ );
    name_ = iodir.name();
    GlobExpr* ge = nmfilt && *nmfilt ? new GlobExpr(nmfilt) : 0;

    IODirIter iter( iodir );
    while ( iter.next() )
    {
	const IOObj& obj = iter.ioObj();
	if ( !obj.isTmp() && ctxt_.validIOObj(obj) )
	{
	    if ( !ge || ge->matches(obj.name()) )
		entries_ += obj.clone();
	}
    }

    delete ge;
    sort();
}


DBKey IODirEntryList::key( idx_type idx ) const
{
    if ( !entries_.validIdx(idx) )
	return DBKey::getInvalid();
    return entries_[idx]->key();
}


BufferString IODirEntryList::name( idx_type idx ) const
{
    if ( !entries_.validIdx(idx) )
	return BufferString::empty();
    return entries_[idx]->name();
}


BufferString IODirEntryList::dispName( idx_type idx ) const
{
    if ( !entries_.validIdx(idx) )
	return BufferString::empty();

    const IOObj& obj = *entries_[idx];
    const DBKey dbky = entries_[idx]->key();
    const BufferString nm( obj.name() );
    if ( IOObj::isSurveyDefault(dbky) )
	return BufferString( "> ", nm, " <" );
    else if ( StreamProvider::isPreLoaded(dbky.toString(),true) )
	return BufferString( "/ ", nm, " \\" );
    return nm;
}


BufferString IODirEntryList::iconName( idx_type idx ) const
{
    if ( entries_.validIdx(idx) )
    {
	const IOObj& obj = *entries_[idx];
	PtrMan<Translator> transl = obj.createTranslator();
	if ( transl )
	    return BufferString( transl->iconName() );
    }

    return BufferString::empty();
}


void IODirEntryList::sort()
{
    BufferStringSet nms; const size_type sz = size();
    for ( idx_type idx=0; idx<sz; idx++ )
	nms.add( entries_[idx]->name() );

    idx_type* idxs = nms.getSortIndexes();

    ObjectSet<IOObj> tmp( entries_ );
    entries_.erase();
    for ( idx_type idx=0; idx<sz; idx++ )
	entries_ += tmp[ idxs[idx] ];
    delete [] idxs;
}


IODirEntryList::idx_type IODirEntryList::indexOf( const char* nm ) const
{
    for ( idx_type idx=0; idx<size(); idx++ )
    {
	const IOObj& entry = *entries_[idx];
	if ( entry.hasName(nm) )
	    return idx;
    }
    return -1;
}
