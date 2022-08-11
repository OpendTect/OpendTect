/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2010
________________________________________________________________________

-*/

#include "posinfo2dsurv.h"

#include "ascstream.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "oddirs.h"
#include "safefileio.h"
#include "settings.h"
#include "perthreadrepos.h"
#include "oscommand.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "timefun.h"

#define mIdxTyp PosInfo::Survey2D::IdxType


static const char* sIdxFilename = "idx.txt";
static const char* sKeyStor = "Storage";
static const char* sKeyMaxID = "Max ID";
static const char* sKeyTrcDist = "Inter-trace Distance";
static const PosInfo::Line2DKey udfl2dkey(
			mUdf(IdxPair::IdxType), mUdf(IdxPair::IdxType) );
static bool cWriteAscii = false;
static PosInfo::Survey2D* s2dpos_inst = 0;
namespace PosInfo { struct Survey2DDeleter : public NamedCallBacker {
	void doDel( CallBacker* ) { delete s2dpos_inst; s2dpos_inst = 0; } }; }


const PosInfo::Line2DKey& PosInfo::Line2DKey::udf()
{ return udfl2dkey; }

bool PosInfo::Line2DKey::isUdf() const
{ return *this == udf(); }


bool PosInfo::Line2DKey::haveLSID() const
{
    return first >= 0 && !mIsUdf(first);
}


bool PosInfo::Line2DKey::haveLineID() const
{
    return second >= 0 && !mIsUdf(second);
}


void PosInfo::Survey2D::initClass()
{
    cWriteAscii = Settings::common().isTrue("2DGeometry.Write Ascii");
}


bool PosInfo::Line2DKey::isOK() const
{
    return S2DPOS().hasLine( lineID(), lsID() );
}



const char* PosInfo::Line2DKey::toString() const
{
    return IdxPair::getUsrStr( "", ".", "", false );
}


bool PosInfo::Line2DKey::fromString( const char* str )
{
    return IdxPair::parseUsrStr( str, "", ".", "" );
}


const PosInfo::Survey2D& S2DPOS()
{
    if ( !s2dpos_inst )
    {
	s2dpos_inst = new PosInfo::Survey2D;
	mDefineStaticLocalObject( PosInfo::Survey2DDeleter, s2dd, );
	const_cast<SurveyInfo&>(SI()).objectToBeDeleted().notify(
			mCB(&s2dd,PosInfo::Survey2DDeleter,doDel) );
    }
    return *s2dpos_inst;
}


#define mErrRet(s1,s2,s3) \
{ \
    BufferString msg( s1, " ", s2 ); \
    msg.add( " " ).add( s3 ); \
    OD::DisplayErrorMessage( msg ); \
    return; \
}


PosInfo::Survey2D::Survey2D()
    : basefp_(*new FilePath(GetDataDir()))
    , lsfp_(*new FilePath)
    , lsindex_(*new IOPar("Line Sets"))
    , lineindex_(*new IOPar("Lines"))
{
    if ( !File::exists(basefp_.fullPath()) )
	return;

    basefp_.add( "2DGeom" );
    const BufferString dirnm = basefp_.fullPath();
    if ( File::exists(dirnm) )
    {
	readIdxFiles();
	if ( isEmpty() )
	    File::remove( dirnm );
    }
}


PosInfo::Survey2D::~Survey2D()
{
    delete &basefp_; delete &lsfp_;
    delete &lsindex_; delete &lineindex_;
}


void PosInfo::Survey2D::readIdxFiles()
{
    if ( lsnm_.isEmpty() )
    {
	readIdxFile( FilePath(basefp_,sIdxFilename).fullPath(), lsindex_ );
	if ( lsindex_.size() <= 1 )
	    return;
	lsnm_ = lsindex_.getKey(0);
    }
    int idxky = lsindex_.indexOf( lsnm_.buf() );
    if ( idxky < 0 )
    {	// selected lsnm_ doesn't exist (anymore): reset to default
	BufferString lsnm = lsnm_;
	lsindex_.setEmpty(); lineindex_.setEmpty(); lsnm_.setEmpty();
	readIdxFiles();
	idxky = lsindex_.indexOf( lsnm.buf() );
	if ( idxky < 0 ) return;
    }

    lsfp_ = basefp_;
    FileMultiString fms( lsindex_.getValue(idxky) );
    lsfp_.add( fms[0] );
    readIdxFile( FilePath(lsfp_,sIdxFilename).fullPath(), lineindex_ );
}


