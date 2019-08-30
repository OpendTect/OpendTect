/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2008
-*/

#include "segyresorter.h"
#include "segydirectdef.h"
#include "segydirecttr.h"
#include "segytr.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "posfilter.h"
#include "cubedata.h"
#include "filepath.h"
#include "od_iostream.h"
#include "uistrings.h"

SEGY::ReSorter::Setup::Setup( Seis::GeomType gt, const DBKey& ky,
			      const char* fnm )
    : geom_(gt)
    , inpkey_(ky)
    , outfnm_(fnm)
    , newfileeach_(-1)
    , inlnames_(false)
    , curfnr_(0)
{
}


BufferString SEGY::ReSorter::Setup::getFileName( const Interval<int>& rg ) const
{
    if ( Seis::is2D(geom_) || newfileeach_ < 1 )
	return outfnm_;

    File::Path fp( outfnm_ );
    const BufferString ext( fp.extension() );
    if ( !ext.isEmpty() )
	fp.setExtension( 0 );

    BufferString fnm( fp.fileName(), "_" );
    if ( inlnames_ )
	fnm.add( rg.start ).add( "-" ).add( rg.stop );
    else
	fnm.add( ++curfnr_ );
    fp.setFileName( fnm );

    if ( !ext.isEmpty() )
	fp.setExtension( ext );

    return BufferString( fp.fullPath() );
}


Interval<int> SEGY::ReSorter::Setup::getInlRg( int curinl,
			const PosInfo::CubeData& cd ) const
{
    int curidx = cd.lineIndexOf( curinl );
    if ( curidx < 0 ) curidx = 0;
    int endidx = newfileeach_ <= 0 ? mUdf(int) : curidx + newfileeach_ - 1;
    if ( endidx >= cd.size() )
	endidx = cd.size() - 1;
    return Interval<int>( cd[curidx]->linenr_, cd[endidx]->linenr_ );
}


SEGY::ReSorter::ReSorter( const SEGY::ReSorter::Setup& su, const char* lnm )
    : Executor("Re-sorting")
    , posfilt_(0)
    , cdp_(*new PosInfo::CubeDataPos)
    , setup_(su)
    , msg_(tr("Reading scan data"))
    , nrdone_(0)
    , drdr_(0)
    , trcbuf_(0)
    , outstrm_(0)
    , needwritefilehdrs_(true)
{
    IOObj* ioobj = setup_.inpkey_.getIOObj();
    if ( !ioobj )
	msg_ = tr("Cannot find provided input in data manager");
    else
    {
	switch ( setup_.geom_ )
	{
	    case Seis::Vol:
	    {
		Translator* trans = ioobj->createTranslator();
		mDynamicCastGet(SEGYDirectSeisTrcTranslator*,str,trans)
		if ( !str )
		{
		    msg_ = tr("Input must be scanned SEG-Y cube");
		    delete trans;
		}
		else
		{
		    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
		    if ( !conn )
		    {
			msg_ = tr("Cannot open SEG-Y scan file");
			delete str;
		    }
		    else if ( !str->initRead(conn) )
		    {
			msg_ = str->errMsg();
			delete str;
		    }
		    else
			drdr_ = str;
		}
	    }
	    break;
	    case Seis::VolPS:
	    {
		drdr_ = new SEGYDirect3DPSReader( ioobj->mainFileName() );
	    }
	    break;
	    case Seis::LinePS:
		drdr_ = new SEGYDirect2DPSReader( ioobj->mainFileName(), lnm );
	    break;
	    case Seis::Line:
		msg_ = tr("2D seismics not supported");
	    break;
	}
    }
    delete ioobj;

    if ( drdr_ && !drdr_->errMsg().isEmpty() )
    {
	msg_ = drdr_->errMsg();
	deleteAndZeroPtr( drdr_ );
    }

    if ( dDef().isEmpty() )
    {
	msg_ = tr("Empty input scan");
	deleteAndZeroPtr( drdr_ );
    }

    if ( !drdr_ )
	return;

    if ( Seis::is2D(setup_.geom_) )
	totnr_ = dDef().lineData().positions().size();
    else
    {
	totnr_ = dDef().cubeData().totalSize();
	cdp_.toStart();
    }
}


