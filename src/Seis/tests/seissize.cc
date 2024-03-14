/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprog.h"
#include "moddepmgr.h"
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

static const int sCBVSSize			= 21102400;
static const int sZSliceCBVSSize		= 254588;
static const int s2DSize			= 2724740;
static const int sSEGYSize			= 39651;

static const int sNrFilesZSlice			= 19;
static const int sNrFilesSEGYDir		= 1;

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
	totalsz += File::getFileSize( *fnm ); \
    }


#define mCheckSeisIOObjInfo( seisobj, obj ) \
    const SeisIOObjInfo seisobj( obj ); \
    mRunStandardTestWithError(seisobj.isOK(),\
			      "Info on object",seisobj.errMsg().getString());


#define mCreateAndReadTransl( trl, trlnm, obj, msg ) \
    mDynamicCast(trl*,PtrMan<trl> trlnm,obj.createTranslator())\
    msg.add( "Failed to create " ).add( #trl ); \
    mRunStandardTestWithError( trlnm.ptr(), "Has translator", msg ) \
    mRunStandardTestWithError( trlnm->initRead(obj.getConn(Conn::Read)), \
			       "Initialize translator read", \
			       trlnm->message().getString() );


bool testCBVSSize( const IOObj& obj, od_ostream& strm )
{
    const BufferString testname( "Single CBVS file size" );
    BufferString msg;
    mCreateAndReadTransl(SeisTrcTranslator,sttr,obj,msg)
    mCheckSeisIOObjInfo( seisobj, obj )
    const od_int64 dssize = seisobj.getFileSize();
    BufferString dserrmsg( "Dataset size: ", dssize );
    dserrmsg.add( "; Actual size: " ).add( sCBVSSize );
    mRunStandardTestWithError( dssize == sCBVSSize,
			       "CBVS dataset size from SeisIOObjInfo",
			       dserrmsg )

    od_int64 totalsz = File::getFileSize( obj.fullUserExpr() );
    mCalcAuxFileSize(totalsz)
    BufferString errmsg( "Calculated size: ", dssize );
    errmsg.add( "; Actual size: " ).add( sCBVSSize );
    mRunStandardTestWithError( totalsz == sCBVSSize,
			       "CBVS dataset size calculated from Files",
			       errmsg )

    return true;
}


bool testZSliceCBVSSize( const IOObj& obj, od_ostream& strm )
{
    const BufferString testname( "Z-Slice CBVS file size" );
    BufferString msg;
    mCreateAndReadTransl(SeisTrcTranslator,sttr,obj,msg)
    mCheckSeisIOObjInfo( seisobj, obj )
    const od_int64 dssize = seisobj.getFileSize();
    BufferString dserrmsg( "Dataset size: ", dssize );
    dserrmsg.add( "; Actual size: " ).add( sZSliceCBVSSize );
    mRunStandardTestWithError( dssize == sZSliceCBVSSize,
			       "Z-Slice CBVS dataset size from SeisIOObjInfo",
			       dserrmsg )

    const BufferString filenm = obj.fullUserExpr();
    od_int64 totalsz = 0;
    int nrfiles = 0;
    while ( true )
    {
	const BufferString currfname
		= CBVSIOMgr::getFileName( filenm, nrfiles );
	if ( !File::exists(currfname) )
	    break;

	const od_int64 fsize = File::getFileSize( currfname );
	totalsz += fsize;
	nrfiles++;
    }

    mRunStandardTest( nrfiles == sNrFilesZSlice, "Number of files" );
    mCalcAuxFileSize(totalsz)
    BufferString errmsg( "Calculated size: ", dssize );
    errmsg.add( "; Actual size: " ).add( sZSliceCBVSSize );
    mRunStandardTestWithError( totalsz == sZSliceCBVSSize,
			   "Z-Slice CBVS dataset size calculated from Files",
			   errmsg )

    return true;
}


bool test2DSize( const IOObj& obj, od_ostream& strm )
{
    const BufferString testname( "2D file size" );
    BufferString msg;
    mCreateAndReadTransl(SeisTrcTranslator,sttr,obj,msg)
    mCheckSeisIOObjInfo( seisobj, obj )
    const od_int64 dssize = seisobj.getFileSize();
    BufferString dserrmsg( "Dataset size: ", dssize );
    dserrmsg.add( "; Actual size: " ).add( s2DSize );
    mRunStandardTestWithError( dssize == s2DSize,
			       "2-D dataset size from SeisIOObjInfo",
			       dserrmsg )

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
	    totalsz += File::getFileSize( filepath.fullPath() );
	}
    }
    else
	totalsz += File::getFileSize( filenm );

    mCalcAuxFileSize(totalsz)
    BufferString errmsg( "Calculated size: ", dssize );
    errmsg.add( "; Actual size: " ).add( s2DSize );
    mRunStandardTestWithError( totalsz == s2DSize,
			       "2-D dataset size calculated from Files",
			       errmsg )

    return true;
}


