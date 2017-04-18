/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

-*/

#include "seisblockswriter.h"
#include "seistrc.h"
#include "posidxpairdataset.h"
#include "paralleltask.h"
#include "executor.h"
#include "uistrings.h"
#include "scaler.h"
#include "datachar.h"
#include "file.h"
#include "oddirs.h"
#include "genc.h"
#include "keystrs.h"
#include "posinfo.h"
#include "survgeom3d.h"
#include "ascstream.h"
#include "separstr.h"

static const unsigned short cHdrSz = 128;


Seis::Blocks::Writer::Column::Column( const Dimensions& dims, int nrcomps )
    : nruniquevisits_(0)
    , dims_(dims)
{
    for ( int icomp=0; icomp<nrcomps; icomp++ )
	blocksets_ += new BlockSet;

    visited_ = new bool* [dims_.inl()];
    for ( IdxType iinl=0; iinl<dims_.inl(); iinl++ )
    {
	visited_[iinl] = new bool [dims_.crl()];
	for ( IdxType icrl=0; icrl<dims_.crl(); icrl++ )
	    visited_[iinl][icrl] = false;
    }
}


Seis::Blocks::Writer::Column::~Column()
{
    deepErase(blocksets_);
    for ( IdxType idx=0; idx<dims_.inl(); idx++ )
	delete [] visited_[idx];
    delete [] visited_;
}


void Seis::Blocks::Writer::Column::retireAll()
{
    for ( int iset=0; iset<blocksets_.size(); iset++ )
    {
	BlockSet& bset = *blocksets_[iset];
	for ( int iblock=0; iblock<bset.size(); iblock++ )
	    bset[iblock]->retire();
    }
}


void Seis::Blocks::Writer::Column::getDefArea( SampIdx& start,
					       Dimensions& dims ) const
{
    IdxType mininl = dims_.inl()-1, mincrl = dims_.crl()-1;
    IdxType maxinl = 0, maxcrl = 0;

    for ( IdxType iinl=0; iinl<dims_.inl(); iinl++ )
    {
	for ( IdxType icrl=0; icrl<dims_.crl(); icrl++ )
	{
	    if ( visited_[iinl][icrl] )
	    {
		if ( mininl > iinl )
		    mininl = iinl;
		if ( mincrl > icrl )
		    mincrl = icrl;
		if ( maxinl < iinl )
		    maxinl = iinl;
		if ( maxcrl < icrl )
		    maxcrl = icrl;
	    }
	}
    }

    start.inl() = mininl;
    start.crl() = mincrl;
    dims.inl() = maxinl - mininl + 1;
    dims.crl() = maxcrl - mincrl + 1;
}


Seis::Blocks::Writer::Writer( const SurvGeom* geom )
    : IOClass(geom)
    , basepath_(GetBaseDataDir(),sSeismicSubDir())
    , scaler_(0)
    , specfprep_(OD::AutoFPRep)
    , usefprep_(OD::F32)
    , needreset_(true)
    , columns_(*new Pos::IdxPairDataSet(sizeof(Column*),false,false))
    , nrcomponents_(1)
    , nrpospercolumn_(((int)dims_.inl()) * dims_.crl())
    , isfinished_(false)
{
}


Seis::Blocks::Writer::~Writer()
{
    if ( !isfinished_ )
    {
	Task* task = finisher();
	if ( task )
	{
	    task->execute();
	    delete task;
	}
    }

    deepErase( auxiops_ );
    delete scaler_;
    setEmpty();
    delete &columns_;
}


// This function + the macros keep IdxPairDataSet debugging possible
// without dependencies in the header file

inline static Seis::Blocks::Writer::Column* gtColumn( Pos::IdxPairDataSet& ds,
				    const Pos::IdxPairDataSet::SPos& spos )
{
    return (Seis::Blocks::Writer::Column*)ds.getObj( spos );
}

#define mGetColumn(spos) gtColumn( columns_, spos )
#define mGetWrrColumn(wrr,spos) gtColumn( wrr_.columns_, spos )


void Seis::Blocks::Writer::setEmpty()
{
    Pos::IdxPairDataSet::SPos spos;
    while ( columns_.next(spos) )
	delete mGetColumn( spos );
    columns_.setEmpty();
    deepErase( zevalpositions_ );
}


void Seis::Blocks::Writer::setBasePath( const File::Path& fp )
{
    if ( fp.fullPath() != basepath_.fullPath() )
    {
	basepath_ = fp;
	needreset_ = true;
    }
}