SEGY::ReSorter::~ReSorter()
{
    wrapUp();
    delete posfilt_;
    delete drdr_;
    delete &cdp_;
}


const SEGY::DirectDef& SEGY::ReSorter::dDef() const
{
    return *drdr_->getDef();
}


const SEGY::FileDataSet& SEGY::ReSorter::fds() const
{
    return drdr_->getDef()->fileDataSet();
}


void SEGY::ReSorter::setFilter( const Pos::Filter& pf )
{
    delete posfilt_; posfilt_ = pf.clone();
}


int SEGY::ReSorter::wrapUp()
{
    // close everything
    delete outstrm_; outstrm_ = 0;
    deepErase( inpstrms_ );

    // prepare for new run ... (if ever needed)
    cdp_.toStart();
    delete [] trcbuf_; trcbuf_ = 0;
    setup_.curfnr_ = 0; nrdone_ = 0;
    needwritefilehdrs_ = true;

    return Finished();
}


int SEGY::ReSorter::nextStep()
{
    if ( !drdr_ )
	return ErrorOccurred();

    if ( binids_.isEmpty() )
	return fillBinIDs();

    BinID bid;
    if ( !getCurPos(bid) )
	return wrapUp();

    if ( !createOutput(bid) )
	return ErrorOccurred();

    return toNext() ? MoreToDo() : Finished();
}


int SEGY::ReSorter::fillBinIDs()
{
    Seis::PosKey pk; bool us;
    for ( od_int64 idx=0; idx<fds().size(); idx++ )
    {
	if ( fds().getDetails(idx,pk,us) )
	    binids_ += pk.binID();
    }

    if ( binids_.isEmpty() )
    {
	msg_ = tr("No positions in input");
	return ErrorOccurred();
    }

    msg_ = tr("Handling traces");
    return MoreToDo();
}


bool SEGY::ReSorter::toNext()
{
    nrdone_++;
    if ( Seis::is2D(setup_.geom_) )
	return nrdone_ < dDef().lineData().positions().size();
    else
	return dDef().cubeData().toNext(cdp_);
}


bool SEGY::ReSorter::getCurPos( BinID& bid )
{
    if ( Seis::is2D(setup_.geom_) )
    {
	const TypeSet<PosInfo::Line2DPos>& posns
				= dDef().lineData().positions();
	if ( nrdone_ >= posns.size() )
	    return false;
	bid.inl() = 1;
	bid.crl() = posns[(int)nrdone_].nr_;
    }
    else
    {
	mDynamicCastGet(Pos::Filter3D*,pf3d,posfilt_)
	const PosInfo::CubeData& cd = dDef().cubeData();

	while ( true )
	{
	    if ( !cd.isValid(cdp_) )
		return false;
	    bid = cd.binID( cdp_ );
	    if ( !pf3d || pf3d->includes(bid) )
		break;
	    if ( !toNext() )
		return false;
	}
    }

    return true;
}


bool SEGY::ReSorter::createOutput( const BinID& bid )
{
    const bool is2d = Seis::is2D( setup_.geom_ );

    if ( nrdone_ < 1 )
    {
	if ( is2d )
	    curinlrg_.start = curinlrg_.stop = 1;
	else
	    curinlrg_ = setup_.getInlRg( -1, dDef().cubeData() );
    }

    if ( nrdone_ < 1 || bid.inl() > curinlrg_.stop )
    {
	if ( bid.inl() > curinlrg_.stop )
	    curinlrg_ = setup_.getInlRg( bid.inl(), dDef().cubeData() );
	if ( !openOutputFile() )
	    return false;
    }

    int prevtidx = -1, tidx;
    while ( true )
    {
	if ( !getNext(bid,prevtidx,tidx) )
	    break;

	const FileDataSet::TrcIdx fdstidx = fds().getFileIndex( tidx );
	const int fidx = ensureFileOpen( fdstidx.filenr_ );
	if ( fidx < 0 )
	    return false;
	else if ( !readData(fidx,mCast(int,fdstidx.trcidx_)) )
	    return false;
	else if ( !writeData() )
	    return false;
    }

    return true;
}


