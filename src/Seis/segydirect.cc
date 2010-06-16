/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2008
-*/

static const char* rcsID = "$Id: segydirect.cc,v 1.19 2010-06-16 12:31:49 cvskris Exp $";

#include "segydirectdef.h"

#include "datainterp.h"
#include "datachar.h"
#include "segyfiledata.h"
#include "seisposindexer.h"
#include "idxable.h"
#include "strmprov.h"
#include "ascstream.h"
#include "keystrs.h"
#include "offsetazimuth.h"
#include "posinfo.h"
#include "survinfo.h"
#include "filepath.h"
#include <iostream>


namespace SEGY
{

const char* DirectDef::sKeyDirectDef = "DirectSEG-Y";
const char* DirectDef::sKeyFileType = "SEG-Y Direct Definition";
const char* DirectDef::sKeyNrFiles = "Number of files";
const char* DirectDef::sKeyFloatDataChar = "Float datachar";
const char* DirectDef::sKeyInt32DataChar = "Int32 datachar";
const char* DirectDef::sKeyInt64DataChar = "Int64 datachar";

class PosKeyList : public Seis::PosKeyList
{
public:
virtual FileDataSet::TrcIdx find( const Seis::PosKey& pk,
			  const Seis::PosIndexer& idxer,
       			  bool chkoffs ) const			= 0;
};

class StreamPosKeyList : public PosKeyList
{
public:

StreamPosKeyList( const char* fnm, od_int64 start, TypeSet<od_int64>& csize,
       		  od_int64 totsz )
    : start_( start )
    , cumszs_( csize )
    , sd_( StreamProvider(fnm).makeIStream() )
    , totsz_( totsz )
{ }


od_int64 size() const { return totsz_; }


Seis::PosKey key( od_int64 nr ) const
{
    std::istream* strm = sd_.istrm;
    if ( !strm || !strm->good() || nr<0 )
	return Seis::PosKey::undef();

    static const int unitsz = sizeof(int)+sizeof(int)+sizeof(int)+sizeof(bool);

    strm->seekg( start_+nr*unitsz, std::ios::beg );
    BinID bid;
    int offsetazimuth;
    //TODO fix endianness
    strm->read( (char*) &bid.inl, sizeof( bid.inl ) );
    strm->read( (char*) &bid.crl, sizeof( bid.crl ) );
    strm->read( (char*) &offsetazimuth, sizeof( offsetazimuth ) );

    if ( !strm->good() )
	return Seis::PosKey::undef();

    OffsetAzimuth oa;
    oa.setFrom( offsetazimuth );

    Seis::PosKey res( bid, oa.offset() );
    return res;
}


FileDataSet::TrcIdx find( const Seis::PosKey& pk,
			  const Seis::PosIndexer& idxer,
       			  bool chkoffs ) const
{
    od_int64 nr = idxer.findFirst( pk, chkoffs );
    FileDataSet::TrcIdx tidx;
    if ( nr < 0 ) return tidx;

    IdxAble::findPos( cumszs_.arr(), cumszs_.size(), nr, -1, tidx.filenr_ );
    tidx.trcidx_ = nr - cumszs_[tidx.filenr_];
    return tidx;
}

    StreamData		sd_;
    TypeSet<od_int64>	cumszs_;
    od_int64		start_;
    od_int64		totsz_;
};

class FDSPosKeyList : public PosKeyList
{
public:

FDSPosKeyList()
{
    setFDS( 0 );
}

void setFDS( const FileDataSet* fds )
{
    fds_ = fds; totsz_ = 0; cumszs_.erase();
    if ( !fds_ ) return;

    for ( int idx=0; idx<fds_->size(); idx++ )
    {
	const int sz = (*fds_)[idx]->size();
	cumszs_ += totsz_;
	totsz_ += sz;
    }
}

od_int64 size() const
{
    return totsz_;
}

Seis::PosKey key( od_int64 nr ) const
{
    if ( !fds_ || nr < 0 )              return Seis::PosKey::undef();

    int idx;
    IdxAble::findPos( cumszs_.arr(), cumszs_.size(), nr, -1, idx );
    if ( idx < 0 )                      return Seis::PosKey::undef();

    const FileData& fd = *(*fds_)[idx];
    const int relidx = nr - cumszs_[idx];
    if ( relidx >= fd.size() )          return Seis::PosKey::undef();
    const SEGY::TraceInfo& ti = *fd[relidx];

    if ( !ti.isUsable() )               return Seis::PosKey::undef();
    return ti.pos_;
}

FileDataSet::TrcIdx find( const Seis::PosKey& pk,
			  const Seis::PosIndexer& idxer,
       			  bool chkoffs ) const
{
    od_int64 nr = idxer.findFirst( pk, chkoffs );
    FileDataSet::TrcIdx tidx;
    if ( nr < 0 ) return tidx;

    IdxAble::findPos( cumszs_.arr(), cumszs_.size(), nr, -1, tidx.filenr_ );
    tidx.trcidx_ = nr - cumszs_[tidx.filenr_];
    return tidx;
}

