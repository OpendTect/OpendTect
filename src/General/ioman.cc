/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 3-8-1994
-*/

static const char* rcsID = "$Id: ioman.cc,v 1.5 2001-02-19 11:27:11 bert Exp $";

#include "ioman.h"
#include "iodir.h"
#include "ioparlist.h"
#include "iolink.h"
#include "iostrm.h"
#include "transl.h"
#include "ctxtioobj.h"
#include "filegen.h"
#include "errh.h"
#include <stdlib.h>
#include <fstream.h>

IOMan*	IOMan::theinst_	= 0;
void	IOMan::stop()	{ delete theinst_; theinst_ = 0; }
extern "C" void SetSurveyName(const char*);


bool IOMan::newSurvey()
{
    delete IOMan::theinst_;
    IOMan::theinst_ = 0;
    return !IOM().bad();
}


void IOMan::setSurvey( const char* survname )
{
    delete IOMan::theinst_;
    IOMan::theinst_ = 0;
    SetSurveyName( survname );
}


IOMan::IOMan()
	: UserIDObject("IO Manager")
	, dirptr(0)
	, state_(IOMan::NeedInit)
{
    rootdir = GetDataDir();
    if ( !File_isDirectory(rootdir) )
	rootdir = getenv( "dGB_DATA" );
}


void IOMan::init()
{
    state_ = Bad;
    if ( !to( prevkey ) ) return;

    state_ = Good;
    curlvl = 0;

    if ( dirPtr()->key() == MultiID("-1") ) return;

    if ( ! (*dirPtr())[MultiID(Translator::sMiscID)] )
    {
	FileNameString dirnm = File_getFullPath( rootdir, "Misc" );
	if ( !File_exists(dirnm) )
	{
	    FileNameString basicdirnm = GetDataFileName( "BasicSurvey" );
	    basicdirnm = File_getFullPath( basicdirnm, "Misc" );
	    if ( !File_copy(basicdirnm,dirnm,YES) )
	    {
		BufferString msg( "Cannot create directory: " );
		msg += dirnm;
		ErrMsg( msg );
		state_ = Bad;
		return;
	    }
	}

	MultiID ky( Translator::sMiscID );
	ky += "1";
	IOObj* iostrm = new IOStream("Misc",ky);
	iostrm->setGroup( "Miscellaneous directory" );
	iostrm->setTranslator( "dGB" );
	IOLink* iol = new IOLink( iostrm );
	iol->key_ = Translator::sMiscID;
	iol->dirname = iostrm->name();
	IOObj* previoobj = (IOObj*)(*dirPtr())[MultiID("100060")];
	dirPtr()->objs_ += iol;
	dirPtr()->objs_.moveAfter( iol, previoobj );
	dirPtr()->doWrite();
	to( prevkey );
    }
}


IOMan::~IOMan()
{
    delete dirptr;
}


bool IOMan::setRootDir( const char* dirnm )
{
    if ( !dirnm || !strcmp(rootdir,dirnm) ) return true;
    if ( !File_isDirectory(dirnm) ) return false;
    rootdir = dirnm;
    return setDir( rootdir );
}


bool IOMan::to( const IOLink* link )
{
    if ( !link && curlvl == 0 )
	return false;
    if ( bad() ) return link ? to( link->link()->key() ) : to( prevkey );

    FileNameString fulldir( curDir() );
    const char* dirnm = link ? (const char*)link->dirname : "..";
    fulldir = File_getFullPath( fulldir, dirnm );
    if ( !File_isDirectory(fulldir) ) return false;

    prevkey = dirptr->key();
    return setDir( fulldir );
}


bool IOMan::to( const MultiID& ky )
{
    MultiID key;
    IOObj* refioobj = IODir::getObj( ky );
    if ( !refioobj )
    {
	key = ky.upLevel();
	refioobj = IODir::getObj( key );
	if ( !refioobj )		key = "";
    }
    else if ( !refioobj->isLink() )	key = ky.upLevel();
    else				key = ky;
    delete refioobj;

    IODir* newdir = key == "" ? new IODir( rootdir ) : new IODir( key );
    if ( !newdir || newdir->bad() ) return false;

    if ( dirptr )
    {
	prevkey = dirptr->key();
	delete dirptr;
    }
    dirptr = newdir;
    curlvl = levelOf( curDir() );
    return true;
}


