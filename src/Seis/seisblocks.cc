/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisblocks.h"
#include "seismemblocks.h"
#include "envvars.h"
#include "datainterp.h"
#include "oddirs.h"
#include "scaler.h"
#include "genc.h"
#include "survinfo.h"
#include "survgeom3d.h"
#include "coordsystem.h"
#include "posidxpairdataset.h"

static const Seis::Blocks::SzType cVersion	= 1;
static const Seis::Blocks::SzType cDefDim	= 64;
Seis::Blocks::SzType Seis::Blocks::IOClass::columnHeaderSize( SzType ver )
						{ return 32; }


Seis::Blocks::HGeom::HGeom( const Survey::Geometry3D& sg )
    : Survey::Geometry3D(sg)
{
}


Seis::Blocks::HGeom::HGeom( const HGeom& oth )
    : Survey::Geometry3D(oth)
{
}


Seis::Blocks::HGeom::~HGeom()
{}


void Seis::Blocks::HGeom::putMapInfo( IOPar& iop ) const
{
    b2c_.fillPar( iop );
    SI().getCoordSystem()->fillPar( iop );
    iop.set( sKey::FirstInl(), sampling_.hsamp_.start_.inl() );
    iop.set( sKey::FirstCrl(), sampling_.hsamp_.start_.crl() );
    iop.set( sKey::StepInl(), sampling_.hsamp_.step_.inl() );
    iop.set( sKey::StepCrl(), sampling_.hsamp_.step_.crl() );
    iop.set( sKey::LastInl(), sampling_.hsamp_.stop_.inl() );
    iop.set( sKey::LastCrl(), sampling_.hsamp_.stop_.crl() );
}


void Seis::Blocks::HGeom::getMapInfo( const IOPar& iop )
{
    b2c_.usePar( iop );
    iop.get( sKey::FirstInl(), sampling_.hsamp_.start_.inl() );
    iop.get( sKey::FirstCrl(), sampling_.hsamp_.start_.crl() );
    iop.get( sKey::StepInl(), sampling_.hsamp_.step_.inl() );
    iop.get( sKey::StepCrl(), sampling_.hsamp_.step_.crl() );
    iop.get( sKey::LastInl(), sampling_.hsamp_.stop_.inl() );
    iop.get( sKey::LastCrl(), sampling_.hsamp_.stop_.crl() );
}


bool Seis::Blocks::HGeom::isCompatibleWith( const Survey::Geometry& geom ) const
{
    return true; // pray!
}


Seis::Blocks::IOClass::IOClass()
    : basepath_(GetBaseDataDir(),sSeismicSubDir(),"new_cube")
    , dims_(Block::defDims())
    , version_(cVersion)
    , scaler_(0)
    , fprep_(DataCharacteristics::F32)
    , hgeom_(*new HGeom(Survey::Geometry3D("",ZDomain::SI())))
    , columns_(*new Pos::IdxPairDataSet(sizeof(Block*),false,false))
    , needreset_(true)
    , datatype_(UnknowData)
{
}


Seis::Blocks::IOClass::~IOClass()
{
    deepErase( auxiops_ );
    delete scaler_;
    clearColumns();
    delete &columns_;
    delete &hgeom_;
}


const ZDomain::Def& Seis::Blocks::IOClass::zDomain() const
{
    return hgeom_.zDomain();
}


BufferString Seis::Blocks::IOClass::infoFileName() const
{
    FilePath fp( basepath_ );
    fp.setExtension( sInfoFileExtension(), false );
    return fp.fullPath();
}


BufferString Seis::Blocks::IOClass::dataFileName() const
{
    FilePath fp( basepath_ );
    fp.setExtension( sKeyDataFileExt(), false );
    return fp.fullPath();
}


BufferString Seis::Blocks::IOClass::overviewFileName() const
{
    FilePath fp( basepath_ );
    fp.setExtension( sKeyOvvwFileExt(), false );
    return fp.fullPath();
}


BufferString Seis::Blocks::IOClass::infoFileNameFor( const char* fnm )
{
    FilePath fp( fnm );
    fp.setExtension( sInfoFileExtension(), true );
    return fp.fullPath();
}


BufferString Seis::Blocks::IOClass::dataFileNameFor( const char* fnm )
{
    FilePath fp( fnm );
    fp.setExtension( sKeyDataFileExt(), false );
    return fp.fullPath();
}


void Seis::Blocks::IOClass::clearColumns()
{
    Pos::IdxPairDataSet::SPos spos;
    while ( columns_.next(spos) )
	delete (Column*)columns_.getObj( spos );
    columns_.setEmpty();
}