    TypeSet<od_int64>	cumszs_;
    od_int64		totsz_;
    const FileDataSet*	fds_;

};
}


#define mDefMembInitList \
    : fds_(0) \
    , myfds_(0) \
    , curfidx_(-1) \
    , keylist_( 0 ) \
    , cubedata_( *new PosInfo::CubeData ) \
    , linedata_( *new PosInfo::Line2DData ) \
    , indexer_( 0 )

SEGY::DirectDef::DirectDef()
    mDefMembInitList
{
}


SEGY::DirectDef::DirectDef( const char* fnm )
    mDefMembInitList
{
    readFromFile( fnm );
}


SEGY::DirectDef::~DirectDef()
{
    delete &cubedata_;
    delete &linedata_;
    delete myfds_;
    delete keylist_;
    delete indexer_;
}


void SEGY::DirectDef::setData( FileDataSet* fds )
{
    if ( fds != myfds_ )
	delete myfds_;

    fds_ = myfds_ = fds;

    delete keylist_;
    delete indexer_;

    SEGY::FDSPosKeyList* keylist = new SEGY::FDSPosKeyList;
    keylist_ = keylist;
    keylist->setFDS( fds_ );
    indexer_ = new Seis::PosIndexer(*keylist_,true);
    getPosData( cubedata_ );
    getPosData( linedata_ );
}


void SEGY::DirectDef::setData( const FileDataSet& fds, bool nc )
{
    if ( &fds != myfds_ )
	delete myfds_;
    if ( nc )
	{ myfds_ = 0; fds_ = &fds; }
    else
	fds_ = myfds_ = new FileDataSet( fds );

    delete keylist_;
    delete indexer_;

    SEGY::FDSPosKeyList* keylist = new SEGY::FDSPosKeyList;
    keylist_ = keylist;
    keylist->setFDS( fds_ );
    indexer_ = new Seis::PosIndexer(*keylist_,true);
    getPosData( cubedata_ );
    getPosData( linedata_ );
}


SEGY::FileDataSet::TrcIdx SEGY::DirectDef::find( const Seis::PosKey& pk,
						 bool chkoffs ) const
{
    return keylist_->find( pk, *indexer_, chkoffs );
}


#define mErrRet(s) { errmsg_ = s; return false; }
#define mGetInterp( str, type, interp ) \
    PtrMan<DataInterpreter<type> > interp = 0; \
    if ( iop1.get(str,dc ) ) \
    { \
	DataCharacteristics writtentype; \
	writtentype.set( dc.buf() ); \
	type dummy; \
	DataCharacteristics owntype(dummy); \
	if ( owntype!=writtentype ) \
	    interp = new DataInterpreter<type> ( writtentype ); \
    }

