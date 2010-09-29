/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: surv2dgeom.cc,v 1.4 2010-09-29 03:49:37 cvssatyaki Exp $";

#include "surv2dgeom.h"
#include "survinfo.h"
#include "filepath.h"
#include "file.h"
#include "ascstream.h"
#include "oddirs.h"
#include "safefileio.h"
#include "settings.h"
#include <iostream>

static PosInfo::Survey2D* theinst = 0;
static const char* sIdxFilename = "idx.txt";
static const char* sKeyStor = "Storage";
static bool cWriteAscii = Settings::common().isTrue("2DGeometry.Write Ascii");


namespace PosInfo {
struct Survey2DDeleter : public NamedObject {
void doDel( CallBacker* ) { delete theinst; theinst = 0; }
};
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


#define mErrRet(s1,s2,s3) { ErrMsg(BufferString(s1 " '",s2, "' " s3)); return; }

PosInfo::Survey2D::Survey2D()
    : basefp_(*new FilePath(GetDataDir()))
    , lsfp_(*new FilePath)
    , lsindex_(*new IOPar("Line Sets"))
    , lineindex_(*new IOPar("Lines"))
{
    basefp_.add( "2DGeom" );
    const BufferString dirnm = basefp_.fullPath();

    if ( File::exists(dirnm) )
    {
	if ( !File::isDirectory(dirnm) )
	{
	    if ( !File::remove(dirnm) )
		mErrRet("File",dirnm,"exists but is not a directory")
	}
    }
    if ( !File::exists(dirnm) )
    {
	if ( !File::createDir(dirnm) )
		mErrRet("Cannot create",dirnm," for 2D geometries")
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
	FilePath fp( basefp_ );
	fp.add( sIdxFilename );
	readIdxFile( fp.fullPath(), lsindex_ );
	if ( lsindex_.isEmpty() )
	    return;
	lsnm_ = lsindex_.getKey(0);
    }
    const int idxky = lsindex_.indexOf( lsnm_.buf() );
    if ( idxky < 0 )
    {	// selected lsnm_ doesn't exist (anymore): reset to default
	lsindex_.clear(); lineindex_.clear(); lsnm_.setEmpty();
	readIdxFiles();  return;
    }

    lsfp_ = basefp_;
    lsfp_.add( lsindex_.getValue(idxky) );
    FilePath fp( lsfp_ ); fp.add( sIdxFilename );
    readIdxFile( fp.fullPath(), lineindex_ );
}


void PosInfo::Survey2D::readIdxFile( const char* fnm, IOPar& iop )
{
    iop.clear();
    SafeFileIO sfio( fnm, true );
    if ( !sfio.open(true) ) return;
    ascistream astrm( sfio.istrm() );
    iop.getFrom( astrm );
    sfio.closeSuccess();
}


void PosInfo::Survey2D::writeIdxFile( bool lines ) const
{
    FilePath fp( lines ? lsfp_ : basefp_ ); fp.add( sIdxFilename );
    SafeFileIO sfio( fp.fullPath(), true );
    if ( !sfio.open(false) )
    {
	ErrMsg("Cannot open 2D Geometry index file for write."
		"\nCheck disk space and file permissions.");
	return;
    }
    ascostream astrm( sfio.ostrm() );
    astrm.putHeader("File Name Table");
    (lines ? lineindex_ : lsindex_).putTo( astrm );
    if ( sfio.ostrm().good() )
	sfio.closeSuccess();
    else
    {
	sfio.closeFail();
	ErrMsg("Error during write to 2D Geometry index file."
		"\nCheck disk space.");
    }
}


void PosInfo::Survey2D::getKeys( const IOPar& iop, BufferStringSet& nms ) const
{
    nms.erase();
    for ( int idx=0; idx<iop.size(); idx++ )
	nms.add( iop.getKey(idx) );
}


bool PosInfo::Survey2D::hasLineSet( const char* lsnm ) const
{
    return lsindex_.hasKey( lsnm );
}


bool PosInfo::Survey2D::hasLine( const char* lnm, const char* lsnm ) const
{
    if ( !lsnm || !strcmp(lsnm_.buf(),lsnm) )
	return lineindex_.hasKey( lsnm );

    BufferStringSet nms; getLines( nms, lsnm );
    return nms.isPresent( lnm );
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
    fp.setFileName( lsindex_.getValue(idxky) ); fp.add( sIdxFilename );
    readIdxFile( fp.fullPath(), iop );
    getKeys( iop, nms );
}


BufferString PosInfo::Survey2D::getNewStorageName( const char* nm,
			const FilePath& inpfp, const IOPar& iop ) const
{
    BufferString clnnm( nm );
    int itry = 1;
    while ( true )
    {
	cleanupString( clnnm.buf(), mC_False, mC_False, mC_False );
	FilePath fp( inpfp ); fp.add( clnnm );
	if ( !File::exists(clnnm) )
	    break;

	itry++;
	clnnm = nm; clnnm += itry;
    }
    return clnnm;
}



void PosInfo::Survey2D::setCurLineSet( const char* lsnm ) const
{
    if ( lsnm_ == lsnm )
	return;

    PosInfo::Survey2D& self = *const_cast<PosInfo::Survey2D*>( this );
    self.lsnm_ = lsnm;
    self.readIdxFiles();
    if ( !lsnm || lsnm_ == lsnm )
	return;

    // New line set specified
    self.lsnm_ = lsnm;
    const BufferString dirnm( getNewStorageName(lsnm,basefp_,lsindex_) );
    self.lsindex_.add( lsnm, dirnm );
    self.lineindex_.clear();
    self.lsfp_ = basefp_; self.lsfp_.add( dirnm );
    File::createDir( lsfp_.fullPath() );
    writeIdxFile( false );
}


bool PosInfo::Survey2D::getGeometry( PosInfo::Line2DData& l2dd ) const
{
    const int lidx = lineindex_.indexOf( l2dd.lineName().buf() );
    l2dd.setEmpty();
    if ( lidx < 0 )
	return false;

    FilePath fp( lsfp_ ); fp.add( lineindex_.getValue(lidx) );
    SafeFileIO sfio( fp.fullPath() );
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
    BufferString fnm;
    if ( lidx >= 0 )
	fnm = lineindex_.getValue( lidx );
    else
	fnm = getNewStorageName( lnm, lsfp_, lineindex_ );

    FilePath fp( lsfp_ ); fp.add( fnm );
    SafeFileIO sfio( fp.fullPath(), true );
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
	    lineindex_.set( lnm, fnm );
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


void PosInfo::Survey2D::removeLine( const char* lnm )
{
    const int lidx = lineindex_.indexOf( lnm );
    if ( lidx < 0 ) return;

    FilePath fp( lsfp_ ); fp.add( lineindex_.getValue(lidx) );
    SafeFileIO sfio( fp.fullPath() );
    sfio.remove();
    lineindex_.remove( lidx );
}


void PosInfo::Survey2D::removeLineSet( const char* lsnm )
{
    if ( !lsnm || !*lsnm ) return;

    const bool iscurls = lsnm_ == lsnm;
    const int lsidx = lsindex_.indexOf( lsnm );
    if ( lsidx < 0 ) return;

    FilePath fp( basefp_ ); fp.add( lsindex_.getValue(lsidx) );
    const BufferString dirnm( fp.fullPath() );
    if ( File::exists(dirnm) )
	File::removeDir(dirnm);
    lsindex_.remove( lsidx );
    if ( !iscurls ) return;

    lsnm = lsindex_.isEmpty() ? "" : lsindex_.getKey( 0 );
    setCurLineSet( lsnm );
}