Seis::Blocks::Column* Seis::Blocks::IOClass::findColumn(
						const HGlobIdx& gidx ) const
{
    const Pos::IdxPair idxpair( gidx.inl(), gidx.crl() );
    Pos::IdxPairDataSet::SPos spos = columns_.find( idxpair );
    return spos.isValid() ? (Column*)columns_.getObj( spos ) : 0;
}


void Seis::Blocks::IOClass::addColumn( Column* column ) const
{
    if ( !column )
	return;

    const Pos::IdxPair idxpair( column->globIdx().inl(),
				column->globIdx().crl() );
    columns_.add( idxpair, column );
}


static PtrMan<Seis::Blocks::Dimensions> def_dims_ = 0;

static Seis::Blocks::SzType getNextDim( char*& startptr )
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
    return (Seis::Blocks::SzType)val;
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
	    dims->inl() = getNextDim( startptr );
	    dims->crl() = getNextDim( startptr );
	    dims->z() = getNextDim( startptr );
	}
	def_dims_.setIfNull( dims, true );
    }
    return *def_dims_;
}


// Following functions are not macro-ed because:
// * It's such fundamental stuff, maintenance will be minimal anyway
// * Easy debugging


Seis::Blocks::IdxType Seis::Blocks::Block::globIdx4Inl( const HGeom& hg,
						       int inl, SzType inldim )
{
    return IdxType( hg.idx4Inl( inl ) / inldim );
}

Seis::Blocks::IdxType Seis::Blocks::Block::globIdx4Crl( const HGeom& hg,
						       int crl, SzType crldim )
{
    return IdxType( hg.idx4Crl( crl ) / crldim );
}

Seis::Blocks::IdxType Seis::Blocks::Block::globIdx4Z( const ZGeom& zg,
						     float z, SzType zdim )
{
    return IdxType( zg.nearestIndex( z ) / zdim );
}


Seis::Blocks::IdxType Seis::Blocks::Block::locIdx4Inl( const HGeom& hg,
						       int inl, SzType inldim )
{
    return IdxType( hg.idx4Inl( inl ) % inldim );
}

Seis::Blocks::IdxType Seis::Blocks::Block::locIdx4Crl( const HGeom& hg,
						       int crl, SzType crldim )
{
    return IdxType( hg.idx4Crl( crl ) % crldim );
}

Seis::Blocks::IdxType Seis::Blocks::Block::locIdx4Z( const ZGeom& zg,
						     float z, SzType zdim )
{
    return IdxType( zg.nearestIndex( z ) % zdim );
}


Seis::Blocks::Block::Block( const GlobIdx& gidx, const HLocIdx& s,
			    const Dimensions& d )
    : globidx_(gidx)
    , start_(s)
    , dims_(d)
{}


Seis::Blocks::Block::~Block()
{}


int Seis::Blocks::Block::startInl4GlobIdx( const HGeom& hg,
					   IdxType gidx, SzType inldim )
{
    return inl4Idxs( hg, inldim, gidx, 0 );
}

int Seis::Blocks::Block::startCrl4GlobIdx( const HGeom& hg,
					   IdxType gidx, SzType crldim )
{
    return crl4Idxs( hg, crldim, gidx, 0 );
}

float Seis::Blocks::Block::startZ4GlobIdx( const ZGeom& zg,
					   IdxType gidx, SzType zdim )
{
    return z4Idxs( zg, zdim, gidx, 0 );
}


int Seis::Blocks::Block::inl4Idxs( const HGeom& hg, SzType inldim,
				  IdxType globidx, IdxType sampidx )
{
    return hg.inl4Idx( (((int)inldim) * globidx) + sampidx );
}


int Seis::Blocks::Block::crl4Idxs( const HGeom& hg, SzType crldim,
				  IdxType globidx, IdxType sampidx )
{
    return hg.crl4Idx( (((int)crldim) * globidx) + sampidx );
}


float Seis::Blocks::Block::z4Idxs( const ZGeom& zg, SzType zdim,
				  IdxType globidx, IdxType sampidx )
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
    const SzType totns = dims_.z();
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
    , fileoffset_(0)
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

    defstart.inl() = mininl;
    defstart.crl() = mincrl;
    defdims.inl() = maxinl - mininl + 1;
    defdims.crl() = maxcrl - mincrl + 1;
}


Seis::Blocks::Column::Column( const HGlobIdx& gidx, const Dimensions& d,
			      int nc )
    : globidx_(gidx)
    , dims_(d)
    , nrcomps_(nc)
{}


Seis::Blocks::Column::~Column()
{}
