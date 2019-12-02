/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

-*/

#include "seisblocksaccess.h"
#include "seismemblocks.h"
#include "cubedata.h"
#include "datainterp.h"
#include "envvars.h"
#include "hdf5access.h"
#include "genc.h"
#include "oddirs.h"
#include "posidxpairdataset.h"
#include "scaler.h"
#include "survgeom3d.h"
#include "survinfo.h"

static const Seis::Blocks::size_type cVersion	= 1;
static const Seis::Blocks::size_type cDefDim	= 64;


Seis::Blocks::Access::Access()
    : basepath_(GetBaseDataDir(),sSeismicSubDir(),"new_cube")
    , dims_(Block::defDims())
    , version_(cVersion)
    , scaler_(0)
    , datarep_(OD::F32)
    , hgeom_(*new HGeom(""))
    , columns_(*new Pos::IdxPairDataSet(sizeof(Block*),false,false))
    , needreset_(true)
    , datatype_(UnknownData)
    , gensectioniop_(sKeyGenSection())
    , fileidsectioniop_(sKeyFileIDSection())
    , cubedata_(*new PosInfo::CubeData)
    , usehdf_(HDF5::isEnabled(HDF5::sSeismicsType()))
    , zdomain_(SI().zDomain())
{
}


Seis::Blocks::Access::~Access()
{
    deepErase( auxiops_ );
    delete scaler_;
    clearColumns();
    delete &columns_;
    delete &hgeom_;
    delete &cubedata_;
}


bool Seis::Blocks::Access::hdf5Active()
{
    return HDF5::isEnabled( HDF5::sSeismicsType() );
}


const char* Seis::Blocks::Access::fileExtension() const
{
    return sDataFileExt( usehdf_ );
}


const char* Seis::Blocks::Access::sDataFileExt( bool forhdf5 )
{
    return forhdf5 ? HDF5::sFileExtension() : "blocks";
}


BufferString Seis::Blocks::Access::infoFileName() const
{
    File::Path fp( basepath_ );
    fp.setExtension( sInfoFileExtension(), false );
    return fp.fullPath();
}


BufferString Seis::Blocks::Access::dataFileName() const
{
    File::Path fp( basepath_ );
    fp.setExtension( fileExtension(), false );
    return fp.fullPath();
}


BufferString Seis::Blocks::Access::overviewFileName() const
{
    File::Path fp( basepath_ );
    fp.setExtension( sKeyOvvwFileExt(), false );
    return fp.fullPath();
}


BufferString Seis::Blocks::Access::infoFileNameFor( const char* fnm )
{
    File::Path fp( fnm );
    fp.setExtension( sInfoFileExtension(), true );
    return fp.fullPath();
}


BufferString Seis::Blocks::Access::dataFileNameFor( const char* fnm,
						    bool usehdf )
{
    File::Path fp( fnm );
    fp.setExtension( sDataFileExt(usehdf), false );
    return fp.fullPath();
}


void Seis::Blocks::Access::clearColumns()
{
    Pos::IdxPairDataSet::SPos spos;
    while ( columns_.next(spos) )
	delete (Column*)columns_.getObj( spos );
    columns_.setEmpty();
}


Seis::Blocks::Column* Seis::Blocks::Access::findColumn(
						const HGlobIdx& gidx ) const
{
    const Pos::IdxPair idxpair( gidx.inl(), gidx.crl() );
    Pos::IdxPairDataSet::SPos spos = columns_.find( idxpair );
    return spos.isValid() ? (Column*)columns_.getObj( spos ) : 0;
}


void Seis::Blocks::Access::addColumn( Column* column ) const
{
    if ( !column )
	return;

    const Pos::IdxPair idxpair( column->globIdx().inl(),
				column->globIdx().crl() );
    columns_.add( idxpair, column );
}


static PtrMan<Seis::Blocks::Dimensions> def_dims_ = 0;

static Seis::Blocks::size_type getNextDimSz( char*& startptr )
{
    int val;
    char* ptr = firstOcc( startptr, 'x' );
    if ( ptr )
	*ptr = '\0';
    val = toInt( startptr );
    if ( ptr )
	startptr = ptr + 1;
    if ( val < 0 || val > 65535 )
	return 0;
    return (Seis::Blocks::size_type)val;
}