struct IdxFileData
{
    BufferString	filename_;
    od_int64		timestamp_;
    IOPar		iopar_;
};

static ManagedObjectSet<IdxFileData> idxfilecache;
static Threads::Lock idxfilecachelock( Threads::Lock::MultiRead );

static int getIdxFileCacheIdx( const char* fnm )
{
    for ( int idx=idxfilecache.size()-1; idx>=0; idx-- )
    {
	if ( idxfilecache[idx]->filename_ == fnm )
	    return idx;
    }
    return -1;
}


#define mGetWriteLocker(var) \
    Threads::Locker var( idxfilecachelock, Threads::Locker::WriteLock )


void PosInfo::Survey2D::readIdxFile( const char* fnm, IOPar& iop )
{
    Threads::Locker rdlocker( idxfilecachelock );
    int idx = getIdxFileCacheIdx( fnm );
    if ( idx>=0 && File::getTimeInSeconds(fnm)<=idxfilecache[idx]->timestamp_ )
	{ iop = idxfilecache[idx]->iopar_; return; }
    rdlocker.unlockNow();

    iop.setEmpty();
    SafeFileIO sfio( fnm, true );
    if ( !sfio.open(true) ) return;
    ascistream astrm( sfio.istrm() );
    iop.getFrom( astrm );

    mGetWriteLocker( wrlocker );
    idx = getIdxFileCacheIdx( fnm );
    if ( idx >= 0 )
	delete idxfilecache.removeSingle( idx );

    IdxFileData* idxfiledata = new IdxFileData();
    idxfiledata->filename_ = fnm;
    idxfiledata->timestamp_ = File::getTimeInSeconds( fnm );
    idxfiledata->iopar_ = iop;
    idxfilecache += idxfiledata;

    sfio.closeSuccess();
}


void PosInfo::Survey2D::writeIdxFile( bool lines ) const
{
    FilePath fp( lines ? lsfp_ : basefp_ , sIdxFilename );
    SafeFileIO sfio( fp.fullPath(), true );
    if ( !sfio.open(false) )
	mErrRet( "Cannot open 2D Geometry index file", fp.fullPath(),
		 "for write. Check disk space and file permissions." )

    ascostream astrm( sfio.ostrm() );
    astrm.putHeader("File Name Table");
    (lines ? lineindex_ : lsindex_).putTo( astrm );
    if ( astrm.isOK() )
    {
	mGetWriteLocker( wrlocker );
	int idx = getIdxFileCacheIdx( fp.fullPath() );
	if ( idx >= 0 )
	    idxfilecache[idx]->timestamp_ = -1;

	sfio.closeSuccess();
    }
    else
    {
	sfio.closeFail();
	mErrRet("Error during write to 2D Geometry index file",fp.fullPath(),
		"Check disk space.");
    }
}


void PosInfo::Survey2D::updateMaxID( mIdxTyp maxid, IOPar& par )
{
    par.set( sKeyMaxID, maxid );
}


void PosInfo::Survey2D::getKeys( const IOPar& iop, BufferStringSet& nms ) const
{
    nms.erase();
    BufferString maxidkey( sKeyMaxID );

    for ( int idx=0; idx<iop.size(); idx++ )
    {
	if ( maxidkey != iop.getKey(idx) )
	    nms.add( iop.getKey(idx) );
    }
}