void Seis::Blocks::Writer::setFileNameBase( const char* nm )
{
    if ( filenamebase_ != nm )
    {
	filenamebase_ = nm;
	needreset_ = true;
    }
}


void Seis::Blocks::Writer::setFPRep( OD::FPDataRepType rep )
{
    if ( specfprep_ != rep )
    {
	specfprep_ = rep;
	needreset_ = true;
    }
}


void Seis::Blocks::Writer::setCubeName( const char* nm )
{
    cubename_ = nm;
}


void Seis::Blocks::Writer::addComponentName( const char* nm )
{
    compnms_.add( nm );
}


void Seis::Blocks::Writer::addAuxInfo( const char* key, const IOPar& iop )
{
    BufferString auxkey( key );
    if ( auxkey.isEmpty() )
	{ pErrMsg( "Refusing to add section without key" ); return; }

    IOPar* toadd = new IOPar( iop );
    toadd->setName( BufferString(sKeySectionPre(),auxkey) );
    auxiops_ += toadd;
}


void Seis::Blocks::Writer::setScaler( const LinScaler* newscaler )
{
    if ( (!scaler_ && !newscaler) )
	return;

    delete scaler_;
    scaler_ = newscaler ? newscaler->clone() : 0;
    needreset_ = true;
}


void Seis::Blocks::Writer::resetZ( const Interval<float>& zrg )
{
    globzidxrg_.start = Block::globIdx4Z( survgeom_, zrg.start, dims_.z() );
    globzidxrg_.stop = Block::globIdx4Z( survgeom_, zrg.stop, dims_.z() );
    const float eps = Seis::cDefSampleSnapDist();
    deepErase( zevalpositions_ );

    bool emptystart = false, emptyend = false;
    for ( IdxType globzidx=globzidxrg_.start; globzidx<=globzidxrg_.stop;
		globzidx++ )
    {
	ZEvalPosSet* posset = new ZEvalPosSet;
	for ( IdxType sampzidx=0; sampzidx<dims_.z(); sampzidx++ )
	{
	    const float z = Block::z4Idxs( survgeom_, dims_.z(),
					  globzidx, sampzidx );
	    if ( z > zrg.start-eps && z < zrg.stop+eps )
		*posset += ZEvalPos( sampzidx, z );
	}
	if ( posset->isEmpty() )
	    (globzidx==globzidxrg_.start ? emptystart : emptyend) = true;
	else
	     zevalpositions_ += posset;
    }
    if ( emptystart )
	globzidxrg_.start++;
    else if ( emptyend )
	globzidxrg_.stop--;

    if ( zevalpositions_.isEmpty() )
    {
	// only possibility is that zrg is entirely between two output samples.
	// As a service, we'll make sure the nearest sample is set
	const float zeval = zrg.center();
	const IdxType globzidx = Block::globIdx4Z( survgeom_, zeval, dims_.z());
	const IdxType sampzidx = Block::sampIdx4Z( survgeom_, zeval, dims_.z());
	globzidxrg_.start = globzidxrg_.stop = globzidx;
	ZEvalPosSet* posset = new ZEvalPosSet;
	*posset += ZEvalPos( sampzidx, zeval );
	zevalpositions_ += posset;
    }
}


BufferString Seis::Blocks::Writer::dirName() const
{
    File::Path fp( basepath_ );
    fp.add( filenamebase_ );
    return fp.fullPath();
}


BufferString Seis::Blocks::Writer::mainFileName() const
{
    File::Path fp( basepath_ );
    fp.add( filenamebase_ );
    fp.setExtension( "cube", false );
    return fp.fullPath();
}


bool Seis::Blocks::Writer::removeExisting( const char* fnm,
					   uiRetVal& uirv ) const
{
    if ( File::exists(fnm) )
    {
	if ( !File::isDirectory(fnm) )
	{
	    if ( !File::remove(fnm) )
	    {
		uirv.add( tr("Cannot remove file:\n%1").arg(fnm) );
		return false;
	    }
	}
	else if ( !File::removeDir(fnm) )
	{
	    uirv.add( tr("Cannot remove directory:\n%1").arg(fnm) );
	    return false;
	}
    }
    return true;
}


bool Seis::Blocks::Writer::prepareWrite( uiRetVal& uirv )
{
    const BufferString mainfnm = mainFileName();
    const BufferString dirnm = dirName();
    if ( !removeExisting(mainfnm,uirv) || !removeExisting(dirnm,uirv) )
        return false;

    if ( !File::createDir(dirnm) )
    {
	uirv.add( tr("Cannot create directory:\n%1").arg(dirnm) );
	return false;
    }

    return true;
}


