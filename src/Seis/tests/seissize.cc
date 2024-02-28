/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprog.h"
#include "testprog.h"

#include "cbvsio.h"
#include "dirlist.h"
#include "file.h"
#include "ioman.h"
#include "multiid.h"
#include "seisioobjinfo.h"
#include "segydirectdef.h"
#include "segydirecttr.h"

static const char* sKeyCBVSID()			{ return "CBVS"; }
static const char* sKeyCBVSZSliceID()		{ return "CBVS_Z_Slice"; }
static const char* sKey2DID()			{ return "2D"; }
static const char* sKeySEGYDirID()		{ return "SEGYDirect"; }

#define mTest( testname, test, message ) \
    if ( (test)==true ) \
    { \
	handleTestResult( true, testname ); \
    } \
    else \
    { \
	handleTestResult( false, testname, message ); \
	return false; \
    }


#define mCalcAuxFileSize( totalsz ) \
    BufferStringSet auxfilenames; \
    if ( sttr->havePars() ) \
    { \
	auxfilenames.addIfNew( sttr->getAuxFileName(sParFileExtension()) ); \
    } \
    if ( sttr->haveStats() ) \
    { \
	auxfilenames.addIfNew( sttr->getAuxFileName(sStatsFileExtension()) ); \
    } \
    if ( sttr->haveProc() ) \
    { \
	auxfilenames.addIfNew( sttr->getAuxFileName(sProcFileExtension()) ); \
    } \
    for ( const auto* fnm : auxfilenames ) \
    { \
	totalsz += File::getFileSizeInBytes( *fnm ); \
    }


#define mCheckSeisIOObjInfo( seisobj, obj, strm ) \
    const SeisIOObjInfo seisobj( obj ); \
    if ( !seisobj.isOK() ) \
    { \
	strm << "SeisIOObjInfo of " << obj.name() \
	     << " is not OK" << od_newline; \
	return false; \
    }


#define mCreateAndReadTransl( trl, trlnm, obj, strm ) \
    mDynamicCast(trl*,PtrMan<trl> trlnm,obj.createTranslator())\
    if ( !trlnm || !trlnm->initRead(obj.getConn(Conn::Read)) ) \
    { \
	strm << "Couldn't create/read " << obj.name() \
	     << " translator" << od_newline; \
	return false; \
    }


bool testCBVSSize( const IOObj& obj, od_ostream& strm )
{
    const BufferString testname( "Single CBVS file size" );
    mCheckSeisIOObjInfo( seisobj, obj, strm )
    const od_int64 trlsize = seisobj.getFileSizeInBytes();

    mCreateAndReadTransl(SeisTrcTranslator,sttr,obj,strm)
    od_int64 fsize = File::getFileSizeInBytes( obj.fullUserExpr() );
    mCalcAuxFileSize(fsize)

    BufferString msg( "Trl size = ", trlsize );
    msg.add( "; ").add("File size: ").add( fsize );
    mTest( testname, (trlsize==fsize), msg )

    return true;
}


bool testZSliceCBVSSize( const IOObj& obj, od_ostream& strm )
{
    const BufferString testname( "Z-Slice CBVS file size" );
    mCheckSeisIOObjInfo( seisobj, obj, strm )
    const od_int64 trlsize = seisobj.getFileSizeInBytes();

    const BufferString filenm = obj.fullUserExpr();
    od_int64 totalsz = 0;
    int nrfiles = 0;
    while ( true )
    {
	const BufferString currfname
		= CBVSIOMgr::getFileName( filenm, nrfiles );
	if ( !File::exists(currfname) )
	    break;

	const od_int64 fsize = File::getFileSizeInBytes( currfname );
	totalsz += fsize;
	nrfiles++;
    }

    mCreateAndReadTransl(SeisTrcTranslator,sttr,obj,strm)
    mCalcAuxFileSize(totalsz)

    BufferString msg( "Trl size = ", trlsize );
    msg.add( "; ").add("File size: ").add( totalsz );
    mTest( testname, (trlsize==totalsz), msg )
    return true;
}


