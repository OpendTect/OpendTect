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
#include "keystrs.h"
#include "posinfo.h"
#include "survgeom3d.h"
#include "survinfo.h" // for survey name only
#include "ascstream.h"
#include "separstr.h"


namespace Seis
{
namespace Blocks
{

/*!\brief Block with data buffer collecting data to be written. */

class MemBlock : public Block
{
public:

			MemBlock(GlobIdx,SampIdx start=SampIdx(),
				   Dimensions dims=defDims(),
				   OD::FPDataRepType fpr=OD::F32);

    void		zero()			{ dbuf_.zero(); }
    void		retire()		{ dbuf_.reSize( 0, false ); }
    bool		isRetired() const	{ return dbuf_.isEmpty(); }

    float		value(const SampIdx&) const;
    void		setValue(const SampIdx&,float);

    DataBuffer		dbuf_;

protected:

    int			getBufIdx(const SampIdx&) const;

};


class MemBlockColumn : public Column
{
public:

    typedef ManagedObjectSet<MemBlock>    BlockSet;

			MemBlockColumn(const GlobIdx&,const Dimensions&,
					int nrcomps);
			~MemBlockColumn();

    MemBlock&		firstBlock()	{ return *blocksets_.first()->first(); }
    const MemBlock&	firstBlock() const
					{ return *blocksets_.first()->first(); }
    void		retireAll();
    void		getDefArea(SampIdx&,Dimensions&) const;

    ObjectSet<BlockSet> blocksets_; // one set per component
    bool**		visited_;
    int			nruniquevisits_;
};

} // namespace Blocks

} // namespace Seis


Seis::Blocks::MemBlock::MemBlock( GlobIdx gidx, SampIdx strt,
				      Dimensions dms, OD::FPDataRepType fpr )
    : Block(gidx,strt,dms)
    , dbuf_(0)
{
    const DataCharacteristics dc( fpr );
    interp_ = DataInterpreter<float>::create( dc, true );
    const int bytesperval = (int)dc.nrBytes();
    dbuf_.reByte( bytesperval, false );
    const int totsz = (((int)dims_.inl())*dims_.crl()) * dims_.z();
    dbuf_.reSize( totsz, false );
}


int Seis::Blocks::MemBlock::getBufIdx( const SampIdx& sidx ) const
{
    const int nrsampsoninl = nrSampsOnInl( dims_, sidx );
    return sidx.inl() ? sidx.inl()*nrSampsPerInl(dims_) + nrsampsoninl
		      : nrsampsoninl;
}


float Seis::Blocks::MemBlock::value( const SampIdx& sidx ) const
{
    return interp_->get( dbuf_.data(), getBufIdx(sidx) );
}


void Seis::Blocks::MemBlock::setValue( const SampIdx& sidx, float val )
{
    interp_->put( dbuf_.data(), getBufIdx(sidx), val );
}



Seis::Blocks::MemBlockColumn::MemBlockColumn( const GlobIdx& gidx,
					      const Dimensions& dims,
					      int nrcomps )
    : Column(gidx,dims,nrcomps)
    , nruniquevisits_(0)
{
    for ( int icomp=0; icomp<nrcomps_; icomp++ )
	blocksets_ += new BlockSet;

    visited_ = new bool* [dims_.inl()];
    for ( IdxType iinl=0; iinl<dims_.inl(); iinl++ )
    {
	visited_[iinl] = new bool [dims_.crl()];
	for ( IdxType icrl=0; icrl<dims_.crl(); icrl++ )
	    visited_[iinl][icrl] = false;
    }
}


Seis::Blocks::MemBlockColumn::~MemBlockColumn()
{
    deepErase(blocksets_);
    for ( IdxType idx=0; idx<dims_.inl(); idx++ )
	delete [] visited_[idx];
    delete [] visited_;
}


void Seis::Blocks::MemBlockColumn::retireAll()
{
    for ( int iset=0; iset<blocksets_.size(); iset++ )
    {
	BlockSet& bset = *blocksets_[iset];
	for ( int iblock=0; iblock<bset.size(); iblock++ )
	    bset[iblock]->retire();
    }
}