uiRetVal Seis::Blocks::Writer::add( const SeisTrc& trc )
{
    uiRetVal uirv;
    if ( needreset_ )
    {
	needreset_ = false;
	isfinished_ = false;
	setEmpty();
	if ( !prepareWrite(uirv) )
	    return uirv;

	resetZ( Interval<float>(trc.startPos(),trc.endPos()) );
	usefprep_ = specfprep_;
	if ( usefprep_ == OD::AutoFPRep )
	    usefprep_ = trc.data().getInterpreter()->dataChar().userType();

	nrcomponents_ = trc.nrComponents();
    }

    const BinID bid = trc.info().binID();
    const GlobIdx globidx( Block::globIdx4Inl(survgeom_,bid.inl(),dims_.inl()),
			   Block::globIdx4Crl(survgeom_,bid.crl(),dims_.crl()),
			   0 );
    const SampIdx sampidx( Block::sampIdx4Inl(survgeom_,bid.inl(),dims_.inl()),
			   Block::sampIdx4Crl(survgeom_,bid.crl(),dims_.crl()),
			   0 );

    Column* column = getColumn( globidx );
    if ( !column )
    {
	uirv.set( tr("Memory needed for writing process unavailable.") );
	setEmpty();
	return uirv;
    }

    for ( int icomp=0; icomp<nrcomponents_; icomp++ )
    {
	Column::BlockSet& blockset = *column->blocksets_[icomp];
	for ( int iblock=0; iblock<blockset.size(); iblock++ )
	{
	    const ZEvalPosSet& posset = *zevalpositions_[iblock];
	    Block& block = *blockset[iblock];
	    add2Block( block, posset, sampidx, trc, icomp );
	}
    }

    if ( isCompletionVisit(*column,sampidx) )
	writeColumn( *column, uirv );

    return uirv;
}


void Seis::Blocks::Writer::add2Block( Block& block, const ZEvalPosSet& zevals,
			SampIdx sampidx, const SeisTrc& trc, int icomp )
{
    if ( block.isRetired() )
	return; // new visit of already written. Won't do, but no error.

    for ( int idx=0; idx<zevals.size(); idx++ )
    {
	const ZEvalPos& evalpos = zevals[idx];
	sampidx.z() = evalpos.first;
	float val2set = trc.getValue( evalpos.second, icomp );
	if ( scaler_ )
	    val2set = (float)scaler_->scale( val2set );
	block.setValue( sampidx, val2set );
    }
}


Seis::Blocks::Writer::Column*
Seis::Blocks::Writer::mkNewColumn( const GlobIdx& globidx )
{
    Column* column = new Column( dims_, nrcomponents_ );

    for ( IdxType globzidx=globzidxrg_.start; globzidx<=globzidxrg_.stop;
	    globzidx++ )
    {
	const ZEvalPosSet& evalposs
		= *zevalpositions_[globzidx-globzidxrg_.start];
	GlobIdx curgidx( globidx ); curgidx.z() = globzidx;
	Dimensions dims( dims_ ); SampIdx start;
	if ( globzidx == globzidxrg_.start )
	{
	    start.z() = (IdxType)(dims_.z() - evalposs.size());
	    dims.z() = dims_.z() - (SzType)start.z();
	}
	else if ( globzidx == globzidxrg_.stop )
	    dims.z() = (SzType)evalposs.size();

	for ( int icomp=0; icomp<nrcomponents_; icomp++ )
	{
	    Block* block = new Block( curgidx, start, dims, usefprep_ );
	    if ( block->isRetired() )
		{ delete column; return 0; }
	    block->zero();
	    *column->blocksets_[icomp] += block;
	}
    }

    return column;
}


Seis::Blocks::Writer::Column*
Seis::Blocks::Writer::getColumn( const GlobIdx& globidx )
{
    const Pos::IdxPair idxpair( globidx.inl(), globidx.crl() );
    Pos::IdxPairDataSet::SPos spos = columns_.find( idxpair );
    Column* column = 0;
    if ( spos.isValid() )
	column = mGetColumn( spos );
    else
    {
	column = mkNewColumn( globidx );
	if ( column )
	    columns_.add( idxpair, column );
    }
    return column;
}


