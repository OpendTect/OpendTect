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
#include "envvars.h"
#include "file.h"
#include "ioman.h"
#include "moddepmgr.h"
#include "multiid.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seisparallelreader.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "segydirectdef.h"
#include "segydirecttr.h"

static const char* sKeyCBVSID()			{ return "CBVS"; }
static const char* sKeyCBVSZSliceID()		{ return "CBVS_Z_Slice"; }
static const char* sKey2DID()			{ return "2D"; }
static const char* sKeySEGYDirID()		{ return "SEGYDirect"; }

static const int sCBVSSize			= 21102400;
static const int sZSliceCBVSSize		= 254756;
static const int s2DSize			= 10899228;
static const int sSEGYSizeImpl			= 37584;
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

    BufferStringSet linkedfnames;
    sttr->getAllFileNames( linkedfnames );
    mRunStandardTest( linkedfnames.size() == 2, "Number of files" );

    linkedfnames.setEmpty();
    sttr->getAllFileNames( linkedfnames, true );
    mRunStandardTest( linkedfnames.size() == 1, "Number of files" )

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

    BufferStringSet linkedfnames;
    sttr->getAllFileNames( linkedfnames );
    mRunStandardTest( linkedfnames.size() == 2, "Number of files" );

    linkedfnames.setEmpty();
    sttr->getAllFileNames( linkedfnames, true );
    mRunStandardTest( linkedfnames.size() == 1, "Number of files" )

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
    if ( filenm.isEmpty() || !File::exists(filenm) )
	return false;

    BufferStringSet linkedfnames;
    sttr->getAllFileNames( linkedfnames );
    mRunStandardTest( linkedfnames.size() == 0, "Number of files" );

    od_int64 totalsz = 0;
    if ( File::isDirectory(filenm) )
    {
	const DirList dl( filenm.buf(), File::DirListType::FilesInDir );
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    const FilePath filepath = dl.fullPath( idx );
	    totalsz += File::getFileSize( filepath.fullPath() );
	}
    }
    else
	totalsz += File::getFileSize( filenm );

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
    dserrmsg.add( "; Actual size: " ).add( sSEGYSizeImpl );
    mRunStandardTestWithError( dssize == sSEGYSizeImpl,
			       "SEG-Y Direct dataset size from SeisIOObjInfo",
			       dserrmsg )

    BufferStringSet linkedfnames;
    sttr->getAllFileNames( linkedfnames );
    mRunStandardTest( linkedfnames.size() == 3, "Number of files" )
    // Should be 4, must include the .hdr file

    linkedfnames.setEmpty();
    sttr->getAllFileNames( linkedfnames, true );
    mRunStandardTest( linkedfnames.size() == 2, "Number of files" )

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

    BufferString errmsg( "Calculated size: ", dssize );
    errmsg.add( "; Actual size: " ).add( sSEGYSize );
    mRunStandardTestWithError( totalsz == sSEGYSize,
			   "SEG-Y Direct dataset size calculated from Files",
			   errmsg )

    return true;
}


bool validDP( const RegularSeisDataPack* dp, const TrcKeyZSampling& tkzs,
	      BufferString& msg )
{
    if ( !dp )
    {
	msg = "No output datapack";
	return false;
    }

    if ( dp->isEmpty() )
    {
	msg = "Datapack is empty";
	return false;
    }

    if ( dp->nrComponents() != 1 )
    {
	msg = "Incorrect number of output components";
	return false;
    }

    const TrcKeyZSampling& dptkzs = dp->sampling();
    if ( dptkzs != tkzs )
    {
	msg = "Incorrect output sampling";
	return false;
    }

    return true;
}