Seis::Blocks::Dimensions Seis::Blocks::Block::defDims()
{
    if ( !def_dims_ )
    {
	Dimensions* dims = new Dimensions( cDefDim, cDefDim, cDefDim );
	BufferString envvval = GetEnvVar( "OD_SEIS_BLOCKS_DIMS" );
	if ( !envvval.isEmpty() )
	{
	    char* startptr = envvval.getCStr();
	    dims->inl() = getNextDimSz( startptr );
	    dims->crl() = getNextDimSz( startptr );
	    dims->z() = getNextDimSz( startptr );
	}
	def_dims_.setIfNull( dims, true );
    }
    return *def_dims_;
}


// Following functions are not macro-ed because:
// * It's such fundamental stuff, maintenance will be minimal anyway
// * Easy debugging


Seis::Blocks::idx_type Seis::Blocks::Block::globIdx4Inl( const HGeom& hg,
					       int inl, size_type inldim )
{
    return idx_type( hg.idx4Inl( inl ) / inldim );
}

Seis::Blocks::idx_type Seis::Blocks::Block::globIdx4Crl( const HGeom& hg,
					       int crl, size_type crldim )
{
    return idx_type( hg.idx4Crl( crl ) / crldim );
}

Seis::Blocks::idx_type Seis::Blocks::Block::globIdx4Z( const ZGeom& zg,
					     float z, size_type zdim )
{
    return idx_type( zg.nearestIndex( z ) / zdim );
}


Seis::Blocks::idx_type Seis::Blocks::Block::locIdx4Inl( const HGeom& hg,
					       int inl, size_type inldim )
{
    return idx_type( hg.idx4Inl( inl ) % inldim );
}

Seis::Blocks::idx_type Seis::Blocks::Block::locIdx4Crl( const HGeom& hg,
					       int crl, size_type crldim )
{
    return idx_type( hg.idx4Crl( crl ) % crldim );
}

Seis::Blocks::idx_type Seis::Blocks::Block::locIdx4Z( const ZGeom& zg,
					     float z, size_type zdim )
{
    return idx_type( zg.nearestIndex( z ) % zdim );
}


int Seis::Blocks::Block::startInl4GlobIdx( const HGeom& hg,
					   idx_type gidx, size_type inldim )
{
    return inl4Idxs( hg, inldim, gidx, 0 );
}

int Seis::Blocks::Block::startCrl4GlobIdx( const HGeom& hg,
					   idx_type gidx, size_type crldim )
{
    return crl4Idxs( hg, crldim, gidx, 0 );
}

float Seis::Blocks::Block::startZ4GlobIdx( const ZGeom& zg,
					   idx_type gidx, size_type zdim )
{
    return z4Idxs( zg, zdim, gidx, 0 );
}


int Seis::Blocks::Block::inl4Idxs( const HGeom& hg, size_type inldim,
				  idx_type globidx, idx_type sampidx )
{
    return hg.inl4Idx( (((int)inldim) * globidx) + sampidx );
}


int Seis::Blocks::Block::crl4Idxs( const HGeom& hg, size_type crldim,
				  idx_type globidx, idx_type sampidx )
{
    return hg.crl4Idx( (((int)crldim) * globidx) + sampidx );
}


float Seis::Blocks::Block::z4Idxs( const ZGeom& zg, size_type zdim,
				  idx_type globidx, idx_type sampidx )
{
    return zg.atIndex( (((int)zdim) * globidx) + sampidx );
}


Seis::Blocks::MemBlock::MemBlock( GlobIdx gidx, const Dimensions& dms,
				  const DataInterp& interp )
    : Block(gidx,HLocIdx(),dms)
    , dbuf_(0)
    , interp_(interp)
{
    const int bytesperval = interp_.nrBytes();
    dbuf_.reByte( bytesperval, false );
    const int totsz = (((int)dims_.inl())*dims_.crl()) * dims_.z();
    dbuf_.reSize( totsz, false );
    dbuf_.zero();
}


int Seis::Blocks::MemBlock::getBufIdx( const LocIdx& sidx ) const
{
    return ((int)dims_.z()) * (sidx.inl()*dims_.crl() + sidx.crl()) + sidx.z();
}


float Seis::Blocks::MemBlock::value( const LocIdx& sidx ) const
{
    return interp_.get( dbuf_.data(), getBufIdx(sidx) );
}


