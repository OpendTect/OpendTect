/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 2-8-1994
-*/

static const char* rcsID = "$Id: iodir.cc,v 1.16 2004-10-04 09:18:46 bert Exp $";

#include "iodir.h"
#include "iolink.h"
#include "ioman.h"
#include "ascstream.h"
#include "separstr.h"
#include "strmoper.h"
#include "errh.h"
#include "timefun.h"
#include "filegen.h"
#include "filepath.h"


IODir::IODir( const char* dirnm )
	: isok_(false)
	, dirname_(dirnm)
	, curid_(0)
{
    if ( build() ) isok_ = true;
}


IODir::IODir()
	: isok_(false)
	, curid_(0)
{
}


IODir::IODir( const MultiID& ky )
	: isok_(false)
	, curid_(0)
{
    IOObj* ioobj = getObj( ky );
    if ( !ioobj ) return;
    dirname_ = ioobj->dirName();
    delete ioobj;

    if ( build() ) isok_ = true;
}


IODir::~IODir()
{
    deepErase(objs_);
}


bool IODir::build()
{
    return doRead( dirname_, this ) ? true : false;
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
    FilePath fp( dirnm ); fp.add( ".omf" );
    BufferString omfname = fp.fullPath();
    bool found1 = false;
    if ( !File_isEmpty(omfname) )
    {
	IOObj* ret = readOmf( omfname, dirnm, dirptr, needid, found1 );
	if ( found1 )
	    return ret;
    }

    // Looks like something went wrong. Try the .omb ...
    if ( dirptr )
    {
	dirptr->setLinked(0);
	deepErase(dirptr->objs_);
    }
    fp.setFileName( ".omb" );
    IOObj* ret = readOmf( fp.fullPath(), dirnm, dirptr, needid, found1 );

    if ( !found1 )
    {
	// Last chance: maybe there's a .omf.new
	fp.setFileName( ".omf.new" );
	ret = readOmf( fp.fullPath(), dirnm, dirptr, needid, found1 );
    }
    return ret;
}