bool testCBVS3DSeisTrcReader( const IOObj& obj, const TrcKeyZSampling& tkzs )
{
    SeisTrcReader volrdr( obj );
    TrcKeySampling trctkzs = tkzs.hsamp_;
    trctkzs.set( TrcKey(BinID(232,728)) );
    volrdr.setSelData( new Seis::RangeSelData(trctkzs) );
    SeisTrc trc;
    mRunStandardTestWithError( volrdr.get(trc.info()) == 1,
			       "Fetch trc info", volrdr.errMsg().getString() )
    mRunStandardTestWithError( volrdr.get(trc),
			       "Fetch trc data", volrdr.errMsg().getString() )
    mRunStandardTest( mIsEqual(trc.getValue(0.756f,0),-263.f,1e-2f),
			 "Sample amplitude from SeisTrcReader::get" )

    SetEnvVar( "OD_ENABLE_TRANSLATOR_DATAPACK", "Yes" );

    BufferString msg;
    TrcKeyZSampling voltkzs = tkzs;
    voltkzs.hsamp_.expand( -2, -2 );
    voltkzs.zsamp_.start_ = tkzs.zsamp_.atIndex( 2 );
    voltkzs.zsamp_.stop_ = tkzs.zsamp_.atIndex( tkzs.nrZ()-2 );
    volrdr.setSelData( new Seis::RangeSelData(voltkzs) );
    RefMan<RegularSeisDataPack> voldp = new RegularSeisDataPack(
				VolumeDataPack::categoryStr(voltkzs) );
    mRunStandardTestWithError( volrdr.getDataPack( *voldp.ptr() ),
			       "Read a sub-volume using the SeisTrcReader",
				volrdr.errMsg().getString() )
    mRunStandardTestWithError( validDP( voldp.ptr(), voltkzs, msg),
			       "Sub-volume datapack shape", msg )
    mRunStandardTest( mIsEqual(voldp->data().get(14,12,87),-263.f,1e-2f),
		      "Sample amplitude within a sub-volume" )

    TrcKeyZSampling inltkzs = voltkzs;
    inltkzs.hsamp_.setLineRange( Interval<int>( 232, 232 ) );
    SeisTrcReader inlrdr( obj );
    inlrdr.setSelData( new Seis::RangeSelData(inltkzs) );
    RefMan<RegularSeisDataPack> inldp = new RegularSeisDataPack(
				VolumeDataPack::categoryStr(inltkzs) );
    mRunStandardTestWithError( inlrdr.getDataPack( *inldp.ptr() ),
			       "Read an inline using the SeisTrcReader",
				inlrdr.errMsg().getString() )
    mRunStandardTestWithError( validDP( inldp.ptr(), inltkzs, msg),
			       "Inline slice datapack shape", msg )
    mRunStandardTest( mIsEqual(inldp->data().get(0,12,87),-263.f,1e-2f),
		      "Sample amplitude within inline slice" )

    TrcKeyZSampling crltkzs = voltkzs;
    crltkzs.hsamp_.setTrcRange( Interval<int>( 728, 728 ) );
    SeisTrcReader crlrdr( obj );
    crlrdr.setSelData( new Seis::RangeSelData(crltkzs) );
    RefMan<RegularSeisDataPack> crldp = new RegularSeisDataPack(
				VolumeDataPack::categoryStr(crltkzs) );
    mRunStandardTestWithError( crlrdr.getDataPack( *crldp.ptr() ),
			       "Read a crossline using the SeisTrcReader",
				crlrdr.errMsg().getString() )
    mRunStandardTestWithError( validDP( crldp.ptr(), crltkzs, msg ),
			       "Crossline slice datapack shape", msg )
    mRunStandardTest( mIsEqual(crldp->data().get(14,0,87),-263.f,1e-2f),
		      "Sample amplitude within crossline slice" )

    TrcKeyZSampling ztkzs = voltkzs;
    ztkzs.zsamp_.setInterval( Interval<float>( 0.756f, 0.756f ) );
    SeisTrcReader zrdr( obj );
    zrdr.setSelData( new Seis::RangeSelData(ztkzs) );
    RefMan<RegularSeisDataPack> zdp = new RegularSeisDataPack(
				VolumeDataPack::categoryStr(ztkzs) );
    mRunStandardTestWithError( zrdr.getDataPack( *zdp.ptr() ),
			       "Read a Z-slice using the SeisTrcReader",
				zrdr.errMsg().getString() )
    mRunStandardTestWithError( validDP( zdp.ptr(), ztkzs, msg ),
			       "Z slice datapack shape", msg )
    mRunStandardTest( mIsEqual(zdp->data().get(14,12,0),-263.f,1e-2f),
		      "Sample amplitude within z-slice" )

    return true;
}


