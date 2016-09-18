/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : April 2001
 * FUNCTION : IODir entries for selectors

-*/


#include "iodirentry.h"
#include "ioobjctxt.h"
#include "transl.h"
#include "iodir.h"
#include "ioman.h"
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
	const IOObj& ioobj = iter.ioObj();
	if ( !ioobj.isTmp() && ctxt_.validIOObj(ioobj) )
	{
	    if ( !ge || ge->matches(ioobj.name()) )
		entries_ += ioobj.clone();
	}
    }

    delete ge;
    sort();
}


DBKey IODirEntryList::key( IdxType idx ) const
{
    if ( !entries_.validIdx(idx) )
	return DBKey::getInvalid();
    return entries_[idx]->key();
}


BufferString IODirEntryList::name( IdxType idx ) const
{
    if ( !entries_.validIdx(idx) )
	return BufferString::empty();
    return entries_[idx]->name();
}


BufferString IODirEntryList::dispName( IdxType idx ) const
{
    if ( !entries_.validIdx(idx) )
	return BufferString::empty();

    const IOObj& ioobj = *entries_[idx];
    const DBKey dbky = entries_[idx]->key();
    const BufferString nm( ioobj.name() );
    if ( IOObj::isSurveyDefault(dbky) )
	return BufferString( "> ", nm, " <" );
    else if ( StreamProvider::isPreLoaded(dbky.toString(),true) )
	return BufferString( "/ ", nm, " \\" );
    return nm;
}


BufferString IODirEntryList::iconName( IdxType idx ) const
{
    if ( entries_.validIdx(idx) )
    {
	const IOObj& ioobj = *entries_[idx];
	PtrMan<Translator> transl = ioobj.createTranslator();
	if ( transl )
	    return BufferString( transl->iconName() );
    }

    return BufferString::empty();
}


void IODirEntryList::sort()
{
    BufferStringSet nms; const size_type sz = size();
    for ( IdxType idx=0; idx<sz; idx++ )
	nms.add( entries_[idx]->name() );

    IdxType* idxs = nms.getSortIndexes();

    ObjectSet<IOObj> tmp( entries_ );
    entries_.erase();
    for ( IdxType idx=0; idx<sz; idx++ )
	entries_ += tmp[ idxs[idx] ];
    delete [] idxs;
}


IODirEntryList::IdxType IODirEntryList::indexOf( const char* nm ) const
{
    for ( IdxType idx=0; idx<size(); idx++ )
    {
	const IOObj& entry = *entries_[idx];
	if ( entry.name() == nm )
	    return idx;
    }
    return -1;
}
