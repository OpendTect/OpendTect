/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 3-8-1994
-*/

static const char* rcsID = "$Id: ioman.cc,v 1.1.1.2 1999-09-16 09:33:34 arend Exp $";

#include "ioman.h"
#include "iodir.h"
#include "ioparlist.h"
#include "iolink.h"
#include "iostrm.h"
#include "transl.h"
#include "ctxtioobj.h"
#include "filegen.h"
#include <stdlib.h>
#include <fstream.h>

IOMan*	IOMan::theinst_	= 0;
void	IOMan::stop()	{ delete theinst_; theinst_ = 0; }
extern "C" void SetSurveyName(const char*);


int IOMan::newSurvey()
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
    if ( !to( prevunitid ) ) return;

    state_ = Good;
    curlvl = 0;

    if ( dirPtr()->unitID() == UnitID("-1") ) return;

    if ( ! (*dirPtr())[UnitID(Translator::sMiscUid)] )
    {
	FileNameString dirnm = File_getFullPath( rootdir, "Misc" );
	if ( !File_exists(dirnm) )
	{
	    FileNameString basicdirnm = GetDataFileName( "BasicSurvey" );
	    basicdirnm = File_getFullPath( basicdirnm, "Misc" );
	    if ( !File_copy(basicdirnm,dirnm,YES) )
	    {
		cerr << "Cannot create directory: "
		     << dirnm << endl;
		state_ = Bad;
		return;
	    }
	}

	UnitID uid( Translator::sMiscUid );
	uid += "1";
	IOObj* iostrm = new IOStream("Misc",uid);
	iostrm->setGroup( "Miscellaneous directory" );
	iostrm->setTranslator( "dGB" );
	IOLink* iol = new IOLink( iostrm );
	iol->unitid = Translator::sMiscUid;
	iol->dirname = iostrm->name();
	IOObj* previoobj = (IOObj*)(*dirPtr())[UnitID("100060")];
	dirPtr()->objs_ += iol;
	dirPtr()->objs_.moveAfter( iol, previoobj );
	dirPtr()->doWrite();
	to( prevunitid );
    }
}


IOMan::~IOMan()
{
    delete dirptr;
}


int IOMan::setRootDir( const char* dirnm )
{
    if ( !dirnm || !strcmp(rootdir,dirnm) ) return YES;
    if ( !File_isDirectory(dirnm) ) return NO;
    rootdir = dirnm;
    return setDir( rootdir );
}


int IOMan::to( const IOLink* link )
{
    if ( !link && curlvl == 0 )
	return NO;
    if ( bad() ) return link ? to( link->link()->unitID() ) : to( prevunitid );

    FileNameString fulldir( curDir() );
    const char* dirnm = link ? (const char*)link->dirname : "..";
    fulldir = File_getFullPath( fulldir, dirnm );
    if ( !File_isDirectory(fulldir) ) return NO;

    prevunitid = dirptr->unitID();
    return setDir( fulldir );
}


int IOMan::to( const UnitID& uid )
{
    UnitID unitid;
    IOObj* refioobj = IODir::getObj( uid );
    if ( !refioobj )
    {
	unitid = uid.parent();
	refioobj = IODir::getObj( unitid );
	if ( !refioobj )		unitid = "";
    }
    else if ( !refioobj->isLink() )	unitid = uid.parent();
    else				unitid = uid;
    delete refioobj;

    IODir* newdir = unitid == "" ? new IODir( rootdir ) : new IODir( unitid );
    if ( !newdir || newdir->bad() ) return NO;

    if ( dirptr )
    {
	prevunitid = dirptr->unitID();
	delete dirptr;
    }
    dirptr = newdir;
    curlvl = levelOf( curDir() );
    return YES;
}


IOObj* IOMan::get( const UnitID& uid ) const
{
    if ( dirptr )
    {
	const IOObj* ioobj = (*dirptr)[uid];
	if ( ioobj ) return ioobj->cloneStandAlone();
    }

    return IODir::getObj( uid );
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

    return ioobj ? ioobj->cloneStandAlone() : 0;
}


IOObj* IOMan::getByName( const char* objname,
			 const char* partrgname, const char* parname )
{
    if ( !objname || !*objname )
	return 0;
    UnitID startuid = dirptr->unitID();
    to( UnitID("") );

    bool havepar = partrgname && *partrgname;
    bool parprov = parname && *parname;
    const UserIDObjectSet<IOObj>* ioobjs = &dirptr->getObjs();
    TypeSet<UnitID> uids;
    const IOObj* ioobj;
    for ( int idx=0; idx<ioobjs->size(); idx++ )
    {
	ioobj = (*ioobjs)[idx];
	if ( !havepar )
	{
	    if ( !strcmp(ioobj->name(),objname) )
		uids += ioobj->unitID();
	}
	else
	{
	    if ( !strcmp(ioobj->group(),partrgname) )
	    {
		if ( !parprov || ioobj->name() == parname )
		{
		    uids += ioobj->unitID();
		    if ( parprov ) break;
		}
	    }
	}
    }

    ioobj = 0;
    for ( int idx=0; idx<uids.size(); idx++ )
    {
	if ( havepar && !to(uids[idx]) )
	{
	    cerr << "Survey is corrupt. Cannot go to directory with ID: "
		 << uids[idx] << endl;
	    return 0;
	}

	ioobj = (*dirptr)[objname];
	if ( ioobj ) break;
    }

    if ( ioobj ) ioobj = ioobj->cloneStandAlone();
    to( UnitID(startuid) );
    return (IOObj*)ioobj;
}