bool testCBVS2DSeisTrcReader( const IOObj& obj, const TrcKeyZSampling& tkzs )
{
    SeisTrcReader linerdr( obj );
    TrcKeySampling trctkzs = tkzs.hsamp_;
    trctkzs.set( TrcKey(tkzs.hsamp_.getGeomID(),500) );
    linerdr.setSelData( new Seis::RangeSelData(trctkzs) );
    SeisTrc trc;
    mRunStandardTestWithError( linerdr.get(trc.info()) == 1,
			       "Fetch trc info", linerdr.errMsg().getString() )
    mRunStandardTestWithError( linerdr.get(trc),
			       "Fetch trc data", linerdr.errMsg().getString() )
    mRunStandardTest( mIsEqual(trc.getValue(0.756f,0),-503.f,1e-2f),
		      "Sample amplitude from SeisTrcReader::get" )

    return true;

    /* TODO enable when implemented
    BufferString msg;
    TrcKeyZSampling linetkzs = tkzs;
    linetkzs.hsamp_.expand( 0, -2 );
    linetkzs.zsamp_.start_ = 0.308f;
    linetkzs.zsamp_.stop_ = 1.392f;
    linerdr.setSelData( new Seis::RangeSelData(linetkzs) );
    RefMan<RegularSeisDataPack> linedp = new RegularSeisDataPack(
				VolumeDataPack::categoryStr(linetkzs) );
    mRunStandardTestWithError( linerdr.getDataPack( *linedp.ptr() ),
			       "Read a sub-line using the SeisTrcReader",
				linerdr.errMsg().getString() )
    mRunStandardTestWithError( validDP( linedp.ptr(), linetkzs, msg),
			       "Sub-line datapack shape", msg )
    mRunStandardTest( mIsEqual(linedp->data().get(0,493,112),-503.f,1e-2f),
		      "Sample amplitude within a sub-line" )

    TrcKeyZSampling trcstkzs = linetkzs;
    trcstkzs.hsamp_.setTrcRange( Interval<int>( 500, 500 ) );
    SeisTrcReader trcrdr( obj );
    trcrdr.setSelData( new Seis::RangeSelData(trcstkzs) );
    RefMan<RegularSeisDataPack> trcdp = new RegularSeisDataPack(
				VolumeDataPack::categoryStr(trcstkzs) );
    mRunStandardTestWithError( trcrdr.getDataPack( *trcdp.ptr() ),
			       "Read a crossline using the SeisTrcReader",
				trcrdr.errMsg().getString() )
    mRunStandardTestWithError( validDP( trcdp.ptr(), trcstkzs, msg ),
			       "Crossline slice datapack shape", msg )
    mRunStandardTest( mIsEqual(trcdp->data().get(0,0,112),-503.f,1e-2f),
		      "Sample amplitude within crossline slice" )

    TrcKeyZSampling ztkzs = linetkzs;
    ztkzs.zsamp_.setInterval( Interval<float>( 0.756f, 0.756f ) );
    SeisTrcReader zrdr( obj );
    zrdr.setSelData( new Seis::RangeSelData(ztkzs) );
    RefMan<RegularSeisDataPack> zdp = new RegularSeisDataPack(
				VolumeDataPack::categoryStr(ztkzs) );
    mRunStandardTestWithError( zrdr.getDataPack( *zdp.ptr() ),
			       "Read a Z-slice using the SeisTrcReader",
				zrdr.errMsg().getString() )
    mRunStandardTestWithError( validDP( zdp.ptr(), ztkzs, msg ),
			       "Z slice datapack shape", msg )
    mRunStandardTest( mIsEqual(zdp->data().get(0,493,0),-503.f,1e-2f),
		      "Sample amplitude within z-slice" )

    return true;
    */
}


