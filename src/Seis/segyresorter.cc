/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2008
-*/
static const char* rcsID = "$Id$";

#include "segyresorter.h"
#include "segydirectdef.h"
#include "segydirecttr.h"
#include "segytr.h"
#include "strmprov.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "posfilter.h"
#include "ioman.h"
#include "filepath.h"
#include "strmoper.h"

SEGY::ReSorter::Setup::Setup( Seis::GeomType gt, const MultiID& ky,
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

    FilePath fp( outfnm_ );
    const BufferString ext( fp.extension() );
    if ( !ext.isEmpty() )
	fp.setExtension( "" );

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
    int curidx = cd.indexOf( curinl );
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
    , msg_("Reading scan data")
    , nrdone_(0)
    , drdr_(0)
    , trcbuf_(0)
    , needwritefilehdrs_(true)
{
    IOObj* ioobj = IOM().get( setup_.inpkey_ );
    if ( !ioobj )
	msg_ = "Cannot find provided input in data manager";
    else
    {
	switch ( setup_.geom_ )
	{
	    case Seis::Vol:
	    {
		Translator* tr = ioobj->getTranslator();
		mDynamicCastGet(SEGYDirectSeisTrcTranslator*,str,tr)
		if ( !str )
		    { msg_ = "Input must be scanned SEG-Y cube"; delete tr; }
		else
		{
		    Conn* conn = ioobj->getConn( Conn::Read );
		    if ( !conn )
			{ msg_ = "Cannot open SEG-Y scan file"; delete str; }
		    else if ( !str->initRead(conn) )
			{ msg_ = str->errMsg(); delete str; delete conn; }
		    else
			drdr_ = str;
		}
	    }
	    break;
	    case Seis::VolPS:
	    {
		drdr_ = new SEGYDirect3DPSReader( ioobj->fullUserExpr(true) );
	    }
	    break;
	    case Seis::LinePS:
		drdr_ = new SEGYDirect2DPSReader( ioobj->fullUserExpr(true),
		       				  lnm );
	    break;
	    case Seis::Line:
		msg_ = "2D seismics not supported";
	    break;
	}
    }
    delete ioobj;

    if ( drdr_ && drdr_->errMsg() && *drdr_->errMsg() )
	{ msg_ = drdr_->errMsg(); delete drdr_; drdr_ = 0; }

    if ( dDef().isEmpty() )
	{ msg_ = "Empty input scan"; delete drdr_; drdr_ = 0; }

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
    sdout_.close();
    for ( int idx=0; idx<inpsds_.size(); idx++ )
	inpsds_[idx]->close();
    deepErase( inpsds_ );

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
	msg_ = "No positions in input";
	return ErrorOccurred();
    }

    msg_ = "Handling traces";
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
	bid.inl = 1;
	bid.crl = posns[(int)nrdone_].nr_;
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
    const bool isps = Seis::isPS( setup_.geom_ );

    if ( nrdone_ < 1 )
    {
	if ( is2d )
	    curinlrg_.start = curinlrg_.stop = 1;
	else
	    curinlrg_ = setup_.getInlRg( -1, dDef().cubeData() );
    }

    if ( nrdone_ < 1 || bid.inl > curinlrg_.stop )
    {
	if ( bid.inl > curinlrg_.stop )
	    curinlrg_ = setup_.getInlRg( bid.inl, dDef().cubeData() );
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
	else if ( !readData(fidx,fdstidx.trcidx_) )
	    return false;
	else if ( !writeData() )
	    return false;
    }

    return true;
}


bool SEGY::ReSorter::openOutputFile()
{
    const BufferString fnm( setup_.getFileName(curinlrg_) );
    sdout_.close();
    sdout_ = StreamProvider(fnm).makeOStream();
    if ( !sdout_.usable() )
    {
	msg_ = "Cannot open output file:\n"; msg_.add( fnm );
	return false;
    }

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
	    if ( pos.crl == bid.crl )
	    {
		if ( is2d || pos.inl == bid.inl )
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
	StreamData* sd = new StreamData( StreamProvider(fnm).makeIStream() );
	if ( !sd->usable() )
	{
	    msg_ = "Cannot open input file:\n"; msg_.add( fnm );
	    delete sd; return -1;
	}

	fidxs_ += inpfidx;
	fidx = inpsds_.size();
	inpsds_ += sd;
	inpfnms_.add( fnm );
    }

    return fidx;
}


bool SEGY::ReSorter::readData( int fidx, int trcidx )
{
    std::istream& strm( *inpsds_[ fidx ]->istrm );
    if ( !trcbuf_ )
    {
	StrmOper::seek( strm, 0 );
	if ( !StrmOper::readBlock(strm,hdrbuf_,3600) )
	{
	    msg_ = "Cannot read SEG-Y file header. Empty file? -\n";
	    msg_.add( inpfnms_.get(fidx) );
	    return false;
	}
	StrmOper::seek( strm, 0 );
	SEGYSeisTrcTranslator tr( "SEGY", "SEG-Y" );
	tr.usePar( fds().segyPars() );
	StreamConn* sc = new StreamConn( strm, false );
	if ( !tr.initRead(sc) || !tr.commitSelections() )
	{
	    msg_ = "Cannot read SEG-Y file details. Corrupt file? -\n";
	    msg_.add( inpfnms_.get(fidx) );
	    return false;
	}
	trcbytes_ = tr.traceSizeOnDisk();
	trcbuf_ = new unsigned char [trcbytes_];
    }

    od_int64 offs = 3600 + trcbytes_ * trcidx;
    StrmOper::seek( strm, offs );
    if ( !StrmOper::readBlock(strm,trcbuf_,trcbytes_) )
    {
	msg_ = "Cannot read trace.\nFile: ";
	msg_.add( inpfnms_.get(fidx) ).add( "\nTrace: " ).add( trcidx );
	return false;
    }

    return true;
}


bool SEGY::ReSorter::writeData()
{
    if ( needwritefilehdrs_ )
    {
	if ( !StrmOper::writeBlock(*sdout_.ostrm,hdrbuf_,3600) )
	{
	    msg_ = "Cannot write file header to: ";
	    msg_.add( sdout_.fileName() );
	    return false;
	}
	needwritefilehdrs_ = false;
    }

    if ( !StrmOper::writeBlock(*sdout_.ostrm,trcbuf_,trcbytes_) )
    {
	msg_ = "Cannot write trace to:\n";
	msg_.add( sdout_.fileName() );
	return false;
    }

    return true;
}