bool Seis::Blocks::Writer::isCompletionVisit( Column& column,
					      const SampIdx& sampidx ) const
{
    bool& visited = column.visited_[sampidx.inl()][sampidx.crl()];
    if ( !visited )
    {
	column.nruniquevisits_++;
	visited = true;
    }

    return column.nruniquevisits_ == nrpospercolumn_;
}


namespace Seis
{
namespace Blocks
{

class ColumnWriter : public Executor
{ mODTextTranslationClass(Seis::Blocks::ColumnWriter)
public:

    typedef Writer::Column  Column;

ColumnWriter( Writer& wrr, Column& colmn, const char* fnm )
    : Executor("Column File Writer")
    , wrr_(wrr)
    , column_(colmn)
    , strm_(fnm)
    , nrblocks_(wrr_.nrColumnBlocks())
    , iblock_(0)
{
    if ( strm_.isBad() )
	setErr( true );
    else
    {
	column_.getDefArea( start_, dims_ );
	start_.z() = 0;
	if ( !wrr_.writeColumnHeader(strm_,column_,start_,dims_) )
	    setErr();
    }
}

virtual od_int64 nrDone() const { return iblock_; }
virtual od_int64 totalNr() const { return nrblocks_; }
virtual uiString nrDoneText() const { return tr("Blocks written"); }
virtual uiString message() const
{
    if ( uirv_.isError() )
	return uirv_;
    return tr("Writing Traces");
}

virtual int nextStep()
{
    if ( uirv_.isError() )
	return ErrorOccurred();
    else if ( iblock_ >= nrblocks_ )
	return Finished();

    for ( int icomp=0; icomp<wrr_.nrcomponents_; icomp++ )
    {
	Writer::Column::BlockSet& blockset = *column_.blocksets_[icomp];
	Block& block = *blockset[iblock_];
	Dimensions wrdims( dims_ ); wrdims.z() = block.dims().z();
	if ( !wrr_.writeBlock( strm_, block, start_, wrdims ) )
	    { setErr(); return ErrorOccurred(); }
    }

    iblock_++;
    return MoreToDo();
}

void setErr( bool initial=false )
{
    uirv_.set( initial ? uiStrings::phrCannotOpen(toUiString(strm_.fileName()))
	    : uiStrings::phrCannotWrite( toUiString(strm_.fileName()) ) );
    strm_.addErrMsgTo( uirv_ );
}

    od_ostream		strm_;
    Writer&		wrr_;
    Column&		column_;
    Dimensions		dims_;
    SampIdx		start_;
    const int		nrblocks_;
    int			iblock_;
    uiRetVal		uirv_;

};

} // namespace Blocks
} // namespace Seis


void Seis::Blocks::Writer::writeColumn( Column& column, uiRetVal& uirv )
{
    File::Path fp( basepath_ );
    fp.add( filenamebase_ )
      .add( fileNameFor(column.firstBlock().globIdx()) );
    ColumnWriter wrr( *this, column, fp.fullPath() );
    if ( !wrr.execute() )
	uirv = wrr.uirv_;
}


bool Seis::Blocks::Writer::writeColumnHeader( od_ostream& strm,
    const Column& column, const SampIdx& start, const Dimensions& dims ) const
{
    const Block& firstblock = column.firstBlock();
    const GlobIdx& globidx = firstblock.globIdx();
    int zdim = 0;
    for ( int idx=0; idx<zevalpositions_.size(); idx++ )
	zdim += zevalpositions_[idx]->size();
    int zstart = globidx.z(); zstart *= dims_.z();
    zstart += firstblock.start().z();
    const unsigned short dfmt = (unsigned short)usefprep_;

    strm.addBin( cHdrSz ).addBin( version_ );
    strm.addBin( globidx.inl() ).addBin( globidx.crl() ).addBin( globidx.z() );
    strm.addBin( start.inl() ).addBin( start.crl() ).addBin( zstart );
    strm.addBin( dims.inl() ).addBin( dims.crl() ).addBin( zdim );
    strm.addBin( dfmt );
    char buf[cHdrSz]; OD::memZero( buf, cHdrSz );
    if ( scaler_ )
    {
	// write the scaler needed to reconstruct the org values
	LinScaler* invscaler = scaler_->inverse();
	float* ptr = (float*)buf;
	*ptr++ = (float)invscaler->constant_;
	*ptr = (float)invscaler->factor_;
	delete invscaler;
    }
    strm.addBin( buf, 2*sizeof(float) );
    survgeom_.putStructure( buf );
    strm.addBin( buf, survgeom_.bufSize4Structure() );

    const int bytes_left_in_hdr = cHdrSz - (int)strm.position();
    OD::memZero( buf, bytes_left_in_hdr );
    strm.addBin( buf, bytes_left_in_hdr );

    return strm.isOK();
}


