/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 3-8-1994
-*/

static const char* rcsID = "$Id: ioman.cc,v 1.18 2001-11-07 17:15:27 bert Exp $";

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
#include <fstream>

IOMan*	IOMan::theinst_	= 0;
void	IOMan::stop()	{ delete theinst_; theinst_ = 0; }
extern "C" void SetSurveyName(const char*);
extern "C" const char* GetBaseDataDir();


static void clearSelHists()
{
    const UserIDObjectSet<Translator>& grps = Translator::groups();
    const int sz = grps.size();
    for ( int idx=0; idx<sz; idx++ )
	grps[idx]->clearSelHist();
}


bool IOMan::newSurvey()
{
    delete IOMan::theinst_;
    IOMan::theinst_ = 0;
    clearSelHists();
    return !IOM().bad();
}


void IOMan::setSurvey( const char* survname )
{
    delete IOMan::theinst_;
    IOMan::theinst_ = 0;
    clearSelHists();
    SetSurveyName( survname );
}


IOMan::IOMan( const char* rd )
	: UserIDObject("IO Manager")
	, dirptr(0)
	, state_(IOMan::NeedInit)
{
    rootdir = rd && *rd ? rd : GetDataDir();
    if ( !File_isDirectory(rootdir) )
	rootdir = GetBaseDataDir();
}


void IOMan::init()
{
    state_ = Bad;
    if ( !to( prevkey ) ) return;

    state_ = Good;
    curlvl = 0;

    if ( dirPtr()->key() == MultiID("-1") ) return;

    int nrstddirdds = IOObjContext::totalNrStdDirs();
    const IOObjContext::StdDirData* prevdd = 0;
    bool needwrite = false;
    for ( int idx=0; idx<nrstddirdds; idx++ )
    {
	const IOObjContext::StdDirData* dd
		= IOObjContext::getStdDirData( (IOObjContext::StdSelType)idx );
	if ( (*dirPtr())[MultiID(dd->id)] ) { prevdd = dd; continue; }

	FileNameString basicdirnm = GetDataFileName( "BasicSurvey" );
	basicdirnm = File_getFullPath( basicdirnm, dd->dirnm );
	if ( !File_exists(basicdirnm) )
	    // Apparently, the application doesn't need such a directory
	    { prevdd = dd; continue; }

	FileNameString dirnm = File_getFullPath( rootdir, dd->dirnm );
	if ( !File_exists(dirnm) )
	{
	    // Apparently, this directory should have been in the survey
	    // It is not. If it is a basic directory, we do not want to
	    // continue. Otherwise, we want to create the directory.
	    if ( !idx ) // 0=Seis, if this is missing, why go on?
	    {
		BufferString msg( "Corrupt survey: missing directory: " );
		msg += dirnm; ErrMsg( msg ); state_ = Bad; return;
	    }
	    else if ( !File_copy(basicdirnm,dirnm,YES) )
	    {
		BufferString msg( "Cannot create directory: " );
		msg += dirnm; ErrMsg( msg ); state_ = Bad; return;
	    }
	}

	MultiID ky( dd->id );
	ky += "1";
	IOObj* iostrm = new IOStream(dd->dirnm,ky);
	iostrm->setGroup( dd->desc );
	iostrm->setTranslator( "dGB" );
	IOLink* iol = new IOLink( iostrm );
	iol->key_ = dd->id;
	iol->dirname = iostrm->name();
	const IOObj* previoobj = prevdd ? (*dirPtr())[prevdd->id]
					: dirPtr()->main();
	dirPtr()->objs_ += iol;
	dirPtr()->objs_.moveAfter( iol, const_cast<IOObj*>(previoobj) );

	prevdd = dd;
	needwrite = true;
    }

    if ( needwrite )
    {
	dirPtr()->doWrite();
	to( prevkey );
    }
}


IOMan::~IOMan()
{
    delete dirptr;
}


extern "C" const char* GetSurveyFileName();

#define mErrRet(str) \
    { errmsg = str; return false; }
#define mErrRetNotGDIDir(fname) \
    { \
        errmsg = GetDgbApplicationCode() == mDgbApplCodeDTECT \
	       ? "$dTECT_DATA=" : "$dGB_DATA="; \
        errmsg += GetBaseDataDir(); \
        errmsg += "\nThis is not a dGB data storage directory."; \
        return false; \
    }

