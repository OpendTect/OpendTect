/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 2-8-1994
-*/

static const char* rcsID = "$Id: iodir.cc,v 1.5 2001-02-13 17:21:02 bert Exp $";

#include "filegen.h"
#include "iodir.h"
#include "iolink.h"
#include "ioman.h"
#include "ascstream.h"
#include "separstr.h"
#include "aobset.h"
#include "strmoper.h"


IODir::IODir( const char* dirnm )
	: state_(Fail)
	, dirname_(dirnm)
	, curid_(0)
{
    if ( build() ) state_ = Ok;
}


IODir::IODir()
	: state_(Fail)
	, curid_(0)
{
}


IODir::IODir( const MultiID& ky )
	: state_(Fail)
	, curid_(0)
{
    IOObj* ioobj = getObj( ky );
    if ( !ioobj ) return;
    dirname_ = ioobj->dirName();
    delete ioobj;

    if ( build() ) state_ = Ok;
}


IODir::~IODir()
{
    objs_.deepErase();
}


bool IODir::build()
{
    return doRead( dirname_, this ) ? YES : NO;
}


IOObj* IODir::getMain( const char* dirnm )
{
    return doRead( dirnm, 0, 1 );
}


const IOObj* IODir::main() const
{
    for ( int idx=0; idx<objs_.size(); idx++ )
    {
	IOObj* ioobj = objs_[idx];
	if ( ioobj->myKey() == 1 ) return ioobj;
    }
    return 0;
}


IOObj* IODir::doRead( const char* dirnm, IODir* dirptr, int needid )
{
    FileNameString omfname = File_getFullPath(dirnm,".omf");
    bool found1 = NO;
    if ( !File_isEmpty(omfname) )
    {
	IOObj* ret = readOmf( omfname, dirnm, dirptr, needid, found1 );
	if ( found1 )
	    return ret;
    }

    // Looks like something went wrong. Read the backup OMF ...
    omfname = File_getFullPath(dirnm,".omb");
    if ( dirptr )
    {
	dirptr->setLinked(0);
	dirptr->objs_.deepErase();
    }
    return readOmf( omfname, dirnm, dirptr, needid, found1 );
}


IOObj* IODir::readOmf( const char* omfname, const char* dirnm,
			IODir* dirptr, int needid, bool& found1 )
{
    found1 = NO;
    istream* streamptr = openInputStream( omfname );
    if ( !streamptr )
	return 0;

    ascistream astream( *streamptr );
    astream.next();
    FileMultiString fms( astream.value() );
    MultiID dirky( fms[0] );
    if ( dirky == "0" ) dirky = "";
    if ( dirptr )
    {
	dirptr->key_ = dirky;
	dirptr->curid_ = atoi(fms[1]);
    }
    astream.next();

    IOObj* retobj = 0;
    while ( astream.type() != ascistream::EndOfFile )
    {
	IOObj* obj = IOObj::get(astream,dirnm,dirky);
	if ( !obj || obj->bad() ) { delete obj; continue; }

	found1 = YES;
	MultiID ky( obj->key() );
	int id = ky.ID( ky.nrKeys()-1 );

	if ( dirptr )
	{
	    if ( (*dirptr)[ky] )
	    {
		delete obj;
		continue;
	    }
	    retobj = obj;
	    if ( id == 1 ) dirptr->setLinked( obj );
	    dirptr->addObj( obj, NO );
	    if ( id < 100000 && id > dirptr->curid_ ) dirptr->curid_ = id;
	}
	else
	{
	    if ( id != needid )
		delete obj;
	    else
	    {
		retobj = obj;
		retobj->setStandAlone( dirnm );
		break;
	    }
	}
    }

    delete streamptr;
    return retobj;
}


IOObj* IODir::getObj( const MultiID& ky )
{
    FileNameString dirnm( IOM().rootDir() );
    int nrkeys = ky.nrKeys();
    for ( int idx=0; idx<nrkeys; idx++ )
    {
	int id = ky.ID( idx );
	IOObj* ioobj = doRead( dirnm, 0, id );
	if ( !ioobj || idx == nrkeys-1 ) return ioobj;
	dirnm = ioobj->dirName();
	delete ioobj;
    }

    return 0;
}


const IOObj* IODir::operator[]( const MultiID& ky ) const
{
    for ( int idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* ioobj = objs_[idx];
	if ( ioobj->key() == ky
	  || ( ioobj->isLink() && ((IOLink*)ioobj)->link()->key() == ky ) )
		return ioobj;
	
    }

    return 0;
}


