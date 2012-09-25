/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "surv2dgeom.h"

#include "ascstream.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "safefileio.h"
#include "settings.h"
#include "staticstring.h"
#include "strmprov.h"
#include "survinfo.h"
#include "timefun.h"

#include <iostream>

static PosInfo::Survey2D* theinst = 0;
static const char* sIdxFilename = "idx.txt";
static const char* sKeyStor = "Storage";
static const char* sKeyMaxID = "Max ID";
static bool cWriteAscii = false;



void PosInfo::Survey2D::initClass()
{
    cWriteAscii = Settings::common().isTrue("2DGeometry.Write Ascii");
}


namespace PosInfo {
struct Survey2DDeleter : public NamedObject {
void doDel( CallBacker* ) { delete theinst; theinst = 0; }
};
}

bool PosInfo::GeomID::isOK() const
{ return S2DPOS().hasLine( lineid_, lsid_ ); }

void PosInfo::GeomID::setUndef()
{ lineid_ = lsid_ = -1; }

BufferString PosInfo::GeomID::toString() const
{
    BufferString str; str.add(lsid_).add(".").add(lineid_);
    return str;
}

bool PosInfo::GeomID::fromString( const char* str )
{
    BufferString idstr = str;
    char* ptr = strchr( idstr.buf(), '.' );
    if ( !ptr ) return false;

    ptr++;
    lineid_ = toInt( ptr );

    ptr--;
    if ( ptr ) *ptr = '\0';
    lsid_ = toInt( idstr.buf() );
    return isOK();
}


PosInfo::Survey2D& PosInfo::POS2DAdmin()
{
    if ( !theinst )
    {
	theinst = new PosInfo::Survey2D;
	static PosInfo::Survey2DDeleter s2dd;
	const_cast<SurveyInfo&>(SI()).deleteNotify(
			mCB(&s2dd,PosInfo::Survey2DDeleter,doDel) );
    }
    return *theinst;
}


#define mErrRet(s1,s2,s3) \
{ \
    BufferString cmd("od_DispMsg --err ",BufferString(s1 " '",s2, "' " s3)); \
    StreamProvider prov( cmd ); \
    prov.executeCommand( false ); \
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
	if ( !File::isDirectory(dirnm) )
	{
	    if ( !File::remove(dirnm) )
		mErrRet("File",dirnm,"exists but is not a directory");
	}
    }

    if ( !File::exists(dirnm) )
    {
	if ( !File::createDir(dirnm) )
	    mErrRet("Cannot create",dirnm,
		    "for 2D geometries. Check write permissions")
    }

    readIdxFiles();
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


void PosInfo::Survey2D::readIdxFile( const char* fnm, IOPar& iop )
{
    iop.setEmpty();
    SafeFileIO sfio( fnm, true );
    if ( !sfio.open(true) ) return;
    ascistream astrm( sfio.istrm() );
    iop.getFrom( astrm );
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
    if ( sfio.ostrm().good() )
	sfio.closeSuccess();
    else
    {
	sfio.closeFail();
	mErrRet("Error during write to 2D Geometry index file",fp.fullPath(),
		"Check disk space.");
    }
}


void PosInfo::Survey2D::updateMaxID( int maxid, IOPar& par )
{
    par.remove( sKeyMaxID );
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


void PosInfo::Survey2D::getIDs( const IOPar& iop, TypeSet<int>& ids ) const
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


int PosInfo::Survey2D::curLineSetID() const
{
    return getLineSetID( lsnm_.buf() );
}


int PosInfo::Survey2D::getLineSetID( const char* lsnm ) const
{
    if ( !lsnm ) return -1;

    for ( int idx=0; idx<lsindex_.size(); idx++ )
    {
	FileMultiString info( lsindex_.getValue(idx) );
	if ( info.size()>0 && !strcmp(lsnm,lsindex_.getKey(idx)) )
	    return info.getIValue( 1 );
    }

    return -1;
}


int PosInfo::Survey2D::getLineID( const char* linenm ) const
{
    if ( !linenm ) return -1;

    for ( int idx=0; idx<lineindex_.size(); idx++ )
    {
	FileMultiString info( lineindex_.getValue(idx) );
	if ( info.size()>0 && !strcmp(linenm,lineindex_.getKey(idx)) )
	    return info.getIValue( 1 );
    }

    return -1;
}


const char* PosInfo::Survey2D::getLineSet( int lsid ) const
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


bool PosInfo::Survey2D::hasLineSet( int lsid ) const
{
    for ( int idx=0; idx<lsindex_.size(); idx++ )
    {
	FileMultiString info( lsindex_.getValue(idx) );
	if ( info.size()>0 && (lsid == info.getIValue(1)) )
	    return true;
    }

    return false;
}


const char* PosInfo::Survey2D::getLineName( int lineid ) const
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
    if ( !lsnm || !strcmp(lsnm_.buf(),lsnm) )
	return lineindex_.hasKey( lnm );

    BufferStringSet nms; getLines( nms, lsnm );
    return nms.isPresent( lnm );
}



