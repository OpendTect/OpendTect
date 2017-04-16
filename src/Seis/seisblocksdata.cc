/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

-*/

#include "seisblocksdata.h"
#include "databuf.h"
#include "scaler.h"
#include "envvars.h"
#include "datainterp.h"
#include "odmemory.h"
#include "survgeom3d.h"

static const unsigned short cDefDim = 80;
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


Seis::Blocks::Block::Block( GlobIdx gidx, SampIdx strt, Dimensions dms,
			    OD::FPDataRepType fpr )
    : globidx_(gidx)
    , start_(strt)
    , dims_(dms)
    , scaler_(0)
    , dbuf_(*new DataBuffer(0))
{
    const DataCharacteristics dc( fpr );
    interp_ = DataInterpreter<float>::create( dc, true );
    const int bytesperval = (int)dc.nrBytes();
    dbuf_.reByte( bytesperval, false );
    const int totsz = (((int)dims_.inl())*dims_.crl()) * dims_.z();
    dbuf_.reSize( totsz );
}


Seis::Blocks::Block::~Block()
{
    delete interp_;
    delete scaler_;
    delete &dbuf_;
}


void Seis::Blocks::Block::zero()
{
    dbuf_.zero();
}


void Seis::Blocks::Block::retire()
{
    dbuf_.reSize( 0, false );
}


bool Seis::Blocks::Block::isRetired() const
{
    return dbuf_.isEmpty();
}


int Seis::Blocks::Block::getBufIdx( const SampIdx& sidx ) const
{
    const int nrsampsoninl = ((int)sidx.crl()) * dims_.z() + sidx.z();
    return sidx.inl() ? sidx.inl()*nrSampsPerInl() + nrsampsoninl
		      : nrsampsoninl;
}


float Seis::Blocks::Block::value( const SampIdx& sidx ) const
{
    float val = interp_->get( dbuf_.data(), getBufIdx(sidx) );
    return !scaler_ ? val : (float)scaler_->scale( val );
}


void Seis::Blocks::Block::getVert( SampIdx sampidx, float* vals,
				  int arrsz ) const
{
    const int startbufidx = getBufIdx( sampidx );
    sampidx.z() = (IdxType)dims_.z();
    const int stopbufidx = getBufIdx( sampidx );

    if ( interp_->isF32() && !scaler_ )
    {
	int nr2copy = stopbufidx - startbufidx + 1;
	if ( nr2copy > arrsz )
	    nr2copy = arrsz;
	OD::sysMemCopy( vals, dbuf_.data() + sizeof(float) * startbufidx,
		        nr2copy * sizeof(float) );
    }
    else
    {
	int arridx = 0;
	for ( int bufidx=startbufidx; bufidx<=stopbufidx; bufidx++ )
	{
	    if ( arridx >= arrsz )
		break;
	    if ( !scaler_ )
		vals[arridx] = interp_->get( dbuf_.data(), bufidx );
	    else
		vals[arridx] = (float)scaler_->scale(
				    interp_->get( dbuf_.data(), bufidx ) );
	    arridx++;
	}
    }
}


void Seis::Blocks::Block::setValue( const SampIdx& sidx, float val )
{
    if ( !scaler_ )
	interp_->put( dbuf_.data(), getBufIdx(sidx), val );
    else
	interp_->put( dbuf_.data(), getBufIdx(sidx),
			    (float)scaler_->unScale(val) );
}


void Seis::Blocks::Block::setVert( SampIdx sampidx, const float* vals,
				  int arrsz )
{
    const int startbufidx = getBufIdx( sampidx );
    sampidx.z() = (IdxType)dims_.z();
    const int stopbufidx = getBufIdx( sampidx );

    if ( interp_->isF32() && !scaler_ )
    {
	int nr2copy = stopbufidx - startbufidx + 1;
	if ( nr2copy > arrsz )
	    nr2copy = arrsz;
	OD::sysMemCopy( dbuf_.data() + sizeof(float) * startbufidx,
		        vals, nr2copy * sizeof(float) );
    }
    else
    {
	int arridx = 0;
	for ( int bufidx=startbufidx; bufidx<=stopbufidx; bufidx++ )
	{
	    if ( arridx >= arrsz )
		break;
	    if ( !scaler_ )
		interp_->put( dbuf_.data(), bufidx, vals[arridx] );
	    else
		interp_->put( dbuf_.data(), bufidx,
			      (float)scaler_->unScale( vals[arridx] ) );
	    arridx++;
	}
    }
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