bool testSEGYSeisTrcReader( const IOObj& obj, const TrcKeyZSampling& tkzs )
{
    SeisTrcReader volrdr( obj );
    TrcKeySampling trctkzs = tkzs.hsamp_;
    trctkzs.set( TrcKey(BinID(206,706)) );
    volrdr.setSelData( new Seis::RangeSelData(trctkzs) );
    SeisTrc trc;
    mRunStandardTestWithError( volrdr.get(trc.info()) == 1,
			       "Fetch trc info", volrdr.errMsg().getString() )
    mRunStandardTestWithError( volrdr.get(trc),
			       "Fetch trc data", volrdr.errMsg().getString() )
    mRunStandardTest( mIsEqual(trc.getValue(0.756f,0),-589.f,1e-2f),
		      "Sample amplitude from SeisTrcReader::get" )

    return true;

    /* TODO Enable when SEGYDirectSeisTrcTranslator::getDataPack is implemented:
    BufferString msg;
    TrcKeyZSampling voltkzs = tkzs;
    voltkzs.hsamp_.expand( -1, -1 );
    voltkzs.zsamp_.start_ = tkzs.zsamp_.atIndex( 2 );
    voltkzs.zsamp_.stop_ = tkzs.zsamp_.atIndex( tkzs.nrZ()-2 );
    volrdr.setSelData( new Seis::RangeSelData(voltkzs) );

    RefMan<RegularSeisDataPack> voldp = new RegularSeisDataPack(
				VolumeDataPack::categoryStr(voltkzs) );
    mRunStandardTestWithError( volrdr.getDataPack( *voldp.ptr() ),
			       "Read a sub-volume using the SeisTrcReader",
				volrdr.errMsg().getString() )
    mRunStandardTestWithError( validDP( voldp.ptr(), voltkzs, msg),
			       "Sub-volume datapack shape", msg )
    mRunStandardTest( mIsEqual(voldp->data().get(2,2,87),-589.f,1e-2f),
		      "Sample amplitude within a sub-volume" )

    TrcKeyZSampling inltkzs = voltkzs;
    inltkzs.hsamp_.setLineRange( Interval<int>( 206, 206 ) );
    SeisTrcReader inlrdr( obj );
    inlrdr.setSelData( new Seis::RangeSelData(inltkzs) );
    RefMan<RegularSeisDataPack> inldp = new RegularSeisDataPack(
				VolumeDataPack::categoryStr(inltkzs) );
    mRunStandardTestWithError( inlrdr.getDataPack( *inldp.ptr() ),
			       "Read an inline using the SeisTrcReader",
				inlrdr.errMsg().getString() )
    mRunStandardTestWithError( validDP( inldp.ptr(), inltkzs, msg),
			       "Inline slice datapack shape", msg )
    mRunStandardTest( mIsEqual(inldp->data().get(0,2,87),-589.f,1e-2f),
		      "Sample amplitude within inline slice" )

    TrcKeyZSampling crltkzs = voltkzs;
    crltkzs.hsamp_.setTrcRange( Interval<int>( 706, 706 ) );
    SeisTrcReader crlrdr( obj );
    crlrdr.setSelData( new Seis::RangeSelData(crltkzs) );
    RefMan<RegularSeisDataPack> crldp = new RegularSeisDataPack(
				VolumeDataPack::categoryStr(crltkzs) );
    mRunStandardTestWithError( crlrdr.getDataPack( *crldp.ptr() ),
			       "Read a crossline using the SeisTrcReader",
				crlrdr.errMsg().getString() )
    mRunStandardTestWithError( validDP( crldp.ptr(), crltkzs, msg ),
			       "Crossline slice datapack shape", msg )
    mRunStandardTest( mIsEqual(crldp->data().get(2,0,87),-589.f,1e-2f),
		      "Sample amplitude within crossline slice" )

    TrcKeyZSampling ztkzs = voltkzs;
    ztkzs.zsamp_.setInterval( Interval<float>( 0.756f, 0.756f ) );
    SeisTrcReader zrdr( obj );
    zrdr.setSelData( new Seis::RangeSelData(ztkzs) );
    RefMan<RegularSeisDataPack> zdp = new RegularSeisDataPack(
				VolumeDataPack::categoryStr(ztkzs) );
    mRunStandardTestWithError( zrdr.getDataPack( *zdp.ptr() ),
			       "Read a Z-slice using the SeisTrcReader",
				zrdr.errMsg().getString() )
    mRunStandardTestWithError( validDP( zdp.ptr(), ztkzs, msg ),
			       "Z slice datapack shape", msg )
    mRunStandardTest( mIsEqual(zdp->data().get(2,2,0),-589.f,1e-2f),
		      "Sample amplitude within z-slice" )

    return true;*/
}