IOObj* IODir::readOmf( const char* omfname, const char* dirnm,
			IODir* dirptr, int needid, bool& found1 )
{
    found1 = false;
    std::istream* streamptr = openInputStream( omfname );
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

	found1 = true;
	MultiID ky( obj->key() );
	int id = ky.ID( ky.nrKeys()-1 );

	if ( dirptr )
	{
	    retobj = obj;
	    if ( id == 1 ) dirptr->setLinked( obj );
	    dirptr->addObj( obj, false );
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


const IOObj* IODir::operator[]( const char* ky ) const
{
    for ( int idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* ioobj = objs_[idx];
	if ( ioobj->name() == ky )
	    return ioobj;
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
    if ( !dirnm || !*dirnm || !mainobj ) return false;
    mainobj->key_ = ky;
    mainobj->key_ += getStringFromInt( 0, 1 );
    IODir dir;
    dir.dirname_ = dirnm;
    dir.key_ = ky;

    dir.objs_ += mainobj;
    dir.isok_ = true;
    bool ret = dir.doWrite();
    dir.objs_ -= mainobj;
    dir.isok_ = false;
    return ret;
}


void IODir::reRead()
{
    deepErase(objs_);
    curid_ = 0;
    if ( build() ) isok_ = true;
}


bool IODir::permRemove( const MultiID& ky )
{
    reRead();
    if ( bad() ) return false;

    int sz = objs_.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	IOObj* obj = objs_[idx];
	if ( obj->key() == ky )
	{
	    objs_ -= obj;
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
    if ( !clone ) return false;
    reRead();
    if ( bad() ) { delete clone; return false; }

    int sz = objs_.size();
    bool found = false;
    for ( int idx=0; idx<sz; idx++ )
    {
	IOObj* obj = objs_[idx];
	if ( obj->key() == clone->key() )
	{
	    delete objs_.replace( clone, idx );
	    found = true;
	    break;
	}
    }
    if ( found )
	return doWrite();
    else
    {
	pErrMsg("Attempt to commit changes to new object");
	delete clone;
	return false;
    }
}


bool IODir::addObj( IOObj* ioobj, bool persist )
{
    if ( persist )
    {
	reRead();
	if ( bad() ) return false;
    }
    if ( (*this)[ioobj->key()] )
	ioobj->setKey( newKey() );

    mkUniqueName( ioobj );
    objs_ += ioobj;
    return persist ? doWrite() : true;
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
	return true;
    }
    return false;
}


#define mAddDirWarn(msg) \
    msg += "\n-> Please check write permissions for directory:\n   "; \
    msg += dirname_


#define mCloseRetNo(strm) \
{ \
    BufferString msg( "\nError during write of Object Management info!" ); \
    mAddDirWarn(msg); \
    ErrMsg( msg ); \
    delete streamptr; \
    return false; \
}


bool IODir::wrOmf( const char* fnm ) const
{
    std::ostream* streamptr = openOutputStream( fnm );
    if ( !streamptr )
    {
	BufferString msg( "Cannot open a new file for Object Management info!");
	mAddDirWarn(msg);
	ErrMsg( msg );
	return false;
    }

    ascostream astream( *streamptr );
    if ( !astream.putHeader( "Object Management file" ) )
	mCloseRetNo(streamptr)
    FileMultiString fms( key_ == "" ? "0" : (const char*)key_ );
    fms += curid_;
    astream.put( "ID", fms );
    astream.newParagraph();

    // First the main obj
    const IOObj* mymain = main();
    if ( mymain && !mymain->put(astream) )
	mCloseRetNo(streamptr)

    // Then the links
    for ( int i=0; i<objs_.size(); i++ )
    {
	IOObj* obj = objs_[i];
	if ( obj == mymain ) continue;
	if ( obj->isLink() && !obj->put(astream) )
	    mCloseRetNo(streamptr)
    }
    // Then the normal objs
    for ( int i=0; i<objs_.size(); i++ )
    {
	IOObj* obj = objs_[i];
	if ( obj == mymain ) continue;
	if ( !obj->isLink() && !obj->put(astream) )
	    mCloseRetNo(streamptr)
    }

    delete streamptr;
    return true;
}


/*
   .omf writing must be _very_ safe. Therefore:

   1) Write the IODir data to file .omf.new

   2) Remove .omb

   3) Rename .omf to .omb

   4) rename .omf.new to .omf
*/

bool IODir::doWrite() const
{
    FilePath fp( dirname_ ); fp.add( ".omf" );
    const BufferString omfname( fp.fullPath() );
    fp.setFileName( ".omb" );
    const BufferString ombname( fp.fullPath() );
    fp.setFileName( ".omf.new" );
    const BufferString tmpomfname( fp.fullPath() );

    // Simple (but by no means secure) attempt to avoid concurrent update
    // problems. I'm afraid that making this a forcing lock will introduce
    // more pain than it solves.
    if ( File_exists(tmpomfname) ) Time_sleep( 0.25 );
    if ( File_exists(tmpomfname) ) Time_sleep( 0.25 );
    if ( File_exists(tmpomfname) ) Time_sleep( 0.25 );
    if ( File_exists(tmpomfname) ) Time_sleep( 0.25 );
    // A full second should be enough nowadays.
    // If there still is a .omf.new - we'll just assume it's a relict of
    // some kind of failed attempt.

    if ( !wrOmf(tmpomfname) )
    {
	File_remove( tmpomfname, NO );
	return false;
    }

    if ( !File_remove(ombname,NO) )
    {
	BufferString msg( "\nCannot remove '.omb' file.");
	mAddDirWarn(msg); ErrMsg( msg );
    }
    else if ( File_exists(omfname) && !File_rename(omfname,ombname) )
    {
	BufferString msg( "\nCannot rename '.omf' to '.omb'.");
	mAddDirWarn(msg); ErrMsg( msg );
    }

    if ( !File_rename(tmpomfname,omfname) )
    {
	File_remove( tmpomfname, NO );
	BufferString msg( "\nCannot rename '.omf.new' to '.omf'!\n"
			  "This means your changes will not be saved!");
	mAddDirWarn(msg); ErrMsg( msg );
	return false;
    }

    // Pff - glad it worked.
    return true;
}


MultiID IODir::newKey() const
{
    MultiID id = key_;
    ((IODir*)this)->curid_++;
    id += getStringFromInt( 0, curid_ );
    return id;
}
