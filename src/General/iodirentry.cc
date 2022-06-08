/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : April 2001
 * FUNCTION : IODir entries for selectors

-*/


#include "iodirentry.h"
#include "ctxtioobj.h"
#include "transl.h"
#include "iodir.h"
#include "ioman.h"
#include "iopar.h"
#include "globexpr.h"
#include "filepath.h"



IODirEntry::IODirEntry( const IOObj* iob )
    : NamedObject("")
    , ioobj_(iob)
{
    setName( ioobj_ ? ioobj_->name().str() : ".." );
}


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
    if ( maycd_ && FilePath(iodir.dirName()) != FilePath(IOM().rootDir()) )
    {
        *this += new IODirEntry( 0 );
	curset++;
    }

    GlobExpr* ge = nmfilt && *nmfilt ? new GlobExpr(nmfilt) : 0;

    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj* ioobj = ioobjs[idx];
	if ( ioobj->isTmp() )
	    continue;

	int selres = 2;
	if ( ctxt.trgroup_ )
	{
	    selres = ctxt.trgroup_->objSelector( ioobj->group() );
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
    int* idxs = nms.getSortIndexes();

    ObjectSet<IODirEntry> tmp( *this );
    erase();
    for ( int idx=0; idx<sz; idx++ )
	*this += tmp[ idxs[idx] ];
    delete [] idxs;
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