bool SEGY::DirectDef::readFromFile( const char* fnm )
{
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	mErrRet(BufferString("Cannot open '",fnm,"'"))

    ascistream astrm( *sd.istrm, true );
    if ( !astrm.isOfFileType(sKeyFileType) )
	mErrRet(BufferString("Input file '",fnm,"' has wrong file type"))

    IOPar iop1; iop1.getFrom( astrm );
    int version = 1;
    iop1.get( sKey::Version, version );
    if ( version==1 )
	return readV1FromFile( iop1, astrm, fnm );

    BufferString dc;
    mGetInterp( sKeyFloatDataChar, float, floatinterp );
    mGetInterp( sKeyInt64DataChar, od_int64, int64interp );
    mGetInterp( sKeyInt32DataChar, od_int32, int32interp );

    segypars_.getFrom( astrm );

    od_int64 datastart = 0; 
    od_int64 textpars = 0; 
    od_int64 cubedatastart = 0;
    od_int64 indexstart = 0; 

    std::istream& strm = *sd.istrm;
    strm.read( (char*) &datastart, sizeof(datastart) );
    strm.read( (char*) &textpars, sizeof(textpars) );
    strm.read( (char*) &cubedatastart, sizeof(cubedatastart) );
    strm.read( (char*) &indexstart, sizeof(indexstart) );
    if ( !strm.good() )
	return false;

    strm.seekg( textpars, std::ios::beg );
    ascistream astrm2( strm, false );

    IOPar iop2; iop2.getFrom( astrm2 );
    int nrfiles = 0;
    iop2.get( sKeyNrFiles, nrfiles );
    if ( !strm.good() )
	return false;

    filenames_.erase();
    TypeSet<od_int64> cumsizes;
    od_int64 accumulatedsize = 0;
    for ( int ifile=0; ifile<nrfiles; ifile++ )
    {
	BufferString key("File ");
	key += ifile;

	PtrMan<IOPar> filepars = iop2.subselect( key.buf() );
	if ( !filepars )
	    return false;

	BufferString filenm;
	if ( !filepars->get( sKey::FileName, filenm ) )
	    return false;

	od_int64 size;
	if ( !filepars->get( sKey::Size, size ) )
	    return false;

	filenames_.add( filenm );
	cumsizes += accumulatedsize;
	accumulatedsize += size;
    }

    if ( strm.tellg()!=cubedatastart )
	strm.seekg( cubedatastart, std::ios::beg );

    if ( !cubedata_.read( strm, false ) || !linedata_.read( strm, false ) )
	return false;

    delete myfds_;
    fds_ = myfds_ = 0;

    delete keylist_;
    delete indexer_;

    keylist_ = new SEGY::StreamPosKeyList( fnm, datastart, cumsizes,
	    				   accumulatedsize );
    indexer_ = new Seis::PosIndexer(*keylist_,false);
    if ( !indexer_->readFrom( fnm, indexstart, false, int32interp, int64interp,
	 floatinterp ) )
	return false;

    return true;
}


bool SEGY::DirectDef::readV1FromFile( const IOPar& iop, ascistream& astrm,
				      const char* fnm ) 
{
    int nrfiles = 0;
    iop.get( sKeyNrFiles, nrfiles );
    Seis::GeomType gt;
    if ( !Seis::getFromPar(iop,gt) )
	mErrRet(BufferString("Missing crucial info in '",fnm,"'"))
    
    FileDataSet* fds = new FileDataSet( iop );
    for ( int idx=0; idx<nrfiles; idx++ )
    {
	FileData* fd = new FileData(0,gt);
	if ( !fd->getFrom(astrm) )
	{
	    BufferString emsg( "Error reading " );
	    if ( nrfiles > 1 )
		{ emsg += idx+1; emsg += getRankPostFix(idx+1); emsg += " "; }
	    emsg += "file data from '"; emsg += fnm; emsg += "'";
	    delete fds;
	    mErrRet(emsg)
	}
	*fds += fd;
    }

    setData( fds );
    return true;
}


FixedString SEGY::DirectDef::fileName( int idx ) const
{
    if ( fds_ )
    {
	if ( fds_->validIdx( idx ) )
	    return (*fds_)[idx]->fname_.buf();
    }
    else
    {
	if ( filenames_.validIdx( idx ) )
	    return filenames_[idx]->buf();
    }

    return sKey::EmptyString;
}


const IOPar& SEGY::DirectDef::segyPars() const
{
    if ( fds_ )
	return fds_->pars();

    return segypars_;
}

#define mSetDc( par, type, string ) \
{ \
    type dummy; \
    DataCharacteristics(dummy).toString( dc.buf() ); \
}\
    par.set( string, dc )