IOObj* IOMan::get( const MultiID& ky ) const
{
    if ( dirptr )
    {
	const IOObj* ioobj = (*dirptr)[ky];
	if ( ioobj ) return ioobj->clone();
    }

    return IODir::getObj( ky );
}


IOObj* IOMan::getIfOnlyOne( const char* tgname ) const
{
    if ( bad() || !tgname ) return 0;

    const IOObj* ioobj = 0;
    for ( int idx=0; idx<dirptr->size(); idx++ )
    {
	if ( !strcmp((*dirptr)[idx]->group(),tgname) )
	{
	    if ( ioobj ) return 0;
	    ioobj = (*dirptr)[idx];
	}
    }

    return ioobj ? ioobj->clone() : 0;
}


IOObj* IOMan::getByName( const char* objname,
			 const char* partrgname, const char* parname )
{
    if ( !objname || !*objname )
	return 0;
    MultiID startky = dirptr->key();
    to( MultiID("") );

    bool havepar = partrgname && *partrgname;
    bool parprov = parname && *parname;
    const UserIDObjectSet<IOObj>* ioobjs = &dirptr->getObjs();
    TypeSet<MultiID> kys;
    const IOObj* ioobj;
    for ( int idx=0; idx<ioobjs->size(); idx++ )
    {
	ioobj = (*ioobjs)[idx];
	if ( !havepar )
	{
	    if ( !strcmp(ioobj->name(),objname) )
		kys += ioobj->key();
	}
	else
	{
	    if ( !strcmp(ioobj->group(),partrgname) )
	    {
		if ( !parprov || ioobj->name() == parname )
		{
		    kys += ioobj->key();
		    if ( parprov ) break;
		}
	    }
	}
    }

    ioobj = 0;
    for ( int idx=0; idx<kys.size(); idx++ )
    {
	if ( havepar && !to( kys[idx] ) )
	{
	    BufferString msg( "Survey is corrupt. Cannot go to dir with ID: " );
	    msg += (const char*)kys[idx];
	    ErrMsg( msg );
	    return 0;
	}

	ioobj = (*dirptr)[objname];
	if ( ioobj ) break;
    }

    if ( ioobj ) ioobj = ioobj->clone();
    to( startky );
    return (IOObj*)ioobj;
}


const char* IOMan::nameOf( const char* id ) const
{
    static FileNameString ret;
    ret = "";
    if ( !id || !*id ) return ret;

    MultiID ky( id );
    IOObj* ioobj = get( ky );
    if ( !ioobj ) ret = id;
    else
    {
	do { 
	    ret += ioobj->name();
	    IOObj* parioobj = ioobj->getParent();
	    delete ioobj;
	    ioobj = parioobj;
	    if ( ioobj ) ret += " <- ";
	} while ( ioobj );
    }

    return ret;
}


void IOMan::back()
{
    to( prevkey );
}


const char* IOMan::curDir() const
{
    if ( !bad() ) return dirptr->dirName();
    return rootdir;
}


MultiID IOMan::key() const
{
    if ( !bad() ) return dirptr->key();
    return MultiID( "" );
}


bool IOMan::setDir( const char* dirname )
{
    if ( !dirname ) dirname = rootdir;

    IODir* newdirptr = new IODir( dirname );
    if ( !newdirptr ) return false;
    if ( newdirptr->bad() )
    {
	delete newdirptr;
	return false;
    }

    prevkey = key();
    delete dirptr;
    dirptr = newdirptr;
    curlvl = levelOf( curDir() );
    return true;
}


MultiID IOMan::newKey() const
{
    if ( bad() ) return MultiID( "" );
    return dirptr->newKey();
}