bool testCBVS3DSeisSequentialReader( const IOObj& obj,
				     const TrcKeyZSampling& tkzs )
{
    TrcKeyZSampling voltkzs = tkzs;
    voltkzs.hsamp_.expand( -2, -2 );
    voltkzs.zsamp_.start_ = tkzs.zsamp_.atIndex( 2 );
    voltkzs.zsamp_.stop_ = tkzs.zsamp_.atIndex( tkzs.nrZ()-2 );
    BufferString msg;

    Seis::SequentialReader volrdr( obj, &voltkzs );
    mRunStandardTestWithError( volrdr.execute(),
			  "Read a sub-volume using the Seis::SequentialReader",
			  volrdr.uiMessage().getString() )
    ConstRefMan<RegularSeisDataPack> voldp = volrdr.getDataPack();
    mRunStandardTestWithError( validDP( voldp.ptr(), voltkzs, msg),
			       "Sub-volume datapack shape", msg )
    mRunStandardTest( mIsEqual(voldp->data().get(14,12,87),-263.f,1e-2f),
		      "Sample amplitude within sub-volume slice" )

    TrcKeyZSampling inltkzs = voltkzs;
    inltkzs.hsamp_.setLineRange( Interval<int>( 232, 232 ) );
    Seis::SequentialReader inlrdr( obj, &inltkzs );
    mRunStandardTestWithError( inlrdr.execute(),
			      "Read an inline using the Seis::SequentialReader",
			      inlrdr.uiMessage().getString() )
    ConstRefMan<RegularSeisDataPack> inldp = inlrdr.getDataPack();
    mRunStandardTestWithError( validDP( inldp.ptr(), inltkzs, msg),
			       "Inline slice datapack shape", msg )
    mRunStandardTest( mIsEqual(inldp->data().get(0,12,87),-263.f,1e-2f),
		      "Sample amplitude within inline slice" )

    TrcKeyZSampling crltkzs = voltkzs;
    crltkzs.hsamp_.setTrcRange( Interval<int>( 728, 728 ) );
    Seis::SequentialReader crlrdr( obj, &crltkzs );
    mRunStandardTestWithError( crlrdr.execute(),
			"Read a crossline using the Seis::SequentialReader",
			crlrdr.uiMessage().getString() )
    ConstRefMan<RegularSeisDataPack> crldp = crlrdr.getDataPack();
    mRunStandardTestWithError( validDP( crldp.ptr(), crltkzs, msg ),
			       "Crossline slice datapack shape", msg )
    mRunStandardTest( mIsEqual(crldp->data().get(14,0,87),-263.f,1e-2f),
		      "Sample amplitude within crossline slice" )

    TrcKeyZSampling ztkzs = voltkzs;
    ztkzs.zsamp_.setInterval( Interval<float>( 0.756f, 0.756f ) );
    Seis::SequentialReader zrdr( obj, &ztkzs );
    mRunStandardTestWithError( zrdr.execute(),
			"Read a Z-slice using the Seis::SequentialReader",
			zrdr.uiMessage().getString() )
    ConstRefMan<RegularSeisDataPack> zdp = zrdr.getDataPack();
    mRunStandardTestWithError( validDP( zdp.ptr(), ztkzs, msg ),
			       "Z slice datapack shape", msg )
    mRunStandardTest( mIsEqual(zdp->data().get(14,12,0),-263.f,1e-2f),
		      "Sample amplitude within z-slice" )

    return true;
}