void Seis::Blocks::MemBlockColumn::getDefArea( SampIdx& start,
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
    : survgeom_(*(geom ? geom : static_cast<const SurvGeom*>(
					&SurvGeom::default3D())))
    , specfprep_(OD::AutoFPRep)
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

    deepErase( zevalpositions_ );
}


void Seis::Blocks::Writer::setEmpty()
{
    clearColumns();
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
    const BufferString dirnm = dataDirName();
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
    Threads::Locker locker( accesslock_ );
    if ( needreset_ )
    {
	needreset_ = false;
	isfinished_ = false;
	setEmpty();
	if ( !prepareWrite(uirv) )
	    return uirv;

	resetZ( Interval<float>(trc.startPos(),trc.endPos()) );
	fprep_ = specfprep_;
	if ( fprep_ == OD::AutoFPRep )
	    fprep_ = trc.data().getInterpreter()->dataChar().userType();

	nrcomponents_ = trc.nrComponents();
    }

    const BinID bid = trc.info().binID();
    const GlobIdx globidx( Block::globIdx4Inl(survgeom_,bid.inl(),dims_.inl()),
			   Block::globIdx4Crl(survgeom_,bid.crl(),dims_.crl()),
			   0 );
    const SampIdx sampidx( Block::sampIdx4Inl(survgeom_,bid.inl(),dims_.inl()),
			   Block::sampIdx4Crl(survgeom_,bid.crl(),dims_.crl()),
			   0 );

    MemBlockColumn* column = getColumn( globidx );
    if ( !column )
    {
	uirv.set( tr("Memory needed for writing process unavailable.") );
	setEmpty();
	return uirv;
    }
    else if ( isCompleted(*column) )
	return uirv; // this check is absolutely necessary to for MT writing

    for ( int icomp=0; icomp<nrcomponents_; icomp++ )
    {
	MemBlockColumn::BlockSet& blockset = *column->blocksets_[icomp];
	for ( int iblock=0; iblock<blockset.size(); iblock++ )
	{
	    const ZEvalPosSet& posset = *zevalpositions_[iblock];
	    MemBlock& block = *blockset[iblock];
	    add2Block( block, posset, sampidx, trc, icomp );
	}
    }

    bool& visited = column->visited_[sampidx.inl()][sampidx.crl()];
    if ( !visited )
    {
	column->nruniquevisits_++;
	visited = true;
    }

    if ( isCompleted(*column) )
    {
	locker.unlockNow();
	writeColumn( *column, uirv );
    }

    return uirv;
}


void Seis::Blocks::Writer::add2Block( MemBlock& block,
			const ZEvalPosSet& zevals, SampIdx sampidx,
			const SeisTrc& trc, int icomp )
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


Seis::Blocks::MemBlockColumn*
Seis::Blocks::Writer::mkNewColumn( GlobIdx globidx )
{
    globidx.z() = globzidxrg_.start;
    MemBlockColumn* column = new MemBlockColumn( globidx, dims_, nrcomponents_);

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
	    MemBlock* block = new MemBlock( curgidx, start, dims, fprep_ );
	    if ( block->isRetired() )
		{ delete column; return 0; }
	    block->zero();
	    *column->blocksets_[icomp] += block;
	}
    }

    return column;
}


Seis::Blocks::MemBlockColumn*
Seis::Blocks::Writer::getColumn( const GlobIdx& globidx )
{
    Column* column = findColumn( globidx );
    if ( !column )
    {
	column = mkNewColumn( globidx );
	addColumn( column );
    }
    return (MemBlockColumn*)column;
}


bool Seis::Blocks::Writer::isCompleted( const MemBlockColumn& column ) const
{
    return column.nruniquevisits_ == nrpospercolumn_;
}


namespace Seis
{
namespace Blocks
{

class ColumnWriter : public Executor
{ mODTextTranslationClass(Seis::Blocks::ColumnWriter)
public:

ColumnWriter( Writer& wrr, MemBlockColumn& colmn, const char* fnm )
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
	start_.z() = colmn.firstBlock().start().z();
	dims_.z() = wrr.dimensions().z();
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
	MemBlockColumn::BlockSet& blockset = *column_.blocksets_[icomp];
	MemBlock& block = *blockset[iblock_];
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
    MemBlockColumn&	column_;
    Dimensions		dims_;
    SampIdx		start_;
    const int		nrblocks_;
    int			iblock_;
    uiRetVal		uirv_;

};

} // namespace Blocks
} // namespace Seis