bool testSEGYDirectSize( const IOObj& obj, od_ostream& strm )
{
    const BufferString testname( "SEGY-Direct file size" );
    BufferString msg;
    mCreateAndReadTransl(SeisTrcTranslator,sttr,obj,msg)
    mCheckSeisIOObjInfo( seisobj, obj )
    const od_int64 dssize = seisobj.getFileSize();
    BufferString dserrmsg( "Dataset size: ", dssize );
    dserrmsg.add( "; Actual size: " ).add( sSEGYSize );
    mRunStandardTestWithError( dssize == sSEGYSize,
			       "SEG-Y Direct dataset size from SeisIOObjInfo",
			       dserrmsg )

    const BufferString filenm = obj.fullUserExpr();
    BufferString segydirmsg;
    mCreateAndReadTransl(SEGYDirectSeisTrcTranslator,trl,obj,segydirmsg)
    const SEGY::DirectDef* def = trl->getDef();
    if ( !def )
	return false;

    const SEGY::FileDataSet& fds = def->fileDataSet();
    const int nrfiles = fds.nrFiles();
    mRunStandardTest( nrfiles == sNrFilesSEGYDir, "Number of files: SEG-Y" );
    od_int64 totalsz = File::getFileSize( filenm );
    for ( int idx=0; idx<nrfiles; idx++ )
    {
	const StringView fname = fds.fileName( idx );
	if ( !File::exists(fname) )
	    continue;

	const od_int64 size = File::getFileSize( fname );
	totalsz += size;
    }

    // --beacuse getAuxFileName function is private in derived class. Not sure
    // --if it needs to be exposed. Didn't see any use-case for exposing outside
    // --this test program.
    mCalcAuxFileSize(totalsz);
    BufferString errmsg( "Calculated size: ", dssize );
    errmsg.add( "; Actual size: " ).add( sSEGYSize );
    mRunStandardTestWithError( totalsz == sSEGYSize,
			   "SEG-Y Direct dataset size calculated from Files",
			   errmsg )

    return true;
}


#define mIOObjAbsent(obj,id,msg,objtyp) \
    PtrMan<IOObj> obj = IOM().get( id ); \
    msg.setEmpty(); \
    msg.add( "Valid obj: " ) \
       .add( objtyp ); \
    mRunStandardTestWithError( obj.ptr(), msg, IOM().message() )


mLoad1Module( "Seis" )

bool BatchProgram::doWork( od_ostream& strm )
{
    mInitBatchTestProg();
    bool alltestspassed = true;
    MultiID cbvsid, zcbvsid, twodid, sgydirid;
    mRunStandardTest(pars().get(sKeyCBVSID(), cbvsid),
		     "CBVS volume multiID found")
    mRunStandardTest(pars().get(sKeyCBVSZSliceID(), zcbvsid),
		     "Z-optimized CBVS vol multiID found")
    mRunStandardTest(pars().get(sKey2DID(), twodid),
		     "2D multiID found")
    mRunStandardTest(pars().get(sKeySEGYDirID(), sgydirid),
		     "SEG-Y Direct multiID found")

    BufferString ioobjmsg;
    mIOObjAbsent(cbvsobj,cbvsid,ioobjmsg,"CBVS")
    mIOObjAbsent(cbvszsobj,zcbvsid,ioobjmsg,"Z-Slice CBVS")
    mIOObjAbsent(twodobj,twodid,ioobjmsg,"2D")
    mIOObjAbsent(sgydirobj,sgydirid,ioobjmsg,"SEG-Y Direct")

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
