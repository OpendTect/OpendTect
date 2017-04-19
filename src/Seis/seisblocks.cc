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

static const unsigned short cVersion = 1;
static const unsigned short cDefDim = 80;
Seis::Blocks::IOClass::HdrSzVersionType
Seis::Blocks::IOClass::columnHeaderSize( HdrSzVersionType ver ) { return 128; }


Seis::Blocks::IOClass::IOClass()
    : basepath_(GetBaseDataDir(),sSeismicSubDir())
    , dims_(Block::defDims())
    , version_(cVersion)
    , scaler_(0)
    , fprep_(OD::F32)
    , needreset_(true)
{
}


Seis::Blocks::IOClass::~IOClass()
{
    deepErase( auxiops_ );
    delete scaler_;
}


BufferString Seis::Blocks::IOClass::dataDirName() const
{
    File::Path fp( basepath_ );
    fp.add( filenamebase_ );
    return fp.fullPath();
}


BufferString Seis::Blocks::IOClass::mainFileName() const
{
    File::Path fp( basepath_ );
    fp.add( filenamebase_ );
    fp.setExtension( "cube", false );
    return fp.fullPath();
}


BufferString Seis::Blocks::IOClass::fileNameFor( const GlobIdx& globidx )
{
    BufferString ret;
    ret.add( globidx.inl() ).add( "_" ).add( globidx.crl() ).add( ".bin" );
    return ret;
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


Seis::Blocks::Block::Block( GlobIdx gidx, const SampIdx& strt,
			    const Dimensions& dms )
    : globidx_(gidx)
    , start_(strt)
    , dims_(dms)
    , interp_(0)
{
}


Seis::Blocks::Block::~Block()
{
    delete interp_;
}


// Following functions are not macro-ed because:
// * It's such fundamental stuff, maintenance will be minimal anyway
// * Easy debugging


Seis::Blocks::IdxType Seis::Blocks::Block::globIdx4Inl( const SurvGeom& sg,
						       int inl, SzType inldim )
{
    return IdxType( sg.idx4Inl( inl ) / inldim );
}

Seis::Blocks::IdxType Seis::Blocks::Block::globIdx4Crl( const SurvGeom& sg,
						       int crl, SzType crldim )
{
    return IdxType( sg.idx4Crl( crl ) / crldim );
}

Seis::Blocks::IdxType Seis::Blocks::Block::globIdx4Z( const SurvGeom& sg,
						     float z, SzType zdim )
{
    return IdxType( sg.idx4Z( z ) / zdim );
}


Seis::Blocks::IdxType Seis::Blocks::Block::sampIdx4Inl( const SurvGeom& sg,
						       int inl, SzType inldim )
{
    return IdxType( sg.idx4Inl( inl ) % inldim );
}

Seis::Blocks::IdxType Seis::Blocks::Block::sampIdx4Crl( const SurvGeom& sg,
						       int crl, SzType crldim )
{
    return IdxType( sg.idx4Crl( crl ) % crldim );
}

Seis::Blocks::IdxType Seis::Blocks::Block::sampIdx4Z( const SurvGeom& sg,
						     float z, SzType zdim )
{
    return IdxType( sg.idx4Z( z ) % zdim );
}


Seis::Blocks::IdxType Seis::Blocks::Block::getSampZIdx( float z,
						   const SurvGeom& sg ) const
{
    return sampIdx4Z( sg, z, dims_.z() );
}


Seis::Blocks::SampIdx Seis::Blocks::Block::getSampIdx( const BinID& bid,
						      const SurvGeom& sg ) const
{
    return SampIdx( sampIdx4Inl(sg,bid.inl(),dims_.inl()),
		    sampIdx4Crl(sg,bid.crl(),dims_.crl()),
		    IdxType(0) );
}


Seis::Blocks::SampIdx Seis::Blocks::Block::getSampIdx( const BinID& bid,
						      float z,
						      const SurvGeom& sg ) const
{
    return SampIdx( sampIdx4Inl(sg,bid.inl(),dims_.inl()),
		    sampIdx4Crl(sg,bid.crl(),dims_.crl()),
		    sampIdx4Z(sg,z,dims_.z()) );
}


int Seis::Blocks::Block::inl4Idxs( const SurvGeom& sg, SzType inldim,
				  IdxType globidx, IdxType sampidx )
{
    return sg.inl4Idx( (((int)inldim) * globidx) + sampidx );
}


int Seis::Blocks::Block::crl4Idxs( const SurvGeom& sg, SzType crldim,
				  IdxType globidx, IdxType sampidx )
{
    return sg.crl4Idx( (((int)crldim) * globidx) + sampidx );
}


float Seis::Blocks::Block::z4Idxs( const SurvGeom& sg, SzType zdim,
				  IdxType globidx, IdxType sampidx )
{
    return sg.z4Idx( (((int)zdim) * globidx) + sampidx );
}