bool PosInfo::Survey2D::hasLine( int lineid, int lsid ) const
{
    if ( lsid < 0 )
	lsid = curLineSetID();

    if ( !hasLineSet(lsid) )
	return false; 

    TypeSet<int> lineids; getLineIDs( lineids, lsid );
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
    if ( !lsnm || !strcmp(lsnm_.buf(),lsnm) )
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


void PosInfo::Survey2D::getLines( BufferStringSet& nms, int lsid ) const
{
    if ( lsid == -1 )
	return getLines( nms, 0 );

    BufferString lsnm = getLineSet( lsid );
    getLines( nms, lsnm );
}


void PosInfo::Survey2D::getLineIDs( TypeSet<int>& ids, int lsid ) const
{
    if ( lsid >=0 && !hasLineSet(lsid) )
	return;

    if ( lsid == curLineSetID() )
    {
	getIDs( lineindex_, ids );
	return;
    }

    if ( lsid == -1 )
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


int PosInfo::Survey2D::getNewID( IOPar& iop ) 
{
    int savedmeaxid = -mUdf(int);
    iop.get( sKeyMaxID, savedmeaxid );
    int newlineidx = 0;
    
    if ( !iop.size() )
	return 0;

    for ( int idx=0; idx<iop.size(); idx++ )
    {
	if ( !strcmp(iop.getKey(idx),sKeyMaxID) )
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
	cleanupString( clnnm.buf(), false, false, false );
	FilePath fp( inpfp, clnnm );
	if ( !File::exists(fp.fullPath()) )
	    break;

	itry++;
	clnnm = nm; clnnm += itry;
    }

    return clnnm;
}


void PosInfo::Survey2D::setCurLineSet( int lsid ) const
{
    if ( !hasLineSet(lsid) )
	return;

    int lsidx = getLineSetIdx( lsid );
    if ( lsidx < 0 || mIsUdf(lsidx) )
	return;
    
    BufferString maxidkey( sKeyMaxID );
    if ( maxidkey != lsindex_.getKey(lsidx) )
	setCurLineSet( lsindex_.getKey(lsidx) );
}


void PosInfo::Survey2D::setCurLineSet( const char* lsnm ) const
{
    if ( !lsnm || !*lsnm )
    {
	lineindex_.setEmpty();
	return;
    }

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


bool PosInfo::Survey2D::getGeometry( int lineid,
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


bool PosInfo::Survey2D::getGeometry( const GeomID& geomid,
				     PosInfo::Line2DData& l2dd ) const
{
    if ( !geomid.isOK() ) return false;

    Threads::MutexLocker* locker = 0;
    if ( geomid.lsid_ != S2DPOS().curLineSetID() )
    {
	locker = new Threads::MutexLocker( mutex_ );
	S2DPOS().setCurLineSet( geomid.lsid_ );
    }

    const char* linenm = S2DPOS().getLineName( geomid.lineid_ );
    if ( !linenm )
    {
	delete locker;
	return false;
    }
    
    const bool ret = S2DPOS().getGeometry( geomid.lineid_, l2dd );
    delete locker;
    
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
	if ( !strcmp(astrm.keyWord(),sKeyStor) )
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
    astrm.put( sKeyStor, cWriteAscii ? "Ascii" : "Binary" );
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
    cleanupString( cleannm.buf(), false, false, false );

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


void PosInfo::Survey2D::removeLine( int lineid )
{
    const int lidx = getLineIdx( lineid );
    if ( lidx < 0 ) return;

    FileMultiString fms( lineindex_.getValue(lidx) );
    SafeFileIO sfio( FilePath(lsfp_,fms[0]).fullPath() );
    sfio.remove();
    lineindex_.remove( lidx );
    writeIdxFile( true );
}


void PosInfo::Survey2D::removeLineSet( int lsid )
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
	setCurLineSet( lsindex_.getKey(0) );
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
	setCurLineSet( lsindex_.getKey(0) );
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
	cleanupString( cleannm.buf(), false, false, false );
	FilePath newfp( basefp_, cleannm.buf() );
	File::rename( dirnm, newfp.fullPath() );
	FileMultiString lspar( cleannm.buf() );
	lspar.add( fms.getIValue(1) );
	lsindex_.setValue( lsidx, lspar );
	writeIdxFile( false );
	setCurLineSet( cleannm.buf() );
    }
}


PosInfo::GeomID PosInfo::Survey2D::getGeomID( const char* linesetnm,
					      const char* linenm ) const
{
    if ( lsnm_ != linesetnm )
	setCurLineSet( linesetnm );
    PosInfo::GeomID geomid( getLineSetID(linesetnm), getLineID(linenm) );
    return geomid;
}


const char* PosInfo::Survey2D::getLSFileNm( const char* lsnm ) const
{
    static StaticStringManager stm;
    BufferString& fnm = stm.getString();
    BufferString cleannm( lsnm );
    cleanupString( cleannm.buf(), false, false, false );
    fnm = FilePath(basefp_,cleannm,sIdxFilename).fullPath();
    return fnm.buf();
}


const char* PosInfo::Survey2D::getLineFileNm( const char* lsnm,
       					      const char* linenm ) const
{
    static StaticStringManager stm;
    BufferString& fnm = stm.getString();
    BufferString cllsnm( lsnm );
    cleanupString( cllsnm.buf(), false, false, false );
    BufferString cllnm( linenm );
    cleanupString( cllnm.buf(), false, false, false );
    
    PosInfo::GeomID geomid = getGeomID( lsnm, linenm );
    if ( !geomid.isOK() )
	return 0;
    return fnm = FilePath(basefp_,cllsnm,cllnm).fullPath();
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