bool Seis::Blocks::Writer::writeBlock( od_ostream& strm, Block& block,
					SampIdx wrstart, Dimensions wrdims )
{
    const DataBuffer& dbuf = block.dataBuf();
    const Dimensions& blockdims = block.dims();

    if ( wrdims == blockdims )
	strm.addBin( dbuf.data(), dbuf.totalBytes() );
    else
    {
	const DataBuffer::buf_type* bufdata = dbuf.data();
	const int bytespersample = dbuf.bytesPerElement();
	const int bytesperentirecrl = bytespersample * blockdims.z();
	const int bytesperentireinl = bytesperentirecrl * blockdims.crl();

	const int bytes2write = wrdims.z() * bytespersample;
	const IdxType wrstopinl = wrstart.inl() + wrdims.inl();
	const IdxType wrstopcrl = wrstart.crl() + wrdims.crl();

	const DataBuffer::buf_type* dataptr;
	for ( IdxType iinl=wrstart.inl(); iinl<wrstopinl; iinl++ )
	{
	    dataptr = bufdata + iinl * bytesperentireinl
			      + wrstart.crl() * bytesperentirecrl
			      + wrstart.z() * bytespersample;
	    for ( IdxType icrl=wrstart.crl(); icrl<wrstopcrl; icrl++ )
	    {
		strm.addBin( dataptr, bytes2write );
		dataptr += bytesperentirecrl;
	    }
	}
    }

    block.retire();
    return strm.isOK();
}


void Seis::Blocks::Writer::writeMainFile( uiRetVal& uirv )
{
    od_ostream strm( mainFileName() );
    if ( strm.isBad() )
    {
	uirv.add( uiStrings::phrCannotOpen( toUiString(strm.fileName()) ) );
	return;
    }

    if ( !writeMainFileData(strm) )
    {
	uirv.add( uiStrings::phrCannotWrite( toUiString(strm.fileName()) ) );
	return;
    }
}


bool Seis::Blocks::Writer::writeMainFileData( od_ostream& strm )
{
    ascostream ascostrm( strm );
    if ( !ascostrm.putHeader(sKeyFileType()) )
	return false;

    PosInfo::CubeData cubedata;
    Interval<IdxType> globinlrg, globcrlrg;
    Interval<int> inlrg, crlrg;
    Interval<double> xrg, yrg;
    Interval<float> zrg;
    scanPositions( cubedata, globinlrg, globcrlrg, inlrg, crlrg, xrg, yrg );
    zrg.start = zevalpositions_.first()->first().second;
    zrg.stop = zevalpositions_.last()->last().second;

    IOPar iop( sKeyGenSection() );
    iop.set( sKeyFmtVersion(), version_ );
    iop.set( sKey::Name(), cubename_ );
    survgeom_.putStructure( iop );
    iop.set( sKey::DataStorage(), DataCharacteristics::toString(usefprep_) );
    if ( scaler_ )
    {
	// write the scaler needed to reconstruct the org values
	LinScaler* invscaler = scaler_->inverse();
	char buf[1024]; invscaler->put( buf );
	iop.set( sKey::Scale(), buf );
	delete invscaler;
    }
    iop.set( sKeyDimensions(), dims_.inl(), dims_.crl(), dims_.z() );
    iop.set( sKeyGlobInlRg(), globinlrg );
    iop.set( sKeyGlobCrlRg(), globcrlrg );
    iop.set( sKeyGlobZRg(), globzidxrg_ );
    iop.set( sKey::XRange(), xrg );
    iop.set( sKey::YRange(), yrg );
    iop.set( sKey::ZRange(), zrg );
    iop.set( sKey::InlRange(), inlrg );
    iop.set( sKey::CrlRange(), crlrg );

    FileMultiString fms;
    for ( int icomp=0; icomp<nrcomponents_; icomp++ )
    {
	BufferString nm;
	if ( icomp < compnms_.size() )
	    nm = compnms_.get( icomp );
	else
	    nm.set( "Component " ).add( icomp+1 );
	fms += nm;
    }
    iop.set( sKeyComponents(), fms );

    iop.putTo( ascostrm );
    if ( !strm.isOK() )
	return false;

    for ( int idx=0; idx<auxiops_.size(); idx++ )
    {
	auxiops_[idx]->putTo( ascostrm );
	if ( !strm.isOK() )
	    return false;
    }

    strm << sKeyPosSection() << od_endl;
    return cubedata.write( strm, true );
}


