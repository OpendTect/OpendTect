/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : April 2001
 * FUNCTION : IODir entries for selectors

-*/
 
static const char* rcsID = "$Id: iodirentry.cc,v 1.13 2004-11-12 09:13:07 nanne Exp $";

#include "iodirentry.h"
#include "ctxtioobj.h"
#include "transl.h"
#include "iodir.h"
#include "ioman.h"
#include "iopar.h"
#include "globexpr.h"
#include "filepath.h"

#include "errh.h"

bool IODirEntry::beingsorted = false;


IODirEntry::IODirEntry( IOObj* iob, int selres, bool maychgdir )
    : UserIDObject("")
    , ioobj(iob)
{
    if ( !maychgdir )
	*name_ = " ";
    else
    {
	if ( !ioobj->isLink() )
	    *name_ = "  ";
	else if ( selres == 1 )
	    *name_ = "->"; // Link, must go there to get usable
	else
	    *name_ = "> "; // Link but also directly usable
    }
    if ( ioobj )
	*name_ += ioobj->name();
    else
	*name_ = "..";
}


const UserIDString& IODirEntry::name() const
{
    return beingsorted && ioobj ? ioobj->name() : *name_;
}


IODirEntryList::IODirEntryList( IODir* id, const TranslatorGroup* tr,
				bool maycd, const char* f )
    : UserIDObject(id && id->main() ? (const char*)id->main()->name()
	    			    : "Objects")
    , ctxt(*new IOObjContext(tr))
    , cur_(-1)
{
    ctxt.maychdir = maycd;
    ctxt.trglobexpr = f;
    ctxt.includekeyval = false;
    fill( id );
}


IODirEntryList::IODirEntryList( IODir* id, const IOObjContext& ct )
    : UserIDObject(id && id->main()
		? (const char*)id->main()->name():"Objects")
    , ctxt(*new IOObjContext(ct))
    , cur_(-1)
{
    fill( id );
}


IODirEntryList::~IODirEntryList()
{
    deepErase(*this);
    delete &ctxt;
}


void IODirEntryList::fill( IODir* iodir )
{
    if ( !iodir ) { pErrMsg("Can't fill IODirEntryList. No iodir"); return; }
    deepErase(*this);
    setName( iodir->main() ? (const char*)iodir->main()->name() : "Objects" );
    const ObjectSet<IOObj>& ioobjs = iodir->getObjs();
    int curset = 0;
    if ( ctxt.maychdir
	&& FilePath(iodir->dirName()) != FilePath(IOM().rootDir()) )
    {
        *this += new IODirEntry( 0, 0, false );
	curset++;
    }

    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	IOObj* ioobj = ioobjs[idx];

	int selres = 2;
	if ( ctxt.trgroup )
	{
	    selres = ctxt.trgroup->objSelector( ioobj->group() );
	    if ( selres == mObjSelUnrelated
	      || (selres == mObjSelRelated && !ioobj->isLink()) )
		continue;
	}
	if ( selres == mObjSelRelated || ctxt.validIOObj(*ioobj) )
	    *this += new IODirEntry( ioobj, selres, ctxt.maychdir );
    }

    sort();
    if ( lastiokey == "" )
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
	    if ( iokey == "" )
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
		    if ( iokey == "" ) break;
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


void IODirEntryList::curRemoved()
{
    IODirEntry* cur = current();
    if ( !cur ) return;
    *this -= cur;
    delete cur;
    if ( cur_ >= size() ) cur_ = size() - 1;
    cur = current();
    lastiokey = cur && cur->ioobj ? (const char*)cur->ioobj->key() : "";
}


void IODirEntryList::removeWithTranslator( const char* trnm )
{
    BufferString nm = trnm;
    int curidx = 0;
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


bool IODirEntryList::mustChDir()
{
    if ( !current() ) return false;
    char ch1 = *(const char*)current()->name();
    return ch1 == '-' || ch1 == '.';
}


bool IODirEntryList::canChDir()
{
    if ( !current() ) return false;
    char ch1 = *(const char*)current()->name();
    return ch1 != ' ';
}


void IODirEntryList::sort()
{
    IODirEntry::beingsorted = true;
    const int sz = size();
    for ( int d=sz/2; d>0; d=d/2 )
	for ( int i=d; i<sz; i++ )
	    for ( int j=i-d;
		  j>=0 && (*this)[j]->name() > (*this)[j+d]->name();
		  j-=d )
		replace( replace( (*this)[j+d], j ), j+d );
    IODirEntry::beingsorted = false;
}


int IODirEntryList::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	IODirEntry* entry = (*this)[idx];
	if ( entry->name() == nm )
	    return idx;
    }
    return -1;
}