bool testCBVS2DSeisSequentialReader( const IOObj& obj,
				     const TrcKeyZSampling& tkzs )
{
    TrcKeyZSampling linetkzs = tkzs;
    linetkzs.hsamp_.expand( 0, -2 );
    linetkzs.zsamp_.start_ = 0.308f;
    linetkzs.zsamp_.stop_ = 1.392f;
    BufferString msg;

    Seis::SequentialReader linerdr( obj, &linetkzs );
    mRunStandardTestWithError( linerdr.execute(),
			  "Read a sub-line using the Seis::SequentialReader",
			  linerdr.uiMessage().getString() )
    ConstRefMan<RegularSeisDataPack> linedp = linerdr.getDataPack();
    mRunStandardTestWithError( validDP( linedp.ptr(), linetkzs, msg),
			       "Sub-line datapack shape", msg )
    mRunStandardTest( mIsEqual(linedp->data().get(0,493,112),-503.f,1e-2f),
		      "Sample amplitude within sub-line slice" )

    TrcKeyZSampling trctkzs = linetkzs;
    trctkzs.hsamp_.setTrcRange( Interval<int>( 500, 500 ) );
    Seis::SequentialReader trcrdr( obj, &trctkzs );
    mRunStandardTestWithError( trcrdr.execute(),
			"Read a trace using the Seis::SequentialReader",
			trcrdr.uiMessage().getString() )
    ConstRefMan<RegularSeisDataPack> trcdp = trcrdr.getDataPack();
    mRunStandardTestWithError( validDP( trcdp.ptr(), trctkzs, msg ),
			       "Crossline slice datapack shape", msg )
    mRunStandardTest( mIsEqual(trcdp->data().get(0,0,112),-503.f,1e-2f),
		      "Sample amplitude within crossline slice" )

    TrcKeyZSampling ztkzs = linetkzs;
    ztkzs.zsamp_.setInterval( Interval<float>( 0.756f, 0.756f ) );
    Seis::SequentialReader zrdr( obj, &ztkzs );
    mRunStandardTestWithError( zrdr.execute(),
			"Read a Z-slice using the Seis::SequentialReader",
			zrdr.uiMessage().getString() )
    ConstRefMan<RegularSeisDataPack> zdp = zrdr.getDataPack();
    mRunStandardTestWithError( validDP( zdp.ptr(), ztkzs, msg ),
			       "Z slice datapack shape", msg )
    mRunStandardTest( mIsEqual(zdp->data().get(0,493,0),-503.f,1e-2f),
		      "Sample amplitude within z-slice" )

    return true;
}