void Seis::Blocks::Writer::scanPositions( PosInfo::CubeData& cubedata,
	Interval<IdxType>& globinlrg, Interval<IdxType>& globcrlrg,
	Interval<int>& inlrg, Interval<int>& crlrg,
	Interval<double>& xrg, Interval<double>& yrg )
{
    Pos::IdxPairDataSet sortedpositions( 0, false );
    Pos::IdxPairDataSet::SPos spos;
    bool first = true;
    while ( columns_.next(spos) )
    {
	const Column& column = *mGetColumn( spos );
	if ( column.nruniquevisits_ < 1 )
	    continue;

	const Block& block = column.firstBlock();
	GlobIdx globidx = block.globIdx();
	if ( first )
	{
	    globinlrg.start = globinlrg.stop = globidx.inl();
	    globcrlrg.start = globcrlrg.stop = globidx.crl();
	}
	else
	{
	    globinlrg.include( globidx.inl(), false );
	    globcrlrg.include( globidx.crl(), false );
	}

	for ( IdxType iinl=0; iinl<dims_.inl(); iinl++ )
	{
	    for ( IdxType icrl=0; icrl<dims_.crl(); icrl++ )
	    {
		if ( !column.visited_[iinl][icrl] )
		    continue;
		const int inl = Block::inl4Idxs( survgeom_, dims_.inl(),
					    globidx.inl(), iinl );
		const int crl = Block::inl4Idxs( survgeom_, dims_.crl(),
					    globidx.crl(), icrl );
		const Coord coord = survgeom_.toCoord( inl, crl );
		if ( first )
		{
		    inlrg.start = inlrg.stop = inl;
		    crlrg.start = crlrg.stop = crl;
		    xrg.start = xrg.stop = coord.x_;
		    yrg.start = yrg.stop = coord.y_;
		}
		else
		{
		    inlrg.include( inl, false );
		    crlrg.include( crl, false );
		    xrg.include( coord.x_, false );
		    yrg.include( coord.y_, false );
		}

		sortedpositions.add( BinID(inl,crl) );
	    }
	}

	first = false;
    }

    PosInfo::CubeDataFiller cdf( cubedata );
    spos.reset();
    while ( sortedpositions.next(spos) )
    {
	const Pos::IdxPair ip( sortedpositions.getIdxPair( spos ) );
	cdf.add( BinID(ip.inl(),ip.crl()) );
    }
}


namespace Seis
{
namespace Blocks
{

class WriterFinisher : public ParallelTask
{ mODTextTranslationClass(Seis::Blocks::WriterFinisher)
public:

    typedef Writer::Column  Column;

WriterFinisher( Writer& wrr )
    : ParallelTask("Seis Blocks Writer finisher")
    , wrr_(wrr)
{
    Pos::IdxPairDataSet::SPos spos;
    while ( wrr_.columns_.next(spos) )
    {
	Writer::Column* column = mGetWrrColumn( wrr_, spos );
	if ( column->nruniquevisits_ < 1 )
	    column->retireAll();
	else if ( !column->firstBlock().isRetired() )
	    towrite_ += column;
    }
}

virtual od_int64 nrIterations() const
{
    return towrite_.size();
}

virtual uiString nrDoneText() const
{
   return tr("Column files written");
}

virtual uiString message() const
{
    if ( uirv_.isOK() )
       return tr("Writing edge blocks");
    return uirv_;
}

virtual bool doWork( od_int64 startidx, od_int64 stopidx, int )
{
    uiRetVal uirv;
    for ( int idx=(int)startidx; idx<=(int)stopidx; idx++ )
    {
	wrr_.writeColumn( *towrite_[idx], uirv );
	if ( uirv.isError() )
	    { uirv_.add( uirv ); return false; }
	addToNrDone( 1 );
    }

    return true;
}

virtual bool doFinish( bool )
{
    uiRetVal uirv;
    wrr_.writeMainFile( uirv );
    uirv_.add( uirv );
    wrr_.isfinished_ = true;
    return uirv.isOK();
}

    uiRetVal		uirv_;
    Writer&		wrr_;
    ObjectSet<Column>	towrite_;

};

} // namespace Blocks
} // namespace Seis


Task* Seis::Blocks::Writer::finisher()
{
    WriterFinisher* wf = new WriterFinisher( *this );
    if ( wf->towrite_.isEmpty() )
	{ delete wf; wf = 0; }
    return wf;
}