void Seis::Blocks::MemBlock::setValue( const LocIdx& sidx, float val )
{
    interp_.put( dbuf_.data(), getBufIdx(sidx), val );
}


void Seis::Blocks::MemBlock::retire( MemColumnSummary* summary,
				     const bool* const* visited )
{
    if ( summary )
	summary->fill( *this, visited );
    dbuf_.reSize( 0, false );
}


Seis::Blocks::MemColumnSummary::~MemColumnSummary()
{
    if ( vals_ )
    {
	for ( int iinl=0; iinl<dims_.inl(); iinl++ )
	    delete vals_[iinl];
	delete [] vals_;
    }
}


void Seis::Blocks::MemColumnSummary::fill( const MemBlock& block,
					  const bool* const* visited )
{
    dims_ = block.dims();
    if ( dims_.inl() == 0 || dims_.crl() == 0 || dims_.z() == 0 )
	return;

    vals_ = new float* [dims_.inl()];
    for ( int iinl=0; iinl<dims_.inl(); iinl++ )
	vals_[iinl] = new float [dims_.crl()];

    LocIdx locidx;
    float* valtrc = new float[dims_.z()];
    for ( locidx.inl()=0; locidx.inl()<dims_.inl(); locidx.inl()++ )
    {
	for ( locidx.crl()=0; locidx.crl()<dims_.inl(); locidx.crl()++ )
	{
	    float val = mUdf(float);
	    if ( visited[locidx.inl()][locidx.crl()] )
	    {
		for ( locidx.z()=0; locidx.z()<dims_.z(); locidx.z()++ )
		    valtrc[locidx.z()] = block.value( locidx );
		val = calcVal( valtrc );
	    }
	    vals_[locidx.inl()][locidx.crl()] = val;
	}
    }
}


float Seis::Blocks::MemColumnSummary::calcVal( float* valtrc ) const
{
    const size_type totns = dims_.z();
    float sumsq = 0.f; int ns = 0;
    for ( int isamp=0; isamp<totns; isamp++ )
    {
	const float val = valtrc[isamp];
	if ( !mIsUdf(val) )
	{
	    ns++;
	    sumsq += val * val;
	}
    }
    return Math::Sqrt( sumsq / ns );
}


Seis::Blocks::MemBlockColumn::MemBlockColumn( const HGlobIdx& gidx,
					      const Dimensions& bldims,
					      int nrcomps )
    : Column(gidx,bldims,nrcomps)
    , nruniquevisits_(0)
    , fileid_(0)
{
    for ( int icomp=0; icomp<nrcomps_; icomp++ )
	blocksets_ += new BlockSet;

    visited_ = new bool* [dims_.inl()];
    for ( idx_type iinl=0; iinl<dims_.inl(); iinl++ )
    {
	visited_[iinl] = new bool [dims_.crl()];
	for ( idx_type icrl=0; icrl<dims_.crl(); icrl++ )
	    visited_[iinl][icrl] = false;
    }
}


Seis::Blocks::MemBlockColumn::~MemBlockColumn()
{
    deepErase(blocksets_);
    for ( idx_type idx=0; idx<dims_.inl(); idx++ )
	delete [] visited_[idx];
    delete [] visited_;
}


void Seis::Blocks::MemBlockColumn::retire()
{
    const int midblockidx = blocksets_.size() / 2;
    for ( int iset=0; iset<blocksets_.size(); iset++ )
    {
	BlockSet& bset = *blocksets_[iset];
	for ( int iblock=0; iblock<bset.size(); iblock++ )
	    bset[iblock]->retire( iset == midblockidx ? &summary_ : 0,
				  visited_ );
    }
}


void Seis::Blocks::MemBlockColumn::getDefArea( HLocIdx& defstart,
					       HDimensions& defdims ) const
{
    idx_type mininl = dims_.inl()-1, mincrl = dims_.crl()-1;
    idx_type maxinl = 0, maxcrl = 0;

    for ( idx_type iinl=0; iinl<dims_.inl(); iinl++ )
    {
	for ( idx_type icrl=0; icrl<dims_.crl(); icrl++ )
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

    defstart.inl() = mininl;
    defstart.crl() = mincrl;
    defdims.inl() = maxinl - mininl + 1;
    defdims.crl() = maxcrl - mincrl + 1;
}