bool testSEGYSeisSequentialReader( const IOObj& obj,
				   const TrcKeyZSampling& tkzs )
{
    TrcKeyZSampling voltkzs = tkzs;
    voltkzs.hsamp_.expand( -1, -1 );
    voltkzs.zsamp_.start_ = tkzs.zsamp_.atIndex( 2 );
    voltkzs.zsamp_.stop_ = tkzs.zsamp_.atIndex( tkzs.nrZ()-2 );
    BufferString msg;

    Seis::SequentialReader volrdr( obj, &voltkzs );
    mRunStandardTestWithError( volrdr.execute(),
			  "Read a sub-volume using the Seis::SequentialReader",
			  volrdr.uiMessage().getString() )
    ConstRefMan<RegularSeisDataPack> voldp = volrdr.getDataPack();
    mRunStandardTestWithError( validDP( voldp.ptr(), voltkzs, msg),
			       "Sub-volume datapack shape", msg )
    mRunStandardTest( mIsEqual(voldp->data().get(2,2,87),-589.f,1e-2f),
		      "Sample amplitude within sub-volume slice" )

    TrcKeyZSampling inltkzs = voltkzs;
    inltkzs.hsamp_.setLineRange( Interval<int>( 206, 206 ) );
    Seis::SequentialReader inlrdr( obj, &inltkzs );
    mRunStandardTestWithError( inlrdr.execute(),
			      "Read an inline using the Seis::SequentialReader",
			      inlrdr.uiMessage().getString() )
    ConstRefMan<RegularSeisDataPack> inldp = inlrdr.getDataPack();
    mRunStandardTestWithError( validDP( inldp.ptr(), inltkzs, msg),
			       "Inline slice datapack shape", msg )
    mRunStandardTest( mIsEqual(inldp->data().get(0,2,87),-589.f,1e-2f),
		      "Sample amplitude within inline slice" )

    TrcKeyZSampling crltkzs = voltkzs;
    crltkzs.hsamp_.setTrcRange( Interval<int>( 706, 706 ) );
    Seis::SequentialReader crlrdr( obj, &crltkzs );
    mRunStandardTestWithError( crlrdr.execute(),
			"Read a crossline using the Seis::SequentialReader",
			crlrdr.uiMessage().getString() )
    ConstRefMan<RegularSeisDataPack> crldp = crlrdr.getDataPack();
    mRunStandardTestWithError( validDP( crldp.ptr(), crltkzs, msg ),
			       "Crossline slice datapack shape", msg )
    mRunStandardTest( mIsEqual(crldp->data().get(2,0,87),-589.f,1e-2f),
		      "Sample amplitude within crossline slice" )

    TrcKeyZSampling ztkzs = voltkzs;
    ztkzs.zsamp_.setInterval( Interval<float>( 0.756f, 0.756f ) );
    Seis::SequentialReader zrdr( obj, &ztkzs );
    mRunStandardTestWithError( zrdr.execute(),
			"Read a Z-slice using the Seis::SequentialReader",
			zrdr.uiMessage().getString() )
    ConstRefMan<RegularSeisDataPack> zdp = zrdr.getDataPack();
    mRunStandardTestWithError( validDP( zdp.ptr(), ztkzs, msg ),
			       "Z slice datapack shape", msg )
    mRunStandardTest( mIsEqual(zdp->data().get(2,2,0),-589.f,1e-2f),
		      "Sample amplitude within z-slice" )

    return true;
}


bool testCBVSRead3D( const IOObj& obj )
{
    const SeisIOObjInfo info( obj );
    TrcKeyZSampling tkzs;
    mRunStandardTest( info.getRanges( tkzs ) &&
		      tkzs.hsamp_.start_.inl() == 200 &&
		      tkzs.hsamp_.start_.crl() == 700 &&
		      tkzs.hsamp_.stop_.inl() == 650 &&
		      tkzs.hsamp_.stop_.crl() == 1200 &&
		      tkzs.hsamp_.step_.inl() == 2 &&
		      tkzs.hsamp_.step_.crl() == 2 &&
		      mIsEqual(tkzs.zsamp_.start_,0.400f,1e-6f) &&
		      mIsEqual(tkzs.zsamp_.stop_,1.100f,1e-6f) &&
		      mIsEqual(tkzs.zsamp_.step_,0.004f,1e-6f)
		      , "Get CBVS 3D ranges" );

    return testCBVS3DSeisSequentialReader( obj, tkzs ) &&
	   testCBVS3DSeisTrcReader( obj, tkzs );
}