void Seis::Blocks::Writer::writeColumn( MemBlockColumn& column, uiRetVal& uirv )
{
    File::Path fp( basepath_ );
    fp.add( filenamebase_ )
      .add( fileNameFor(column.globidx_) );
    ColumnWriter wrr( *this, column, fp.fullPath() );
    if ( !wrr.execute() )
	uirv = wrr.uirv_;
}


bool Seis::Blocks::Writer::writeColumnHeader( od_ostream& strm,
		    const MemBlockColumn& column, const SampIdx& start,
		    const Dimensions& dims ) const
{
    const Block& firstblock = column.firstBlock();
    const GlobIdx& globidx = column.globidx_;
    int nrsamples = 0;
    for ( int idx=0; idx<zevalpositions_.size(); idx++ )
	nrsamples += zevalpositions_[idx]->size();
    int zstart = globidx.z(); zstart *= dims_.z();
    zstart += firstblock.start().z();
    const unsigned short dfmt = (unsigned short)fprep_;

    const HdrSzVersionType hdrsz = columnHeaderSize( version_ );
    strm.addBin( hdrsz ).addBin( version_ );
    strm.addBin( globidx.first ).addBin( globidx.second ).addBin(globidx.third);
    strm.addBin( start.first ).addBin( start.second ).addBin( start.third );
    strm.addBin( dims.first ).addBin( dims.second ).addBin( dims_.third );
    strm.addBin( nrsamples );
    strm.addBin( dfmt );
    char* buf = new char [hdrsz];
    OD::memZero( buf, hdrsz );
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

    const int bytes_left_in_hdr = hdrsz - (int)strm.position();
    OD::memZero( buf, bytes_left_in_hdr );
    strm.addBin( buf, bytes_left_in_hdr );

    delete [] buf;
    return strm.isOK();
}


bool Seis::Blocks::Writer::writeBlock( od_ostream& strm, MemBlock& block,
					SampIdx wrstart, Dimensions wrdims )
{
    const DataBuffer& dbuf = block.dbuf_;
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
    Interval<IdxType> globinlidxrg, globcrlidxrg;
    Interval<int> inlrg, crlrg;
    Interval<double> xrg, yrg;
    Interval<float> zrg;
    scanPositions( cubedata, globinlidxrg, globcrlidxrg,
		    inlrg, crlrg, xrg, yrg );
    zrg.start = zevalpositions_.first()->first().second;
    zrg.stop = zevalpositions_.last()->last().second;

    IOPar iop( sKeyGenSection() );
    iop.set( sKeyFmtVersion(), version_ );
    iop.set( sKeySurveyName(), SI().name() );
    iop.set( sKeyCubeName(), cubename_ );
    survgeom_.putStructure( iop );
    DataCharacteristics::putUserTypeToPar( iop, fprep_ );
    if ( scaler_ )
    {
	// write the scaler needed to reconstruct the org values
	LinScaler* invscaler = scaler_->inverse();
	invscaler->put( iop );
	delete invscaler;
    }
    iop.set( sKeyDimensions(), dims_.inl(), dims_.crl(), dims_.z() );
    iop.set( sKeyGlobInlRg(), globinlidxrg );
    iop.set( sKeyGlobCrlRg(), globcrlidxrg );
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
	const MemBlockColumn& column = *(MemBlockColumn*)columns_.getObj(spos);
	if ( column.nruniquevisits_ < 1 )
	    continue;

	const GlobIdx& globidx = column.globidx_;
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
		const int crl = Block::crl4Idxs( survgeom_, dims_.crl(),
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

WriterFinisher( Writer& wrr )
    : ParallelTask("Seis Blocks Writer finisher")
    , wrr_(wrr)
{
    Pos::IdxPairDataSet::SPos spos;
    while ( wrr_.columns_.next(spos) )
    {
	MemBlockColumn* column = (MemBlockColumn*)wrr_.columns_.getObj( spos );
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
    ObjectSet<MemBlockColumn> towrite_;

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
