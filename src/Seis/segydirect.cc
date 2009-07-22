/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2008
-*/

static const char* rcsID = "$Id: segydirect.cc,v 1.14 2009-07-22 16:01:34 cvsbert Exp $";

#include "segydirectdef.h"
#include "segyfiledata.h"
#include "seisposindexer.h"
#include "idxable.h"
#include "strmprov.h"
#include "ascstream.h"
#include "keystrs.h"
#include "posinfo.h"
#include "survinfo.h"
#include "filepath.h"

const char* sKeyDirectDef = "DirectSEG-Y";
static const char* sKeyFileType = "SEG-Y Direct Definition";
static const char* sKeyNrFiles = "Number of files";

namespace SEGY
{

class PosKeyList : public Seis::PosKeyList
{
public:

PosKeyList()
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
    if ( !fds_ || nr < 0 )		return Seis::PosKey::undef();

    int idx;
    IdxAble::findPos( cumszs_.arr(), cumszs_.size(), nr, -1, idx );
    if ( idx < 0 )			return Seis::PosKey::undef();

    const FileData& fd = *(*fds_)[idx];
    const int relidx = nr - cumszs_[idx];
    if ( relidx >= fd.size() )		return Seis::PosKey::undef();
    const SEGY::TraceInfo& ti = *fd[relidx];

    if ( !ti.isUsable() )		return Seis::PosKey::undef();
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
    , keylist_(*new SEGY::PosKeyList) \
    , indexer_(*new Seis::PosIndexer(keylist_))

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
    delete myfds_;
    delete &keylist_;
    delete &indexer_;
}


void SEGY::DirectDef::setData( FileDataSet* fds )
{
    if ( fds != myfds_ )
	delete myfds_;
    fds_ = myfds_ = fds;
    keylist_.setFDS( fds_ ); indexer_.reIndex();
}


void SEGY::DirectDef::setData( const FileDataSet& fds, bool nc )
{
    if ( &fds != myfds_ )
	delete myfds_;
    if ( nc )
	{ myfds_ = 0; fds_ = &fds; }
    else
	fds_ = myfds_ = new FileDataSet( fds );
    keylist_.setFDS( fds_ ); indexer_.reIndex();
}


SEGY::FileDataSet::TrcIdx SEGY::DirectDef::find( const Seis::PosKey& pk,
						 bool chkoffs ) const
{
    return keylist_.find( pk, indexer_, chkoffs );
}


#define mErrRet(s) { errmsg_ = s; return false; }

bool SEGY::DirectDef::readFromFile( const char* fnm )
{
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	mErrRet(BufferString("Cannot open '",fnm,"'"))

    ascistream astrm( *sd.istrm, true );
    if ( !astrm.isOfFileType(sKeyFileType) )
	mErrRet(BufferString("Input file '",fnm,"' has wrong file type"))

    int nrfiles = 0;
    IOPar iop; iop.getFrom( astrm );
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


bool SEGY::DirectDef::writeToFile( const char* fnm ) const
{
    StreamData sd( StreamProvider(fnm).makeOStream() );
    if ( !sd.usable() )
	mErrRet(BufferString("Cannot open '",fnm,"' for write"))

    const int nrfiles = fds_ ? fds_->size() : 0;
    ascostream astrm( *sd.ostrm );
    astrm.putHeader( sKeyFileType );
    if ( fds_ )
    {
	IOPar iop( fds_->pars() );
	iop.set( sKeyNrFiles, nrfiles );
	iop.putTo( astrm );
    }

    for ( int ifile=0; ifile<nrfiles; ifile++ )
    {
	const SEGY::FileData& fd = *(*fds_)[ifile];
	if ( !fd.putTo(astrm) )
	{
	    BufferString emsg( "Error writing data for '" );
	    emsg += fd.fname_; emsg += "'"; emsg += "\nto '";
	    emsg += fnm; emsg += "'";
	    mErrRet(emsg)
	}
    }

    return true;
}


void SEGY::DirectDef::getPosData( PosInfo::CubeData& cd ) const
{
    if ( !fds_ || Seis::is2D(indexer_.geomType()) ) return;

    Interval<int> inlrg( indexer_.inlRange() ); inlrg.sort();
    Interval<int> crlrg( indexer_.crlRange() ); crlrg.sort();
    const BinID step( SI().inlStep(), SI().crlStep() );

    PosInfo::CubeDataFiller cdf( cd );
    for ( int inl=inlrg.start; inl<=inlrg.stop; inl+=step.inl )
    {
	for ( int crl=crlrg.start; crl<=crlrg.stop; crl+=step.crl )
	{
	    const BinID bid( inl, crl );
	    const FileDataSet::TrcIdx tidx = keylist_.find( Seis::PosKey(bid),
		    					    indexer_, false );
	    if ( tidx.isValid() )
		cdf.add( bid );
	}
    }

    cdf.finish();
}


void SEGY::DirectDef::getPosData( PosInfo::Line2DData& ld ) const
{
    if ( !fds_ || fds_->isEmpty() || !Seis::is2D(indexer_.geomType()) )
	return;

    Interval<int> nrrg( indexer_.trcNrRange() );
    nrrg.sort();
    for ( int nr=nrrg.start; nr<=nrrg.stop; nr++ )
    {
	const FileDataSet::TrcIdx tidx = keylist_.find( Seis::PosKey(nr),
							indexer_, false );
	if ( !tidx.isValid() ) continue;

	PosInfo::Line2DPos l2dpos( nr );
	l2dpos.coord_ = (*(*fds_)[tidx.filenr_])[tidx.trcidx_]->coord();
	ld.posns_ += l2dpos;
    }
    const FileData& fd = *(*fds_)[0];
    ld.zrg_.start = fd.sampling_.start;
    ld.zrg_.step = fd.sampling_.step;
    ld.zrg_.stop = ld.zrg_.start + (fd.trcsz_-1) * ld.zrg_.step;
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