bool test2DSize( const IOObj& obj, od_ostream& strm )
{
    const BufferString testname( "2D file size" );
    mCheckSeisIOObjInfo( seisobj, obj, strm )
    const od_int64 trlsize = seisobj.getFileSizeInBytes();

    const BufferString filenm = obj.fullUserExpr();
    if ( filenm.isEmpty() || !File::exists(filenm))
	return false;

    od_int64 totalsz = 0;
    if ( File::isDirectory(filenm) )
    {
	const DirList dl( filenm.buf(), File::FilesInDir);
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    const FilePath filepath = dl.fullPath( idx );
	    totalsz += File::getFileSizeInBytes( filepath.fullPath() );
	}
    }
    else
	totalsz += File::getFileSizeInBytes( filenm );

    mCreateAndReadTransl(SeisTrcTranslator,sttr,obj,strm)
    mCalcAuxFileSize(totalsz)

    BufferString msg( "Trl size = ", trlsize );
    msg.add( "; ").add("File size: ").add( totalsz );
    mTest( testname, (trlsize==totalsz), msg )
    return true;
}


bool testSEGYDirectSize( const IOObj& obj, od_ostream& strm )
{
    const BufferString testname( "SEGY-Direct file size" );
    mCheckSeisIOObjInfo( seisobj, obj, strm )
    const od_int64 trlsize = seisobj.getFileSizeInBytes();

    const BufferString filenm = obj.fullUserExpr();
    mCreateAndReadTransl(SEGYDirectSeisTrcTranslator,trl,obj,strm)
    const SEGY::DirectDef* def = trl->getDef();
    if ( !def )
	return false;

    const SEGY::FileDataSet& fds = def->fileDataSet();
    const int nrfiles = fds.nrFiles();
    od_int64 totalsz = File::getFileSizeInBytes( filenm );
    for ( int idx=0; idx<nrfiles; idx++ )
    {
	const StringView fname = fds.fileName( idx );
	if ( !File::exists(fname) )
	    continue;

	const od_int64 size = File::getFileSizeInBytes( fname );
	totalsz += size;
    }

    // --beacuse getAuxFileName function is private in derived class. Not sure
    // --if it needs to be exposed. Didn't see any use-case for exposing outside
    // --this test program.
    mCreateAndReadTransl(SeisTrcTranslator,sttr,obj,strm)
    mCalcAuxFileSize(totalsz);
    BufferString msg( "Trl size = ", trlsize );
    msg.add( "; ").add("File size: ").add( totalsz );
    mTest( testname, (trlsize==totalsz), msg )
    return true;
}


#define mLoadMIDFail(strm,pars,idstr,id)\
    if ( !pars().get(idstr,id) )\
    {\
	strm << "Can not find " \
	     << idstr << " from parameter file" << od_newline; \
	return false;\
    }\


#define mIOObjAbsent(obj,id,strm)\
    PtrMan<IOObj> obj = IOM().get( id );\
    if ( !obj )\
    {\
	strm << obj->name() << " is not available" << od_newline;\
	return false;\
    }\


mLoad1Module( "Seis" )

bool BatchProgram::doWork( od_ostream& strm )
{
    mInitBatchTestProg();
    bool alltestspassed = true;
    MultiID cbvsid, zcbvsid, twodid, sgydirid;
    mLoadMIDFail(strm,pars,sKeyCBVSID(),cbvsid)
    mLoadMIDFail(strm,pars,sKeyCBVSZSliceID(),zcbvsid)
    mLoadMIDFail(strm,pars,sKey2DID(),twodid)
    mLoadMIDFail(strm,pars,sKeySEGYDirID(),sgydirid)

    mIOObjAbsent(cbvsobj, cbvsid, strm)
    mIOObjAbsent(cbvszsobj,zcbvsid,strm)
    mIOObjAbsent(twodobj,twodid,strm)
    mIOObjAbsent(sgydirobj,sgydirid,strm)

    if ( !testCBVSSize(*cbvsobj,strm) )
	alltestspassed = false;
    if ( !testZSliceCBVSSize(*cbvszsobj,strm) )
	alltestspassed = false;
    if ( !test2DSize(*twodobj,strm) )
	alltestspassed = false;
    if ( !testSEGYDirectSize(*sgydirobj,strm) )
	alltestspassed = false;

    return alltestspassed;
}
