/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

-*/

#include "seisblocks.h"
#include "envvars.h"
#include "datainterp.h"
#include "oddirs.h"
#include "scaler.h"
#include "genc.h"
#include "survgeom3d.h"
#include "posidxpairdataset.h"

static const Seis::Blocks::SzType cVersion	= 1;
static const Seis::Blocks::SzType cDefDim	= 64;
Seis::Blocks::SzType Seis::Blocks::IOClass::columnHeaderSize( SzType ver )
						{ return 32; }


Seis::Blocks::IOClass::IOClass()
    : basepath_(GetBaseDataDir(),sSeismicSubDir(),"new_cube")
    , dims_(Block::defDims())
    , version_(cVersion)
    , scaler_(0)
    , fprep_(OD::F32)
    , hgeom_(*new HGeom("",ZDomain::SI()))
    , columns_(*new Pos::IdxPairDataSet(sizeof(Block*),false,false))
    , needreset_(true)
    , datatype_(UnknownData)
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
    File::Path fp( basepath_ );
    fp.setExtension( sInfoFileExtension(), false );
    return fp.fullPath();
}


BufferString Seis::Blocks::IOClass::dataFileName() const
{
    File::Path fp( basepath_ );
    fp.setExtension( sKeyDataFileExt(), false );
    return fp.fullPath();
}


BufferString Seis::Blocks::IOClass::overviewFileName() const
{
    File::Path fp( basepath_ );
    fp.setExtension( sKeyOvvwFileExt(), false );
    return fp.fullPath();
}


BufferString Seis::Blocks::IOClass::infoFileNameFor( const char* fnm )
{
    File::Path fp( fnm );
    fp.setExtension( sInfoFileExtension(), true );
    return fp.fullPath();
}


BufferString Seis::Blocks::IOClass::dataFileNameFor( const char* fnm )
{
    File::Path fp( fnm );
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