void PosInfo::Survey2D::getIDs( const IOPar& iop, TypeSet<mIdxTyp>& ids ) const
{
    ids.erase();
    for ( int idx=0; idx<iop.size(); idx++ )
    {
	FileMultiString info( iop.getValue(idx) );
	if ( info.size() == 1 )
	    continue;
	ids += info.getIValue( 1 );
    }
}


mIdxTyp PosInfo::Survey2D::curLineSetID() const
{
    return getLineSetID( lsnm_.buf() );
}


mIdxTyp PosInfo::Survey2D::getLineSetID( const char* lsnmstr ) const
{
    if ( !lsnmstr || !*lsnmstr )
	return Line2DKey::udf().lsID();

    StringView lsnm = lsnmstr;

    for ( int idx=0; idx<lsindex_.size(); idx++ )
    {
	FileMultiString info( lsindex_.getValue(idx) );
	if ( info.size()>0 && lsnm==lsindex_.getKey(idx) )
	    return info.getIValue( 1 );
    }

    return Line2DKey::udf().lsID();
}


mIdxTyp PosInfo::Survey2D::getLineID( const char* lnm ) const
{
    StringView linenm( lnm );
    if ( linenm.isEmpty() )
	return Line2DKey::udf().lineID();

    for ( int idx=0; idx<lineindex_.size(); idx++ )
    {
	FileMultiString info( lineindex_.getValue(idx) );
	if ( info.size()>0 && linenm==lineindex_.getKey(idx) )
	    return info.getIValue( 1 );
    }

    return Line2DKey::udf().lineID();
}


const char* PosInfo::Survey2D::getLineSet( mIdxTyp lsid ) const
{
    for ( int idx=0; idx<lsindex_.size(); idx++ )
    {
	FileMultiString info( lsindex_.getValue(idx) );
	if ( info.size()>0 && (lsid == info.getIValue(1)) )
	    return lsindex_.getKey( idx );
    }

    return 0;
}


bool PosInfo::Survey2D::hasLineSet( const char* lsnm ) const
{
    return lsindex_.hasKey( lsnm );
}


bool PosInfo::Survey2D::hasLineSet( mIdxTyp lsid ) const
{
    for ( int idx=0; idx<lsindex_.size(); idx++ )
    {
	FileMultiString info( lsindex_.getValue(idx) );
	if ( info.size()>0 && (lsid == info.getIValue(1)) )
	    return true;
    }

    return false;
}


const char* PosInfo::Survey2D::getLineName( mIdxTyp lineid ) const
{
    if ( lineid < 0 )
	return 0;

    for ( int idx=0; idx<lineindex_.size(); idx++ )
    {
	FileMultiString info( lineindex_.getValue(idx) );
	if ( info.size()>0 && (lineid == info.getIValue(1)) )
	    return lineindex_.getKey( idx );
    }

    return 0;
}


bool PosInfo::Survey2D::hasLine( const char* lnm, const char* lsnm ) const
{
    if ( !lsnm || lsnm_==lsnm )
	return lineindex_.hasKey( lnm );

    BufferStringSet nms; getLines( nms, lsnm );
    return nms.isPresent( lnm );
}



bool PosInfo::Survey2D::hasLine( mIdxTyp lineid, mIdxTyp lsid ) const
{
    if ( lsid < 0 )
	lsid = curLineSetID();

    if ( !hasLineSet(lsid) )
	return false;

    TypeSet<mIdxTyp> lineids; getLineIDs( lineids, lsid );
    return lineids.isPresent( lineid );
}


int PosInfo::Survey2D::getLineSetIdx( int lsid ) const
{
    for ( int idx=0; idx<lsindex_.size(); idx++ )
    {
	FileMultiString info( lsindex_.getValue(idx) );
	if ( info.size()>0 && info.getIValue(1) == lsid )
	    return idx;
    }

    return -1;
}