void IOMan::getEntry( CtxtIOObj& ctio, MultiID parentkey )
{
    ctio.setObj( 0 );
    if ( ctio.ctxt.name() == "" ) return;

    if ( parentkey == "" )
	parentkey = ctio.ctxt.parentkey;
    if ( parentkey != "" )
	to( parentkey );
    else if ( ctio.ctxt.selkey != ""
	   && ( ctio.ctxt.newonlevel != 2
	     || dirPtr()->main()->key().nrKeys() < 3 ) )
	to( ctio.ctxt.selkey );

    IOObj* ioobj = (*dirPtr())[ctio.ctxt.name()];
    if ( ioobj && ctio.ctxt.trgroup->name() != ioobj->group() )
	ioobj = 0;

    if ( !ioobj )
    {
	UserIDString cleanname( ctio.ctxt.name() );
	char* ptr = cleanname;
	cleanupString( ptr, NO, *ptr == *sDirSep, YES );
	IOStream* iostrm = new IOStream( ctio.ctxt.name(), newKey(), NO );
	iostrm->setFileName( cleanname );
	dirPtr()->mkUniqueName( iostrm );
	iostrm->setParentKey( parentkey );
	iostrm->setGroup( ctio.ctxt.trgroup->name() );
	iostrm->setTranslator( ctio.ctxt.deftransl ?
			    (const char*)ctio.ctxt.deftransl
			  : (const char*)ctio.ctxt.trgroup->defs()[0]->name() );
	while ( File_exists( iostrm->fileName() ) )
	{
	     FileNameString fname( iostrm->fileName() );
	     fname += "X";
	     iostrm->setFileName( fname );
	}

	ioobj = iostrm;
	if ( ctio.ctxt.crlink )
	    ioobj = new IOLink( ioobj->name(), ioobj );
	dirPtr()->addObj( ioobj );
    }

    ctio.setObj( ioobj->clone() );
}


int IOMan::levelOf( const char* dirnm ) const
{
    if ( !dirnm ) return 0;

    int lendir = strlen(dirnm);
    int lenrootdir = strlen(rootdir);
    if ( lendir <= lenrootdir ) return 0;

    int lvl = 0;
    const char* ptr = ((const char*)dirnm) + lenrootdir;
    while ( ptr )
    {
	ptr++; lvl++;
	ptr = strchr( ptr, *sDirSep );
    }
    return lvl;
}


IOParList* IOMan::getParList( const char* typ ) const
{
    return new IOParList( typ );
}


bool IOMan::getAuxfname( const MultiID& ky, FileNameString& fn ) const
{
    bool local = true;
    IOObj* ioobj = (IOObj*)(*dirptr)[ky];
    if ( !ioobj ) { local = false; ioobj = IODir::getObj( ky ); }
    if ( !ioobj ) return false;
    fn = File_getFullPath( ioobj->dirName(), ".aux" );
    if ( !local ) delete ioobj;
    return true;
}


bool IOMan::hasAux( const MultiID& ky ) const
{
    IOParList* iopl = getAuxList( ky );
    bool rv = iopl && (*iopl)[ky];
    delete iopl;
    return rv;
}


IOParList* IOMan::getAuxList( const MultiID& ky ) const
{
    FileNameString fn;
    if ( !getAuxfname(ky,fn) ) return 0;

    ifstream strm( fn );
    return new IOParList( strm );
}


bool IOMan::putAuxList( const MultiID& ky, const IOParList* iopl ) const
{
    if ( !iopl ) return true;
    FileNameString fn;
    if ( !getAuxfname(ky,fn) ) return false;

    ofstream strm( fn ); if ( strm.fail() ) return false;
    return iopl->write( strm );
}


IOPar* IOMan::getAux( const MultiID& ky ) const
{
    IOParList* iopl = getAuxList( ky );
    if ( !iopl ) return new IOPar( ky );
    IOPar* iopar = (*iopl)[ky];
    if ( iopar ) *iopl -= iopar;
    else	 iopar = new IOPar( ky );

    delete iopl;
    return iopar;
}


bool IOMan::putAux( const MultiID& ky, const IOPar* iopar ) const
{
    if ( !iopar ) return true;
    IOParList* iopl = getAuxList( ky );
    if ( !iopl ) return false;

    IOPar* listiopar = (*iopl)[iopar->name()];
    if ( listiopar ) *listiopar = *iopar;
    else	     *iopl += new IOPar( *iopar );

    int rv = putAuxList( ky, iopl );
    delete iopl;
    return rv;
}


bool IOMan::removeAux( const MultiID& ky ) const
{
    IOParList* iopl = getAuxList( ky );
    if ( !iopl ) return true;
    IOPar* iopar = (*iopl)[ky];
    if ( !iopar ) { delete iopl; return true; }
    *iopl -= iopar;

    int rv;
    FileNameString fn;
    getAuxfname( ky, fn );
    if ( iopl->size() == 0 )
	rv = File_remove( fn, YES, NO );
    else
    {
	ofstream strm( fn );
	rv = iopl->write( strm );
    }

    delete iopar;
    delete iopl;
    return rv;
}