const char* IOMan::nameOf( const char* id ) const
{
    static FileNameString ret;
    ret = "";
    if ( !id || !*id ) return ret;

    UnitID uid( id );
    IOObj* ioobj = get( uid );
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
    to( prevunitid );
}


const char* IOMan::curDir() const
{
    if ( !bad() ) return dirptr->dirName();
    return rootdir;
}


UnitID IOMan::unitID() const
{
    if ( !bad() ) return dirptr->unitID();
    return UnitID( "" );
}


int IOMan::setDir( const char* dirname )
{
    if ( !dirname ) dirname = rootdir;

    IODir* newdirptr = new IODir( dirname );
    if ( !newdirptr ) return NO;
    if ( newdirptr->bad() )
    {
	delete newdirptr;
	return NO;
    }

    prevunitid = unitID();
    delete dirptr;
    dirptr = newdirptr;
    curlvl = levelOf( curDir() );
    return YES;
}


UnitID IOMan::newId() const
{
    if ( bad() ) return UnitID( "" );
    return dirptr->newId();
}


void IOMan::getEntry( CtxtIOObj& ctio, UnitID parentid )
{
    ctio.setObj( 0 );
    if ( ctio.ctxt.name() == "" ) return;

    if ( parentid == "" )
	parentid = ctio.ctxt.parentid;
    if ( parentid != "" )
	to( parentid );
    else if ( ctio.ctxt.selid != ""
	   && ( ctio.ctxt.newonlevel != 2
	     || dirPtr()->main()->unitID().level() < 2 ) )
	to( ctio.ctxt.selid );

    IOObj* ioobj = (*dirPtr())[ctio.ctxt.name()];
    if ( ioobj && ctio.ctxt.trgroup->name() != ioobj->group() )
	ioobj = 0;

    if ( !ioobj )
    {
	UserIDString cleanname( ctio.ctxt.name() );
	char* ptr = cleanname;
	cleanupString( ptr, NO, *ptr == '/', YES );
	IOStream* iostrm = new IOStream( ctio.ctxt.name(), newId(), NO );
	iostrm->setFileName( cleanname );
	dirPtr()->mkUniqueName( iostrm );
	iostrm->setParentId( parentid );
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

    ctio.setObj( ioobj->cloneStandAlone() );
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
	ptr = strchr( ptr, '/' );
    }
    return lvl;
}


IOParList* IOMan::getParList( const char* typ ) const
{
    return new IOParList( typ );
}


int IOMan::getAuxfname( const UnitID& uid, FileNameString& fn ) const
{
    int local = YES;
    IOObj* ioobj = (IOObj*)(*dirptr)[uid];
    if ( !ioobj ) { local = NO; ioobj = IODir::getObj( uid ); }
    if ( !ioobj ) return NO;
    fn = File_getFullPath( ioobj->dirName(), ".aux" );
    if ( !local ) delete ioobj;
    return YES;
}


int IOMan::hasAux( const UnitID& uid ) const
{
    IOParList* iopl = getAuxList( uid );
    int rv = iopl && (*iopl)[uid] ? YES : NO;
    delete iopl;
    return rv;
}


IOParList* IOMan::getAuxList( const UnitID& uid ) const
{
    FileNameString fn;
    if ( !getAuxfname(uid,fn) ) return 0;

    ifstream strm( fn );
    return new IOParList( strm );
}


int IOMan::putAuxList( const UnitID& uid, const IOParList* iopl ) const
{
    if ( !iopl ) return YES;
    FileNameString fn;
    if ( !getAuxfname(uid,fn) ) return NO;

    ofstream strm( fn ); if ( strm.fail() ) return NO;
    return iopl->write( strm );
}


IOPar* IOMan::getAux( const UnitID& uid ) const
{
    IOParList* iopl = getAuxList( uid );
    if ( !iopl ) return new IOPar( uid );
    IOPar* iopar = (*iopl)[uid];
    if ( iopar ) *iopl -= iopar;
    else	 iopar = new IOPar( uid );

    delete iopl;
    return iopar;
}


int IOMan::putAux( const UnitID& uid, const IOPar* iopar ) const
{
    if ( !iopar ) return YES;
    IOParList* iopl = getAuxList( uid );
    if ( !iopl ) return NO;

    IOPar* listiopar = (*iopl)[iopar->name()];
    if ( listiopar ) *listiopar = *iopar;
    else	     *iopl += new IOPar( *iopar );

    int rv = putAuxList( uid, iopl );
    delete iopl;
    return rv;
}


int IOMan::removeAux( const UnitID& uid ) const
{
    IOParList* iopl = getAuxList( uid );
    if ( !iopl ) return YES;
    IOPar* iopar = (*iopl)[uid];
    if ( !iopar ) { delete iopl; return YES; }
    *iopl -= iopar;

    int rv;
    FileNameString fn;
    getAuxfname( uid, fn );
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