void PosInfo::Survey2D::getLines( BufferStringSet& nms, const char* lsnm ) const
{
    if ( !lsnm || lsnm_==lsnm )
    {
	getKeys(lineindex_,nms);
	return;
    }

    const int idxky = lsindex_.indexOf( lsnm );
    if ( idxky < 0 ) return;

    IOPar iop;
    FilePath fp( lsfp_ );
    FileMultiString fms( lsindex_.getValue(idxky) );
    fp.setFileName( fms[0] ); fp.add( sIdxFilename );
    readIdxFile( fp.fullPath(), iop );
    getKeys( iop, nms );
}


void PosInfo::Survey2D::getLines( BufferStringSet& nms, mIdxTyp lsid ) const
{
    if ( mIsUdf(lsid) || lsid < 0 )
	return getLines( nms, 0 );

    BufferString lsnm = getLineSet( lsid );
    getLines( nms, lsnm );
}


void PosInfo::Survey2D::getLineIDs( TypeSet<mIdxTyp>& ids, mIdxTyp lsid ) const
{
    if ( lsid >=0 && !hasLineSet(lsid) )
	return;

    if ( lsid == curLineSetID() )
	{ getIDs( lineindex_, ids ); return; }

    if ( mIsUdf(lsid) || lsid < 0 )
	lsid = curLineSetID();

    const int lsidx = getLineSetIdx( lsid );
    if ( lsidx < 0 ) return;

    IOPar iop;
    FilePath fp( lsfp_ );
    FileMultiString fms( lsindex_.getValue(lsidx) );
    fp.setFileName( fms[0] ); fp.add( sIdxFilename );
    readIdxFile( fp.fullPath(), iop );
    getIDs( iop, ids );
}


mIdxTyp PosInfo::Survey2D::getNewID( IOPar& iop )
{
    mIdxTyp savedmeaxid = -mUdf(int);
    iop.get( sKeyMaxID, savedmeaxid );
    mIdxTyp newlineidx = 0;

    if ( !iop.size() )
	return 0;

    for ( int idx=0; idx<iop.size(); idx++ )
    {
	if ( iop.getKey(idx)==sKeyMaxID )
	    continue;

	FileMultiString fms( iop.getValue(idx) );
	if ( fms.size() == 1 )
	{
	    if ( newlineidx )
		newlineidx++;
	    fms.add( newlineidx );
	    iop.set( iop.getKey(idx), fms );
	}
	else if ( newlineidx < fms.getIValue(1) )
	    newlineidx = fms.getIValue( 1 );
    }

    newlineidx++;
    if ( savedmeaxid <= newlineidx )
	savedmeaxid = newlineidx;

    return savedmeaxid;
}


BufferString PosInfo::Survey2D::getNewStorageName( const char* nm,
			const FilePath& inpfp, const IOPar& iop ) const
{
    BufferString clnnm( nm );
    int itry = 1;
    while ( true )
    {
	clnnm.clean();
	FilePath fp( inpfp, clnnm );
	if ( !File::exists(fp.fullPath()) )
	    break;

	itry++;
	clnnm = nm; clnnm += itry;
    }

    return clnnm;
}


void PosInfo::Survey2D::setCurLineSet( mIdxTyp lsid ) const
{
    if ( !hasLineSet(lsid) )
	return;

    int lsidx = getLineSetIdx( lsid );
    if ( lsidx < 0 )
	return;

    BufferString maxidkey( sKeyMaxID );
    if ( maxidkey != lsindex_.getKey(lsidx) )
	setCurLineSet( lsindex_.getKey(lsidx).str() );
}


void PosInfo::Survey2D::setCurLineSet( const char* lsnm ) const
{
    Threads::Locker lckr( lock_ );
    if ( !lsnm || !*lsnm )
	{ lineindex_.setEmpty(); return; }

    if ( lsnm_ == lsnm && !isIdxFileNew(lsnm) )
	return;

    PosInfo::Survey2D& self = *const_cast<PosInfo::Survey2D*>( this );
    self.lsnm_ = lsnm;
    self.readIdxFiles();
    curlstimestr_ = getIdxTimeStamp( lsnm_ );
    if ( lsnm_ == lsnm )
	return;

    // New line set specified
    self.lsnm_ = lsnm;
    FileMultiString driinfo( getNewStorageName(lsnm,basefp_,lsindex_) );
    driinfo.add( self.getNewID(lsindex_) );
    self.lsindex_.add( lsnm, driinfo );
    self.lineindex_.setEmpty();
    self.lsfp_ = basefp_; self.lsfp_.add( driinfo[0] );
    File::createDir( lsfp_.fullPath() );
    self.updateMaxID( driinfo.getIValue(1), lsindex_ );
    writeIdxFile( false );
    self.readIdxFiles();
    curlstimestr_ = getIdxTimeStamp( lsnm_ );
}