bool IOMan::validSurveySetup( BufferString& errmsg )
{
    errmsg = "";
    if ( !IOM().bad() ) return true;

    delete IOMan::theinst_;
    IOMan::theinst_ = 0;
    FileNameString fname;
    if ( !GetBaseDataDir() )
    {
	if ( GetDgbApplicationCode() == mDgbApplCodeDTECT )
	    mErrRet("Please set the environment variable dTECT_DATA.")
	else
	    mErrRet("Please set the environment variable dGB_DATA.")
    }
    else if ( !File_exists(GetBaseDataDir()) )
	mErrRetNotGDIDir(fname)

    fname = GetBaseDataDir();
    fname = File_getFullPath( fname, ".omf" );
    if ( File_isEmpty(fname) ) mErrRetNotGDIDir(fname)

    fname = GetDataDir();
    if ( !File_exists(fname) )
    {
	fname = GetSurveyFileName();
	if ( File_exists(fname) && !File_remove( fname, YES, NO ) )
	{
	    fname = "The file ";
	    fname += GetSurveyFileName();
	    fname += " contains an invalid survey.\n";
	    fname += "Please remove this file";
	    mErrRet(fname)
	}
	else if ( IOM().bad() ) mErrRetNotGDIDir(fname)
    }
    else
    {
	fname = File_getFullPath( fname, ".survey" );
	bool hassurv = !File_isEmpty(fname);
	fname = File_getFullPath( GetDataDir(), ".omf" );
	bool hasomf = !File_isEmpty(fname);

	if ( hassurv && hasomf )
	    // Why the heck did it fail?
	    mErrRet("Cannot start Object Management. Please contact support")
	if ( !hassurv && !hasomf )
	    cerr << "Warning: Essential data files not found in ";
	else if ( !hassurv )
	    cerr << "Warning: No '.survey' found in ";
	else if ( !hasomf )
	    cerr << "Warning: No '.omf' found in ";
	cerr << GetDataDir();
	cerr << ". This survey is corrupt." << endl;

	fname = GetSurveyFileName();
	if ( File_exists(fname) && !File_remove( fname, YES, NO ) )
	{
	    fname = "The file ";
	    fname += GetSurveyFileName();
	    fname += " contains an invalid survey.\n";
	    fname += "Please remove this file";
	    mErrRet(fname)
	}
	else if ( IOM().bad() ) mErrRetNotGDIDir(fname)
    }

    return true;
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
    if ( matchString("ID=<",objname) )
    {
	BufferString oky( objname+4 );
	char* ptr = strchr( oky.buf(), '>' );
	if ( ptr ) *ptr = '\0';
	return get( MultiID((const char*)oky) );
    }

    MultiID startky = dirptr->key();
    to( MultiID("") );

    bool havepar = partrgname && *partrgname;
    bool parprov = parname && *parname;
    const UserIDObjectSet<IOObj>* ioobjs = &dirptr->getObjs();
    ObjectSet<MultiID> kys;
    const IOObj* ioobj;
    for ( int idx=0; idx<ioobjs->size(); idx++ )
    {
	ioobj = (*ioobjs)[idx];
	if ( !havepar )
	{
	    if ( !strcmp(ioobj->name(),objname) )
		kys += new MultiID(ioobj->key());
	}
	else
	{
	    if ( !strcmp(ioobj->group(),partrgname) )
	    {
		if ( !parprov || ioobj->name() == parname )
		{
		    kys += new MultiID(ioobj->key());
		    if ( parprov ) break;
		}
	    }
	}
    }

    ioobj = 0;
    for ( int idx=0; idx<kys.size(); idx++ )
    {
	if ( havepar && !to( *kys[idx] ) )
	{
	    BufferString msg( "Survey is corrupt. Cannot go to dir with ID: " );
	    msg += (const char*)(*kys[idx]);
	    ErrMsg( msg );
	    deepErase( kys );
	    return 0;
	}

	ioobj = (*dirptr)[objname];
	if ( ioobj ) break;
    }

    if ( ioobj ) ioobj = ioobj->clone();
    to( startky );
    deepErase( kys );
    return (IOObj*)ioobj;
}


const char* IOMan::nameOf( const char* id, bool full ) const
{
    static FileNameString ret;
    ret = "";
    if ( !id || !*id ) return ret;

    MultiID ky( id );
    IOObj* ioobj = get( ky );
    if ( !ioobj )
	{ ret = "ID=<"; ret += id; ret += ">"; }
    else
    {
	do { 
	    ret += ioobj->name();
	    if ( !full ) break;
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
    else if ( ctio.ctxt.hasStdSelType()
	   && ( ctio.ctxt.newonlevel != 2
	     || dirPtr()->main()->key().nrKeys() < 3 ) )
	to( ctio.ctxt.stdSelKey() );

    IOObj* ioobj = dirPtr()->findObj( ctio.ctxt.name() );
    if ( ioobj && ctio.ctxt.trgroup->name() != ioobj->group() )
	ioobj = 0;

    if ( !ioobj )
    {
	UserIDString cleanname( ctio.ctxt.name() );
	char* ptr = cleanname.buf();
	cleanupString( ptr, NO, *ptr == *sDirSep, YES );
	IOStream* iostrm = new IOStream( ctio.ctxt.name(), newKey(), NO );
	iostrm->setFileName( cleanname );
	dirPtr()->mkUniqueName( iostrm );
	iostrm->setParentKey( parentkey );
	iostrm->setGroup( ctio.ctxt.trgroup->name() );
	iostrm->setTranslator( ctio.ctxt.deftransl != "" ?
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
	if ( ctio.ctxt.includekeyval
	  && *((const char*)ctio.ctxt.ioparkeyval[0]) )
	    ioobj->pars().set( ctio.ctxt.ioparkeyval[0],
		    		ctio.ctxt.ioparkeyval[1] );
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
