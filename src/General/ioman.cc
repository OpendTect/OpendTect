/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 3-8-1994
-*/

static const char* rcsID = "$Id: ioman.cc,v 1.43 2004-03-26 15:45:03 bert Exp $";

#include "ioman.h"
#include "iodir.h"
#include "ioparlist.h"
#include "iolink.h"
#include "iostrm.h"
#include "transl.h"
#include "ctxtioobj.h"
#include "filegen.h"
#include "errh.h"
#include "strmprov.h"
#include <stdlib.h>

IOMan*	IOMan::theinst_	= 0;
void	IOMan::stop()	{ delete theinst_; theinst_ = 0; }
extern "C" void SetSurveyName(const char*);
extern "C" const char* GetSurveyName();
extern "C" void SetSurveyNameDirty();
extern "C" const char* GetBaseDataDir();


static void clearSelHists()
{
    const ObjectSet<TranslatorGroup>& grps = TranslatorGroup::groups();
    const int sz = grps.size();
    for ( int idx=0; idx<grps.size(); idx++ )
	grps[idx]->clearSelHist();
}


bool IOMan::newSurvey()
{
    delete IOMan::theinst_;
    IOMan::theinst_ = 0;
    clearSelHists();
    SetSurveyNameDirty();

    return !IOM().bad();
}


void IOMan::setSurvey( const char* survname )
{
    delete IOMan::theinst_;
    IOMan::theinst_ = 0;
    clearSelHists();
    SetSurveyName( survname );
}