bool IODir::create( const char* dirnm, const MultiID& ky, IOObj* mainobj )
{
    if ( !dirnm || !*dirnm || !mainobj ) return NO;
    mainobj->key_ = ky;
    mainobj->key_ += getStringFromInt( 0, 1 );
    IODir dir;
    dir.dirname_ = dirnm;
    dir.key_ = ky;

    dir.objs_ += mainobj;
    dir.state_ = Ok;
    bool ret = dir.doWrite();
    dir.objs_ -= mainobj;
    dir.state_ = Fail;
    return ret;
}


void IODir::reRead()
{
    objs_.deepErase();
    curid_ = 0;
    if ( build() ) state_ = Ok;
}


bool IODir::permRemove( const MultiID& ky )
{
    reRead();
    if ( bad() ) return NO;

    int sz = objs_.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	IOObj* obj = objs_[idx];
	if ( obj->key() == ky )
	{
	    *this -= obj;
	    delete obj;
	    break;
	}
    }
    return doWrite();
}


bool IODir::commitChanges( const IOObj* ioobj )
{
    if ( ioobj->isLink() )
    {
	IOObj* obj = (IOObj*)(*this)[ioobj->key()];
	if ( obj != ioobj ) obj->copyFrom( ioobj );
	return doWrite();
    }

    IOObj* clone = ioobj->clone();
    reRead();
    if ( bad() ) { delete clone; return NO; }

    int sz = objs_.size();
    int found = NO;
    for ( int idx=0; idx<sz; idx++ )
    {
	IOObj* obj = objs_[idx];
	if ( obj->key() == clone->key() )
	{
	    delete objs_.replace( clone, obj );
	    found = YES;
	    break;
	}
    }
    return found ? doWrite() : addObj( clone );
}


bool IODir::addObj( IOObj* ioobj, bool persist )
{
    if ( persist )
    {
	reRead();
	if ( bad() ) return NO;
    }
    if ( (*this)[ioobj->key()] )
	ioobj->setKey( newKey() );

    mkUniqueName( ioobj );
    *this += ioobj;
    return persist ? doWrite() : YES;
}


bool IODir::mkUniqueName( IOObj* ioobj )
{
    if ( (*this)[ioobj->name()] )
    {
	int nr = 1;
	UserIDString nm;

	do {
	    nr++;
	    nm = ioobj->name();
	    nm += " "; nm += nr;
	} while ( (*this)[nm] );

	ioobj->setName( nm );
	return YES;
    }
    return NO;
}

#define mCloseRetNo(strm) { delete streamptr; return NO; }

bool IODir::doWrite() const
{
    FileNameString ombname = File_getFullPath(dirname_,".omb");
    FileNameString omfname = File_getFullPath(dirname_,".omf");

    if ( File_isEmpty(omfname) )
    {
	if ( !File_isEmpty(ombname) )
	    File_copy( ombname, omfname, NO );
    }
    else
	File_copy( omfname, ombname, NO );

    ostream* streamptr = openOutputStream( omfname );
    if ( !streamptr ) return NO;

    ascostream astream( *streamptr );
    if ( !astream.putHeader( "Object Management file" ) ) mCloseRetNo(streamptr)
    astream.paddingOff();
    FileMultiString fms( key_ == "" ? "0" : (const char*)key_ );
    fms += curid_;
    astream.put( "ID", fms );
    astream.newParagraph();

    const IOObj* mymain = main();
    if ( mymain && !mymain->put(astream) ) mCloseRetNo(streamptr)

    for ( int i=0; i<objs_.size(); i++ )
    {
	IOObj* obj = objs_[i];
	if ( obj == mymain ) continue;
	if ( obj->isLink() && !obj->put(astream) ) mCloseRetNo(streamptr)
    }
    for ( int i=0; i<objs_.size(); i++ )
    {
	IOObj* obj = objs_[i];
	if ( obj == mymain ) continue;
	if ( !obj->isLink() && !obj->put(astream) ) mCloseRetNo(streamptr)
    }

    delete streamptr;
    return YES;
}


MultiID IODir::newKey() const
{
    MultiID id = key_;
    ((IODir*)this)->curid_++;
    id += getStringFromInt( 0, curid_ );
    return id;
}