bool SEGY::DirectDef::writeToFile( const char* fnm ) const
{
    StreamData sd( StreamProvider(fnm).makeOStream() );
    if ( !sd.usable() )
	mErrRet(BufferString("Cannot open '",fnm,"' for write"))

    const int nrfiles = fds_ ? fds_->size() : 0;
    ascostream astrm( *sd.ostrm );
    astrm.putHeader( sKeyFileType );

    IOPar iop1;
    iop1.set( sKey::Version, 2 );
    BufferString dc;
    mSetDc( iop1, od_int64, sKeyInt64DataChar );
    mSetDc( iop1, od_int32, sKeyInt32DataChar );
    mSetDc( iop1, float, sKeyFloatDataChar );
    iop1.putTo( astrm );
    fds_->pars().putTo( astrm );

    std::ostream& strm = astrm.stream();
    const od_uint64 headerstart = strm.tellp();
    
    //Reserve space for offsets, which are written at the end
    od_int64 datastart = 0; 
    od_int64 textpars = 0; 
    od_int64 cubedatastart = 0; 
    od_int64 indexstart = 0; 

#define mWriteOffsets \
    strm.write( (const char*) &datastart, sizeof(datastart) ); \
    strm.write( (const char*) &textpars, sizeof(textpars) ); \
    strm.write( (const char*) &cubedatastart, sizeof(cubedatastart) ); \
    strm.write( (const char*) &indexstart, sizeof(indexstart) )

    mWriteOffsets;

    //Write the data
    datastart = strm.tellp();
    for ( int ifile=0; ifile<nrfiles; ifile++ )
    {
	const SEGY::FileData& fd = *(*fds_)[ifile];
	for ( int idx=0; idx<fd.size(); idx++ )
	{
	    const BinID bid = fd.binID( idx );
	    strm.write( (const char*) &bid.inl, sizeof( bid.inl ) );
	    strm.write( (const char*) &bid.crl, sizeof( bid.crl ) );

	    const OffsetAzimuth oa( fd.offset( idx ), 0 );
	    const int oaint = oa.asInt();
	    strm.write( (const char*) &oaint, sizeof(oaint) );
	    const bool usable = fd.isUsable( idx );
	    strm.write( (const char*) &usable, sizeof(usable) );
	}
    }

    strm << '\n'; //Just for nice formatting

    textpars = strm.tellp();
    IOPar iop2;
    iop2.set( sKeyNrFiles, nrfiles );

    for ( int ifile=0; ifile<nrfiles; ifile++ )
    {
	IOPar filepars;
	const SEGY::FileData& fd = *(*fds_)[ifile];
	filepars.set( sKey::FileName, fd.fname_ );
	filepars.set( sKey::Size, fd.size() );
	
	BufferString key("File ");
	key += ifile;
	iop2.mergeComp( filepars, key.buf() );
    }

    ascostream astrm2( sd.ostrm );
    iop2.putTo( astrm2 );

    cubedatastart = strm.tellp();

    cubedata_.write( strm, false );
    linedata_.write( strm, false );

    indexstart = strm.tellp();

    indexer_->dumpTo( strm );

    const od_int64 eof = strm.tellp();
    strm.seekp( headerstart, std::ios::beg  );
    mWriteOffsets;

    strm.seekp( eof, std::ios::beg );
    return strm.good();
}


void SEGY::DirectDef::getPosData( PosInfo::CubeData& cd ) const
{
    if ( !indexer_ || Seis::is2D(indexer_->geomType()) ) return;

    Interval<int> inlrg( indexer_->inlRange() ); inlrg.sort();
    Interval<int> crlrg( indexer_->crlRange() ); crlrg.sort();
    const BinID step( SI().inlStep(), SI().crlStep() );

    PosInfo::CubeDataFiller cdf( cd );
    for ( int inl=inlrg.start; inl<=inlrg.stop; inl+=step.inl )
    {
	for ( int crl=crlrg.start; crl<=crlrg.stop; crl+=step.crl )
	{
	    const BinID bid( inl, crl );
	    const FileDataSet::TrcIdx tidx = keylist_->find( Seis::PosKey(bid),
		    					    *indexer_, false );
	    if ( tidx.isValid() )
		cdf.add( bid );
	}
    }

    cdf.finish();
}


void SEGY::DirectDef::getPosData( PosInfo::Line2DData& ld ) const
{
    if ( !fds_ || !indexer_ || fds_->isEmpty() ||
	 !Seis::is2D(indexer_->geomType()) )
	return;

    Interval<int> nrrg( indexer_->trcNrRange() );
    nrrg.sort();
    for ( int nr=nrrg.start; nr<=nrrg.stop; nr++ )
    {
	const FileDataSet::TrcIdx tidx = keylist_->find( Seis::PosKey(nr),
		*indexer_, false );
	if ( !tidx.isValid() ) continue;

	PosInfo::Line2DPos l2dpos( nr );
	l2dpos.coord_ = (*(*fds_)[tidx.filenr_])[tidx.trcidx_]->coord();
	ld.posns_ += l2dpos;
    }
    const FileData& fd = *(*fds_)[0];
    ld.zrg_ = fd.sampling_.interval( fd.trcsz_ );
}


const char* SEGY::DirectDef::get2DFileName( const char* dirnm, const char* unm )
{
    static BufferString ret;
    FilePath fp( dirnm );
    BufferString nm( unm ); cleanupString( nm.buf(), 1, 1, 1 );
    fp.add( nm ); fp.setExtension( "sgydef" );

    ret = fp.fullPath();
    return ret.buf();
}