const char* IOMan::surveyName() const
{
    return GetSurveyName();
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
	int idxof = dirPtr()->objs_.indexOf( (IOObj*)previoobj );
	dirPtr()->objs_.insertAfter( iol, idxof );

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
#define mErrRetNotODDir(fname) \
    { \
        errmsg = "$DTECT_DATA="; errmsg += GetBaseDataDir(); \
        errmsg += "\nThis is not an OpendTect data storage directory."; \
	if ( fname ) \
	    { errmsg += "\n[Cannot find: "; errmsg += fname; errmsg += "]"; } \
        return false; \
    }

bool IOMan::validSurveySetup( BufferString& errmsg )
{
    errmsg = "";
    BufferString basedatadir( GetBaseDataDir() );
    if ( basedatadir == "" )
	mErrRet("Please set the environment variable DTECT_DATA.")
    else if ( !File_exists(basedatadir.buf()) )
	mErrRetNotODDir(0)

    BufferString fname = basedatadir;
    fname = File_getFullPath( fname, ".omf" );
    if ( File_isEmpty(fname) ) mErrRetNotODDir(fname.buf())

    const BufferString projdir = GetDataDir();
    if ( projdir != basedatadir && File_isDirectory(projdir) )
    {
	BufferString omffname = File_getFullPath( projdir, ".omf" );
	if ( File_isEmpty(omffname) )
	{
	    fname = File_getFullPath( projdir, ".omb" );
	    if ( File_exists(fname) )
		File_copy( fname, omffname, NO );
	}
	BufferString survfname = File_getFullPath( projdir, ".survey" );
	bool noomf = File_isEmpty(omffname);
	bool nosurv = File_isEmpty(survfname);

	if ( !noomf && !nosurv )
	{
	    if ( !IOM().bad() )
		return true; // This is normal
	    // So what's wrong here? In any case - survey is not good.
	}

	else
	{
	    if ( nosurv && noomf )
		cerr << "Warning: Essential data files not found in ";
	    else if ( nosurv )
		cerr << "Warning: Invalid or no '.survey' found in ";
	    else if ( noomf )
		cerr << "Warning: Invalid or no '.omf' found in ";
	    cerr << projdir << ".\nThis survey is corrupt." << endl;
	}
    }

    // Survey in ~/.od/survey[.$DETCT_USER] is invalid. Remove it if necessary
    BufferString survfname = GetSurveyFileName();
    if ( File_exists(survfname) && !File_remove( survfname, NO ) )
    {
	fname = "The file "; fname += survfname;
	fname += " contains an invalid survey.\n";
	fname += "Please remove this file";
	mErrRet(fname)
    }

    delete IOMan::theinst_; IOMan::theinst_ = 0;
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
    if ( !IOObj::isKey(ky) )
	return 0;

    if ( dirptr )
    {
	const IOObj* ioobj = (*dirptr)[ky];
	if ( ioobj ) return ioobj->clone();
    }

    return IODir::getObj( ky );
}


IOObj* IOMan::getOfGroup( const char* tgname, bool first,
			  bool onlyifsingle ) const
{
    if ( bad() || !tgname ) return 0;

    const IOObj* ioobj = 0;
    for ( int idx=0; idx<dirptr->size(); idx++ )
    {
	if ( !strcmp((*dirptr)[idx]->group(),tgname) )
	{
	    if ( onlyifsingle && ioobj ) return 0;

	    ioobj = (*dirptr)[idx];
	    if ( first && !onlyifsingle ) break;
	}
    }

    return ioobj ? ioobj->clone() : 0;
}


IOObj* IOMan::getLocal( const char* objname ) const
{
    if ( dirptr )
    {
	const IOObj* ioobj = (*dirptr)[objname];
	if ( ioobj ) return ioobj->clone();
    }

    if ( IOObj::isKey(objname) )
	return get( MultiID(objname) );

    return 0;
}


IOObj* IOMan::getByName( const char* objname,
			 const char* pardirname, const char* parname )
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

    bool havepar = pardirname && *pardirname;
    bool parprov = parname && *parname;
    const ObjectSet<IOObj>& ioobjs = dirptr->getObjs();
    ObjectSet<MultiID> kys;
    const IOObj* ioobj;
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	ioobj = ioobjs[idx];
	if ( !havepar )
	{
	    if ( !strcmp(ioobj->name(),objname) )
		kys += new MultiID(ioobj->key());
	}
	else
	{
	    if ( !strncmp(ioobj->group(),pardirname,3) )
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

    const IOObj* ioobj = (*dirPtr())[ ctio.ctxt.name() ];
    ctio.ctxt.fillTrGroup();
    if ( ioobj && ctio.ctxt.trgroup->userName() != ioobj->group() )
	ioobj = 0;

    if ( !ioobj )
    {
	IOStream* iostrm = new IOStream( ctio.ctxt.name(), newKey(), NO );
	dirPtr()->mkUniqueName( iostrm );
	iostrm->setParentKey( parentkey );
	iostrm->setGroup( ctio.ctxt.trgroup->userName() );
	const Translator* tr = ctio.ctxt.trgroup->templates().size() ?
	    			ctio.ctxt.trgroup->templates()[0] : 0;
	BufferString trnm( ctio.ctxt.deftransl != ""
			 ? ctio.ctxt.deftransl.buf()
			 : (tr ? tr->userName().buf() : "") );
	iostrm->setTranslator( trnm );

	// Generate the right filename
	Translator* tmptr = ctio.ctxt.trgroup->make( trnm );
	const char* fnm = generateFileName( tmptr, ctio.ctxt.name() );
	iostrm->setFileName( fnm );
	delete tmptr;

	ioobj = iostrm;
	if ( ctio.ctxt.crlink )
	    ioobj = new IOLink( ioobj->name(), (IOObj*)ioobj );
	if ( ctio.ctxt.includekeyval
	  && *((const char*)ctio.ctxt.ioparkeyval[0]) )
	    ioobj->pars().set( ctio.ctxt.ioparkeyval[0],
		    		ctio.ctxt.ioparkeyval[1] );
	dirPtr()->addObj( (IOObj*)ioobj );
    }

    ctio.setObj( ioobj->clone() );
}


const char* IOMan::generateFileName( Translator* tr, const char* fname )
{
    int subnr = 0;
    BufferString cleanname( fname );
    char* ptr = cleanname.buf();
    cleanupString( ptr, NO, *ptr == *sDirSep, YES );
    static BufferString fnm;
    for ( int subnr=0; ; subnr++ )
    {
	fnm = cleanname;
	if ( subnr ) fnm += subnr;
	if ( tr && tr->defExtension() )
	    { fnm += "."; fnm += tr->defExtension(); }
	if ( !File_exists(fnm) ) break;
    }

    return fnm;
}


bool IOMan::setFileName( MultiID key, const char* fname )
{
    IOObj* ioobj = get( key );
    if ( !ioobj ) return false;
    const FileNameString fulloldname = ioobj->fullUserExpr( true );
    ioobj->setName( fname );
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    if ( !iostrm ) return false;

    Translator* tr = ioobj->getTranslator();
    BufferString fnm = generateFileName( tr, fname );
    if ( File_isAbsPath(fulloldname) )
    {
	BufferString oldpath = File_getPathOnly( fulloldname );
	fnm = File_getFullPath( oldpath, fnm );
    }
    iostrm->setFileName( fnm );
  
    const FileNameString fullnewname = ioobj->fullUserExpr(true); 
    int ret = File_rename( fulloldname, fullnewname );
    if ( !ret || !commitChanges( *ioobj ) )
	return false;

    return true;
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


bool IOMan::haveEntries( const MultiID& id,
			 const char* trgrpnm, const char* trnm ) const
{
    IODir iodir( id );
    const bool chkgrp = trgrpnm && *trgrpnm;
    const bool chktr = trnm && *trnm;
    const int sz = iodir.size();
    for ( int idx=1; idx<sz; idx++ )
    {
	const IOObj& ioobj = *iodir[idx];
	if ( chkgrp && strcmp(ioobj.group(),trgrpnm) )
	    continue;
	if ( chktr && strcmp(ioobj.translator(),trnm) )
	    continue;
	return true;
    }
    return false;
}


bool IOMan::commitChanges( const IOObj& ioobj )
{
    to( ioobj.key() );
    return dirPtr() ? dirPtr()->commitChanges( &ioobj ) : false;
}


bool IOMan::permRemove( const MultiID& ky )
{
    if ( !dirPtr() || !dirPtr()->permRemove(ky) )
	return false;

    removeAux( ky );
    return true;
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
    bool rv = iopl && find( *iopl, (const char*)ky );
    delete iopl;
    return rv;
}


IOParList* IOMan::getAuxList( const MultiID& ky ) const
{
    FileNameString fn;
    if ( !getAuxfname(ky,fn) ) return 0;

    StreamData sd = StreamProvider( fn ).makeIStream();
    if ( !sd.usable() ) return 0;

    IOParList* iopl = new IOParList( *sd.istrm );
    iopl->setFileName( fn );

    sd.close();
    return iopl;
}


bool IOMan::putAuxList( const MultiID& ky, const IOParList* iopl ) const
{
    if ( !iopl ) return true;
    FileNameString fn;
    if ( !getAuxfname(ky,fn) ) return false;

    StreamData sd = StreamProvider( fn ).makeOStream();

    if( !sd.usable() ) { sd.close(); return false; }

    bool ret = iopl->write( *sd.ostrm );

    sd.close();
    return ret;
}


IOPar* IOMan::getAux( const MultiID& ky ) const
{
    IOParList* iopl = getAuxList( ky );
    if ( !iopl ) return new IOPar( ky );
    IOPar* iopar = find( *iopl, (const char*)ky );
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

    IOPar* listiopar = find( *iopl, iopar->name().buf() );
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
    IOPar* iopar = find( *iopl, (const char*)ky );
    if ( !iopar ) { delete iopl; return true; }
    *iopl -= iopar;

    int rv;
    FileNameString fn;
    getAuxfname( ky, fn );
    if ( iopl->size() == 0 )
	rv = File_remove( fn, NO );
    else
    {
	StreamData sd = StreamProvider( fn ).makeOStream();
	rv = iopl->write( *sd.ostrm );
	sd.close();
    }

    delete iopar;
    delete iopl;
    return rv;
}