bool PosInfo::Survey2D::getGeometry( mIdxTyp lineid,
				     PosInfo::Line2DData& l2dd ) const
{
    for ( int idx=0; idx<lineindex_.size(); idx++ )
    {
	FileMultiString info( lineindex_.getValue(idx) );
	BufferString linename( lineindex_.getKey(idx) );
	if ( info.size()>1 && info.getIValue(1) == lineid )
	{
	    l2dd.setLineName( linename );
	    return getGeometry( l2dd );
	}
    }

    return false;
}


bool PosInfo::Survey2D::getGeometry( const Line2DKey& l2dky,
				     PosInfo::Line2DData& l2dd ) const
{
    if ( !l2dky.isOK() ) return false;

    Threads::Locker* lckr = 0;
    if ( l2dky.lsID() != S2DPOS().curLineSetID() )
    {
	lckr = new Threads::Locker( lock_ );
	S2DPOS().setCurLineSet( l2dky.lsID() );
    }

    const char* linenm = S2DPOS().getLineName( l2dky.lineID() );
    if ( !linenm )
	{ delete lckr; return false; }

    const bool ret = S2DPOS().getGeometry( l2dky.lineID(), l2dd );
    delete lckr;
    return ret;
}


bool PosInfo::Survey2D::getGeometry( PosInfo::Line2DData& l2dd ) const
{
    const int lidx = lineindex_.indexOf( l2dd.lineName().buf() );
    l2dd.setEmpty();
    if ( lidx < 0 )
	return false;

    FileMultiString fms( lineindex_.getValue(lidx) );
    SafeFileIO sfio( FilePath(lsfp_,fms[0]).fullPath() );
    if ( !sfio.open(true) )
	return false;

    ascistream astrm( sfio.istrm() ); // read header
    bool isascii = cWriteAscii;
    while ( !atEndOfSection(astrm.next()) )
    {
	if ( StringView(astrm.keyWord())==sKeyStor )
	    isascii = *astrm.value() != 'B';
    }
    if ( !l2dd.read(sfio.istrm(),isascii) )
    {
	sfio.closeFail();
	return false;
    }

    sfio.closeSuccess();
    return true;
}


bool PosInfo::Survey2D::setGeometry( const PosInfo::Line2DData& l2dd )
{
    const char* lnm = l2dd.lineName().buf();
    const int lidx = lineindex_.indexOf( lnm );
    FileMultiString fms;
    if ( lidx >= 0 )
    {
	fms = lineindex_.getValue( lidx );
	if ( fms.size() == 1 )
	    fms.add( getNewID(lineindex_) );
    }
    else
    {
	fms = getNewStorageName( lnm, lsfp_, lineindex_ );
	fms.add( getNewID(lineindex_) );
    }

    SafeFileIO sfio( FilePath(lsfp_,fms[0]).fullPath(), true );
    if ( !sfio.open(false) )
	return false;

    ascostream astrm( sfio.ostrm() );
    astrm.putHeader( "Line2D Geometry" );

    float max, median;
    l2dd.compDistBetwTrcsStats( max, median );
    astrm.put( sKeyTrcDist, max, median );

    astrm.put( sKeyStor, cWriteAscii ? sKey::Ascii() : sKey::Binary() );
    astrm.newParagraph();
    if ( l2dd.write(sfio.ostrm(),cWriteAscii,true) )
    {
	sfio.closeSuccess();
	if ( lidx < 0 )
	{
	    readIdxFiles(); // lower chance of concurrent update
	    lineindex_.set( lnm, fms );
	    updateMaxID( fms.getIValue(1), lineindex_ );
	    writeIdxFile( true );
	}
    }
    else
    {
	sfio.closeFail();
	return false;
    }

    return true;
}


