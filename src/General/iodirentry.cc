/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : April 2001
 * FUNCTION : IODir entries for selectors

-*/
 
static const char* rcsID mUnusedVar = "$Id$";

#include "iodirentry.h"
#include "ctxtioobj.h"
#include "transl.h"
#include "iodir.h"
#include "ioman.h"
#include "iopar.h"
#include "globexpr.h"
#include "filepath.h"

#include "errh.h"


IODirEntry::IODirEntry( IOObj* iob )
    : NamedObject("")
    , ioobj(iob)
{
    setName( ioobj ? ioobj->name() : ".." );
}


IODirEntryList::IODirEntryList( IODir* id, const TranslatorGroup* tr,
				bool maycd, const char* f )
    : ctxt(*new IOObjContext(tr))
    , cur_(-1)
    , maycd_(maycd)
{
    ctxt.toselect.allowtransls_ = f;
    fill( id );
}


IODirEntryList::IODirEntryList( IODir* id, const IOObjContext& ct )
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


void IODirEntryList::fill( IODir* iodir, const char* nmfilt )
{
    if ( !iodir ) { pErrMsg("Can't fill IODirEntryList. No iodir"); return; }

    deepErase(*this);
    name_ = iodir->main() ? (const char*)iodir->main()->name() : "Objects";
    const ObjectSet<IOObj>& ioobjs = iodir->getObjs();

    int curset = 0;
    if ( maycd_ && FilePath(iodir->dirName()) != FilePath(IOM().rootDir()) )
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
	if ( ctxt.trgroup )
	{
	    selres = ctxt.trgroup->objSelector( ioobj->group() );
	    if ( selres == mObjSelUnrelated )
		continue;
	}
	if ( ctxt.validIOObj(*ioobj) )
	{
	    if ( !ge || ge->matches(ioobj->name()) )
		*this += new IODirEntry( const_cast<IOObj*>(ioobj) );
	}
    }

    delete ge;
    sort();
    if ( lastiokey.isEmpty() )
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
	if ( !entry->ioobj )
	{
	    if ( iokey.isEmpty() )
		matches = true;
	}
	else
	{
	    if ( iokey == entry->ioobj->key() )
		matches = true;
	    else
	    {
		while ( 1 )
		{
		    iokey = iokey.upLevel();
		    if ( iokey.isEmpty() ) break;
		    if ( iokey == entry->ioobj->key() )
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
	if ( entry->ioobj && nm == entry->ioobj->translator() )
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