bool testCBVSRead3DZ( const IOObj& obj )
{
    const SeisIOObjInfo info( obj );
    TrcKeyZSampling tkzs;
    mRunStandardTest( info.getRanges( tkzs ) &&
		      tkzs.hsamp_.start_.inl() == 200 &&
		      tkzs.hsamp_.start_.crl() == 700 &&
		      tkzs.hsamp_.stop_.inl() == 250 &&
		      tkzs.hsamp_.stop_.crl() == 750 &&
		      tkzs.hsamp_.step_.inl() == 2 &&
		      tkzs.hsamp_.step_.crl() == 2 &&
		      mIsEqual(tkzs.zsamp_.start_,0.400f,1e-6f) &&
		      mIsEqual(tkzs.zsamp_.stop_,1.100f,1e-6f) &&
		      mIsEqual(tkzs.zsamp_.step_,0.004f,1e-6f)
		      , "Get CBVS 3D ranges" );

    return testCBVS3DSeisSequentialReader( obj, tkzs ) &&
	   testCBVS3DSeisTrcReader( obj, tkzs );
}


bool testCBVSRead2D( const IOObj& obj )
{
    const Pos::GeomID geomid( 11 ); //Strike 5
    const SeisIOObjInfo info( obj );
    TrcKeyZSampling tkzs( geomid );
    mRunStandardTest( info.getRanges( tkzs ) &&
		      tkzs.hsamp_.getGeomID() == geomid &&
		      tkzs.hsamp_.start_.crl() == 5 &&
		      tkzs.hsamp_.stop_.crl() == 679 &&
		      tkzs.hsamp_.step_.crl() == 1 &&
		      mIsEqual(tkzs.zsamp_.start_,0.f,1e-6f) &&
		      mIsEqual(tkzs.zsamp_.stop_,1.848f,1e-6f) &&
		      mIsEqual(tkzs.zsamp_.step_,0.004f,1e-6f)
		      , "Get CBVS 2D ranges" );

    return testCBVS2DSeisSequentialReader( obj, tkzs ) &&
	   testCBVS2DSeisTrcReader( obj, tkzs );
}


bool testSEGYRead( const IOObj& obj )
{
    const SeisIOObjInfo info( obj );
    TrcKeyZSampling tkzs;
    mRunStandardTest( info.getRanges( tkzs ) &&
		      tkzs.hsamp_.start_.inl() == 200 &&
		      tkzs.hsamp_.start_.crl() == 700 &&
		      tkzs.hsamp_.stop_.inl() == 210 &&
		      tkzs.hsamp_.stop_.crl() == 710 &&
		      tkzs.hsamp_.step_.inl() == 2 &&
		      tkzs.hsamp_.step_.crl() == 2 &&
		      mIsEqual(tkzs.zsamp_.start_,0.400f,1e-6f) &&
		      mIsEqual(tkzs.zsamp_.stop_,1.100f,1e-6f) &&
		      mIsEqual(tkzs.zsamp_.step_,0.004f,1e-6f)
		      , "Get SEG-Y 3D ranges" );

    return testSEGYSeisSequentialReader( obj, tkzs ) &&
	   testSEGYSeisTrcReader( obj, tkzs );
}

#define mIOObjAbsent(obj,id,msg,objtyp) \
    PtrMan<IOObj> obj = IOM().get( id ); \
    msg.setEmpty(); \
    msg.add( "Valid obj: " ) \
       .add( objtyp ); \
    mRunStandardTestWithError( obj.ptr(), msg, IOM().message() )


mLoad1Module( "Seis" )

bool BatchProgram::doWork( od_ostream& /*strm*/ )
{
    mInitBatchTestProg();
    od_ostream& strm = logStream();

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

    if ( !testCBVSSize(*cbvsobj,strm) ||
	 !testZSliceCBVSSize(*cbvszsobj,strm) ||
	 !test2DSize(*twodobj,strm) ||
	 !testSEGYDirectSize(*sgydirobj,strm) ||
	 !testCBVSRead3D(*cbvsobj) ||
	 !testCBVSRead3DZ(*cbvszsobj) ||
	 !testCBVSRead2D(*twodobj) ||
	 !testSEGYRead(*sgydirobj) )
	return false;

    return true;
}