int PosInfo::Survey2D::getLineIdx( int lineid ) const
{
    for ( int idx=0; idx<lineindex_.size(); idx++ )
    {
	FileMultiString info( lineindex_.getValue(idx) );
	if ( info.size()>1 && info.getIValue(1) == lineid )
	    return idx;
    }

    return -1;
}


void PosInfo::Survey2D::renameLine( const char* oldlnm, const char* newlnm )
{
    BufferString cleannm( newlnm );
    cleannm.clean();

    int lidx = lineindex_.indexOf( oldlnm );
    if ( lidx < 0 ) return;

    FileMultiString oldfms( lineindex_.getValue(lidx) );
    const FilePath oldfp( lsfp_ , oldfms[0] );
    const FilePath newfp( lsfp_ , cleannm.buf() );

    if ( hasLine(newlnm) )
	removeLine( newlnm );

    lidx = lineindex_.indexOf( oldlnm );
    if ( lidx < 0 ) return;

    if ( !File::rename(oldfp.fullPath(),newfp.fullPath()) )
	return;

    lineindex_.setKey( lidx, newlnm );
    FileMultiString newval( cleannm.buf() );
    newval.add( oldfms.getIValue(1) );
    lineindex_.setValue( lidx, newval.buf() );
    writeIdxFile( true );
}


void PosInfo::Survey2D::removeLine( const char* lnm )
{
    const int lidx = lineindex_.indexOf( lnm );
    if ( lidx < 0 ) return;

    FileMultiString fms( lineindex_.getValue(lidx) );
    SafeFileIO sfio( FilePath(lsfp_,fms[0]).fullPath() );
    sfio.remove();
    lineindex_.remove( lidx );
    writeIdxFile( true );
}


void PosInfo::Survey2D::removeLine( mIdxTyp lineid )
{
    const int lidx = getLineIdx( lineid );
    if ( lidx < 0 ) return;

    FileMultiString fms( lineindex_.getValue(lidx) );
    SafeFileIO sfio( FilePath(lsfp_,fms[0]).fullPath() );
    sfio.remove();
    lineindex_.remove( lidx );
    writeIdxFile( true );
}


void PosInfo::Survey2D::removeLineSet( mIdxTyp lsid )
{
    const int lsidx = getLineSetIdx( lsid );
    if ( lsidx<0 ) return;

	BufferString linesetnm( getLineSet(lsid) );
	const char* curlinesetnm = curLineSet();
	const bool iscurls = linesetnm == curlinesetnm;
    FileMultiString fms( lsindex_.getValue(lsidx) );
    FilePath fp( basefp_ , fms[0] );
    if ( basefp_ == fp )
	return ErrMsg( "Cannot delete 2DGeom folder" );

    const BufferString dirnm( fp.fullPath() );
    if ( File::exists(dirnm) )
	File::removeDir(dirnm);
    lsindex_.remove( lsidx );
    writeIdxFile( false );

	if ( !iscurls )	return;
    if ( lsindex_.size() > 1 )
	setCurLineSet( lsindex_.getKey(0).str() );
}


void PosInfo::Survey2D::removeLineSet( const char* lsnm )
{
    if ( !lsnm || !*lsnm ) return;

    const bool iscurls = lsnm_ == lsnm;
    const int lsidx = lsindex_.indexOf( lsnm );
    if ( lsidx < 0 ) return;

    FileMultiString fms( lsindex_.getValue(lsidx) );
    FilePath fp( basefp_ , fms[0] );
    if ( basefp_ == fp )
	return ErrMsg( "Cannot delete 2DGeom folder" );
    const BufferString dirnm( fp.fullPath() );
    if ( File::exists(dirnm) )
	File::removeDir(dirnm);
    lsindex_.remove( lsidx );
    writeIdxFile( false );
    if ( !iscurls ) return;

    if ( lsindex_.size() > 1 )
	setCurLineSet( lsindex_.getKey(0).str() );
}


