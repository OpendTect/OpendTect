/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "iodirentry.h"
#include "ctxtioobj.h"
#include "transl.h"
#include "iodir.h"
#include "ioman.h"
#include "iopar.h"
#include "globexpr.h"
#include "filepath.h"


// IODirEntry
IODirEntry::IODirEntry( const IOObj* iob )
    : NamedObject("")
    , ioobj_(iob)
{
    setName( ioobj_ ? ioobj_->name().str() : ".." );
}


IODirEntry::~IODirEntry()
{}



// IODirEntryList

IODirEntryList::IODirEntryList( const IODir& id, const TranslatorGroup* tr,
				bool maycd, const char* f )
    : ctxt(*new IOObjContext(tr))
    , cur_(-1)
    , maycd_(maycd)
{
    ctxt.toselect_.allowtransls_ = f;
    fill( id );
}


IODirEntryList::IODirEntryList( const IODir& id, const IOObjContext& ct )
    : ctxt(*new IOObjContext(ct))
    , cur_(-1)
    , maycd_(false)
{
    fill( id );
}


IODirEntryList::~IODirEntryList()
{
    deepErase(*this);
    delete &ctxt;
}


void IODirEntryList::fill( const IODir& iodir, const char* nmfilt )
{
    if ( iodir.isBad() )
    {
	pErrMsg("Bad iodir" );
	return;
    }

    deepErase(*this);
    name_ = iodir.main() ? (const char*)iodir.main()->name() : "Objects";
    const ObjectSet<IOObj>& ioobjs = iodir.getObjs();

    int curset = 0;
    if ( maycd_ && FilePath(iodir.dirName()) !=
		   FilePath(IOM().rootDir().fullPath()) )
    {
	*this += new IODirEntry( nullptr );
	curset++;
    }

    GlobExpr* ge = nmfilt && *nmfilt ? new GlobExpr(nmfilt) : nullptr;

    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj* ioobj = ioobjs[idx];
	if ( ioobj->isTmp() )
	    continue;

	if ( ctxt.trgroup_ )
	{
	    const int selres = ctxt.trgroup_->objSelector( ioobj->group() );
	    if ( selres == mObjSelUnrelated )
		continue;
	}

	if ( ctxt.validIOObj(*ioobj) )
	{
	    if ( !ge || ge->matches(ioobj->name()) )
		*this += new IODirEntry( ioobj );
	}
    }

    delete ge;
    sort();
    if ( lastiokey.isUdf() )
	{ if ( size() > curset ) setCurrent( curset ); }
    else
	setSelected( lastiokey );
}


void IODirEntryList::setSelected( const MultiID& iniokey )
{
    bool matches = false;
    for ( int idx=0; idx<size(); idx++ )
    {
	IODirEntry* entry = (*this)[idx];
	MultiID iokey( iniokey );
	if ( !entry->ioobj_ )
	{
	    if ( iokey.isUdf() )
		matches = true;
	}
	else
	{
	    if ( iokey == entry->ioobj_->key() )
		matches = true;
	    else
	    {
		while ( 1 )
		{
		    iokey = iokey.mainID();
		    if ( iokey.isUdf() )
			break;

		    if ( iokey == entry->ioobj_->key() )
			matches = true;
		}
	    }
	}
	if ( matches )
	{
	    setCurrent( idx );
	    lastiokey = iokey;
	    return;
	}
    }
}


void IODirEntryList::removeWithTranslator( const char* trnm )
{
    BufferString nm = trnm;
    for ( int idx=0; idx<size(); idx++ )
    {
	IODirEntry* entry = (*this)[idx];
	if ( entry->ioobj_ && nm == entry->ioobj_->translator() )
	{
	    if ( idx == cur_ )
		cur_--;
	    *this -= entry;
	    idx--;
	}
    }
    if ( cur_ < 0 ) cur_ = size() - 1;
}


void IODirEntryList::sort()
{
    BufferStringSet nms; const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
	nms.add( (*this)[idx]->name() );

    ConstArrPtrMan<int> idxs = nms.getSortIndexes();
    ObjectSet<IODirEntry> tmp( *this );
    erase();
    for ( int idx=0; idx<sz; idx++ )
	*this += tmp[ idxs[idx] ];
}


int IODirEntryList::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const IODirEntry* entry = (*this)[idx];
	if ( entry->name() == nm )
	    return idx;
    }
    return -1;
}


void IODirEntryList::getIOObjNames( BufferStringSet& nms ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const IOObj* ioobj = (*this)[idx]->ioobj_;
	if ( ioobj )
	    nms.add( ioobj->name() );
    }
}


BufferStringSet IODirEntryList::getValuesFor( const char* key ) const
{
    BufferStringSet res;
    for ( idx_type idx=0; idx<size(); idx++ )
    {
	const IOObj* ioobj = (*this)[idx]->ioobj_;
	if ( ioobj )
	{
	    BufferString val;
	    if ( ioobj->group() == key )
		val = ioobj->translator();
	    else if ( !ioobj->pars().get( key, val ) )
		continue;

	    if ( !val.isEmpty() )
		res.addIfNew( val );
	}
    }
    return res;
}