bool SEGY::ReSorter::openOutputFile()
{
    const BufferString fnm( setup_.getFileName(curinlrg_) );
    delete outstrm_;
    outstrm_ = new od_ostream( fnm );
    if ( !outstrm_->isOK() )
	{ msg_ = uiStrings::phrCannotOpenForWrite( fnm ); return false; }

    needwritefilehdrs_ = true;
    return true;
}


bool SEGY::ReSorter::getNext( const BinID& bid, int& previdx,
			      int& newidx ) const
{
    const bool is2d = Seis::is2D(setup_.geom_);
    newidx = previdx;
    while ( true )
    {
	newidx++;
	if ( newidx >= binids_.size() )
	    break;
	else
	{
	    const BinID& pos( binids_[newidx] );
	    if ( pos.crl() == bid.crl() )
	    {
		if ( is2d || pos.inl() == bid.inl() )
		    { previdx = newidx; return true; }
	    }
	}
    }

    return false;
}


int SEGY::ReSorter::ensureFileOpen( int inpfidx )
{
    int fidx = -1;
    for ( int idx=0; idx<fidxs_.size(); idx++ )
	if ( fidxs_[idx] == inpfidx )
	    { fidx = idx; break ; }

    if ( fidx < 0 )
    {
	const BufferString fnm(
			drdr_->getDef()->fileDataSet().fileName(inpfidx) );
	od_istream* strm = new od_istream( fnm );
	if ( !strm->isOK() )
	{
	    msg_ = uiStrings::phrCannotOpenForRead( fnm );
	    delete strm;
	    return -1;
	}

	fidxs_ += inpfidx;
	fidx = inpstrms_.size();
	inpstrms_ += strm;
	inpfnms_.add( fnm );
    }

    return fidx;
}


bool SEGY::ReSorter::readData( int fidx, int trcidx )
{
    od_istream& odstrm = *inpstrms_[fidx];
    if ( !trcbuf_ )
    {
	odstrm.setReadPosition( 0 );
	if ( !odstrm.getBin(hdrbuf_,3600) )
	{
	    msg_ = tr("Cannot read SEG-Y file header. Empty file? -\n%1" )
			.arg( inpfnms_.get(fidx) );
	    return false;
	}
	odstrm.setReadPosition( 0 );
	SEGYSeisTrcTranslator trctr( "SEGY", "SEG-Y" );
	trctr.usePar( fds().segyPars() );
	StreamConn* sc = new StreamConn( odstrm );
	if ( !trctr.initRead(sc) || !trctr.commitSelections() )
	{
	    msg_ = tr("Cannot read '%1' for details. Corrupt file?")
		.arg( inpfnms_.get(fidx) );
	    return false;
	}
	trcbytes_ = trctr.traceSizeOnDisk();
	trcbuf_ = new unsigned char [trcbytes_];
    }

    odstrm.setReadPosition( 0 );
    od_stream::Pos pos = 3600 + trcbytes_ * trcidx;
    odstrm.setReadPosition( pos );
    if ( !odstrm.getBin(trcbuf_,trcbytes_) )
    {
	msg_ = tr("Cannot read trace.\nFile: %1\nTrace: %2")
		.arg( inpfnms_.get(fidx) )
		.arg( trcidx );
	return false;
    }

    return true;
}


bool SEGY::ReSorter::writeData()
{
    if ( needwritefilehdrs_ )
    {
	if ( !outstrm_->addBin(hdrbuf_,3600) )
	{
	    msg_ = tr("Cannot write file header to: %1")
		      .arg( outstrm_->fileName() );
	    outstrm_->addErrMsgTo( msg_ );
	    return false;
	}
	needwritefilehdrs_ = false;
    }

    if ( !outstrm_->addBin(trcbuf_,trcbytes_) )
    {
	msg_ = tr("Cannot write trace to:\n%1").arg( outstrm_->fileName() );
	outstrm_->addErrMsgTo( msg_ );
	return false;
    }

    return true;
}