void PosInfo::Survey2D::renameLineSet( const char* oldlsnm, const char* newlsnm)
{
    if ( !oldlsnm || !*oldlsnm ) return;
    int lsidx = lsindex_.indexOf( oldlsnm );
    if ( lsidx < 0 ) return;

    if ( hasLineSet(newlsnm) )
	removeLineSet( newlsnm );

    lsidx = lsindex_.indexOf( oldlsnm );
    if ( lsidx < 0 ) return;
    FileMultiString fms( lsindex_.getValue(lsidx) );
    FilePath fp( basefp_, fms[0] );
    const BufferString dirnm( fp.fullPath() );
    if ( File::exists(dirnm) )
    {
	lsindex_.setKey( lsidx, newlsnm );
	BufferString cleannm( newlsnm );
	cleannm.clean();
	FilePath newfp( basefp_, cleannm.buf() );
	File::rename( dirnm, newfp.fullPath() );
	FileMultiString lspar( cleannm.buf() );
	lspar.add( fms.getIValue(1) );
	lsindex_.setValue( lsidx, lspar );
	writeIdxFile( false );
	setCurLineSet( cleannm.buf() );
    }
}


PosInfo::Line2DKey PosInfo::Survey2D::getLine2DKey( const char* linesetnm,
					      const char* linenm ) const
{
    if ( lsnm_ != linesetnm )
	setCurLineSet( linesetnm );
    return PosInfo::Line2DKey( getLineSetID(linesetnm), getLineID(linenm) );
}


const char* PosInfo::Survey2D::getLSFileNm( const char* lsnm ) const
{
    BufferString cleannm( lsnm );
    cleannm.clean();
    mDeclStaticString( ret );
    ret = FilePath(basefp_,cleannm,sIdxFilename).fullPath();
    return ret.buf();
}


const char* PosInfo::Survey2D::getLineFileNm( const char* lsnm,
					      const char* linenm ) const
{
    BufferString cllsnm( lsnm );
    cllsnm.clean();
    BufferString cllnm( linenm );
    cllnm.clean();

    PosInfo::Line2DKey l2dky = getLine2DKey( lsnm, linenm );
    if ( !l2dky.isOK() )
	return 0;

    mDeclStaticString( ret );
    ret = FilePath( basefp_, cllsnm, cllnm ).fullPath();
    return ret.buf();
}


bool PosInfo::Survey2D::isIdxFileNew( const char* lsnm ) const
{
    BufferString timestamp( getIdxTimeStamp(lsnm) );
    if ( timestamp.isEmpty() )
	return false;
    return Time::isEarlier( curlstimestr_.buf(), getIdxTimeStamp(lsnm) );
}


BufferString PosInfo::Survey2D::getIdxTimeStamp( const char* lsnm ) const
{
    FilePath fp( basefp_, lsnm, sIdxFilename );
    if ( fp.isEmpty() ) return 0;
    BufferString timestamp( File::timeLastModified(fp.fullPath()) );
    return timestamp;
}


bool PosInfo::Survey2D::readDistBetwTrcsStats( const char* linenm,
					       float& max, float& median ) const
{
    if ( !linenm )
	return false;

    SafeFileIO sfio( FilePath(lsfp_,linenm).fullPath() );
    if ( !sfio.open(true) )
	return false;

    ascistream astrm( sfio.istrm() ); // read header
    while ( !atEndOfSection(astrm.next()) )
    {
	if ( StringView(astrm.keyWord()) == sKeyTrcDist )
	{
	    FileMultiString statsstr(astrm.value());
	    max = statsstr.getFValue(0);
	    median = statsstr.getFValue(1);
	    sfio.closeSuccess();
	    return true;
	}
    }
    sfio.closeSuccess();
    return false;
}
