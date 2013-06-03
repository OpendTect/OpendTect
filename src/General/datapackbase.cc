/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "datapackbase.h"
#include "arrayndimpl.h"
#include "flatposdata.h"
#include "cubesampling.h"
#include "interpol2d.h"
#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"
#include "separstr.h"
#include "bufstringset.h"
#include <math.h>


class MapDataPackXYRotater : public ParallelTask
{
public:
    MapDataPackXYRotater( MapDataPack& mdp )
	: mdp_( mdp )
    {
	float anglenorth = fabs( SI().computeAngleXInl() );
	if ( anglenorth > M_PI_2 )
	    anglenorth = ( float )( M_PI - anglenorth );

	const int inlsz = mdp_.arr2d_->info().getSize(0);
	const int crlsz = mdp_.arr2d_->info().getSize(1);
	const float truelength = inlsz*cos(anglenorth) + crlsz*sin(anglenorth);
	const float truewidth = inlsz*sin(anglenorth) + crlsz*cos(anglenorth);
	const int length = mNINT32( truelength );
	const int width = mNINT32( truewidth );
	
	delete mdp_.xyrotarr2d_;
	mdp_.xyrotarr2d_ = new Array2DImpl<float>( length+1, width+1 );
	
	StepInterval<double> tmpirg( mdp_.posdata_.range(true) );
	StepInterval<double> tmpcrg( mdp_.posdata_.range(false) );
	hsamp_.set( StepInterval<int>((int)tmpirg.start,(int)tmpirg.stop,
		    (int)tmpirg.step),
		StepInterval<int>((int)tmpcrg.start,(int)tmpcrg.stop,
		    (int)tmpcrg.step) );
	Coord spt1 = SI().transform(BinID(hsamp_.start.inl,hsamp_.start.crl) );
	Coord spt2 = SI().transform( BinID(hsamp_.start.inl,hsamp_.stop.crl) );
	Coord spt3 = SI().transform( BinID(hsamp_.stop.inl,hsamp_.start.crl) );
	Coord spt4 = SI().transform( BinID(hsamp_.stop.inl,hsamp_.stop.crl) );
	startpt_ = Coord( mMIN( mMIN(spt1.x, spt2.x), mMIN(spt3.x, spt4.x) ),
		mMIN( mMIN(spt1.y, spt2.y), mMIN(spt3.y, spt4.y) ) );
	stoppt_ = Coord( mMAX( mMAX(spt1.x, spt2.x), mMAX(spt3.x, spt4.x) ),
		mMAX( mMAX(spt1.y, spt2.y), mMAX(spt3.y, spt4.y) ) );
	xstep_ = ( float ) (stoppt_.x - startpt_.x)/length;
	ystep_ = ( float ) (stoppt_.y - startpt_.y)/width;
    }
    
    od_int64 nrIterations() const
    { return mdp_.xyrotarr2d_->info().getTotalSz(); }
    
    bool doFinish( bool success )
    {
	mdp_.xyrotposdata_.setRange( true,
		StepInterval<double>(startpt_.x,stoppt_.x,xstep_) );
	mdp_.xyrotposdata_.setRange( false,
		StepInterval<double>(startpt_.y,stoppt_.y,ystep_) );
	
	return success;
    }
    
    bool doWork( od_int64 start, od_int64 stop, int )
    {
	int startpos[2];
	if ( !mdp_.xyrotarr2d_->info().getArrayPos( start, startpos ) )
	    return false;
	
	ArrayNDIter iter( mdp_.xyrotarr2d_->info() );
	iter.setPos( startpos );
	
	BinID toreach00;
	const int nriters = mCast( int, stop-start+1 );
	for ( od_int64 idx=0; idx<nriters && shouldContinue();
		idx++, iter.next(), addToNrDone(1) )
	{
	    const int* curpos = iter.getPos();
	    Coord coord( startpt_.x + curpos[0] * xstep_,
			 startpt_.y + curpos[1] * ystep_ );
	    float val = mUdf(float );
	    BinID approxbid = SI().transform(coord);
	    if ( hsamp_.includes( approxbid ) )
	    {
		Coord approxcoord = SI().transform( approxbid );
		float diffx = ( float ) (( coord.x - approxcoord.x ) / xstep_);
		float diffy = ( float ) (( coord.y - approxcoord.y ) / ystep_);
		toreach00.inl = diffx>=0 ? 0 : -1;
		toreach00.crl = diffy>=0 ? 0 : -1;
		int id0v00 = (approxbid.inl - hsamp_.start.inl)/hsamp_.step.inl
		    + toreach00.inl;
		int id1v00 = (approxbid.crl - hsamp_.start.crl)/hsamp_.step.crl
		    + toreach00.crl;
		float val00 = mdp_.getValAtIdx( id0v00, id1v00 );
		float val01 = mdp_.getValAtIdx( id0v00 , id1v00+1 );
		float val10 = mdp_.getValAtIdx( id0v00+1, id1v00 );
		float val11 = mdp_.getValAtIdx( id0v00+1, id1v00+1 );
		if ( diffx<0 ) diffx = 1 + diffx;
		if ( diffy<0 ) diffy = 1 + diffy;
		val = Interpolate::linearReg2DWithUdf( val00, val01, val10,
						       val11, diffx, diffy );
	    }

	    mdp_.xyrotarr2d_->set( curpos[0], curpos[1], val );
	}

	return true;
    }

protected:

    MapDataPack&	mdp_;
    Coord		startpt_;
    Coord		stoppt_;
    HorSampling		hsamp_;
    float		xstep_;
    float		ystep_;    
};
			    

Coord PointDataPack::coord( int idx ) const
{
    return SI().transform( binID(idx) );
}


FlatDataPack::FlatDataPack( const char* cat, Array2D<float>* arr )
    : DataPack(cat)
    , arr2d_(arr ? arr : new Array2DImpl<float>(1,1))
    , posdata_(*new FlatPosData)
{
    init();
}


FlatDataPack::FlatDataPack( const FlatDataPack& fdp )
    : DataPack( fdp )
    , arr2d_( fdp.arr2d_ ? new Array2DImpl<float>( *fdp.arr2d_ ) : 0 )
    , posdata_( *new FlatPosData(fdp.posdata_) )
{ }


FlatDataPack::FlatDataPack( const char* cat )
    : DataPack(cat)
    , arr2d_(0)
    , posdata_(*new FlatPosData)
{
    // We cannot call init() here: size() does not dispatch virtual here
    // Subclasses with no arr3d_ will have to do a position init 'by hand'
}


void FlatDataPack::init()
{
    if ( !arr2d_ ) return;
    posdata_.setRange( true, StepInterval<double>(0,size(true)-1,1) );
    posdata_.setRange( false, StepInterval<double>(0,size(false)-1,1) );
}


FlatDataPack::~FlatDataPack()
{
    delete arr2d_;
    delete &posdata_;
}


Coord3 FlatDataPack::getCoord( int i0, int i1 ) const
{
    return Coord3( posData().range(true).atIndex(i0),
		   posData().range(false).atIndex(i1), 0 );
}


double FlatDataPack::getAltDim0Value( int ikey, int idim0 ) const
{
    return posdata_.position( true, idim0 );
}


float FlatDataPack::nrKBytes() const
{
    static const float kbfac = ((float)sizeof(float)) / 1024.0;
    return size(true) * kbfac * size(false);
}


void FlatDataPack::dumpInfo( IOPar& iop ) const
{
    DataPack::dumpInfo( iop );
    iop.set( sKey::Type(), "Flat" );
    const int sz0 = size(true); const int sz1 = size(false);
    for ( int idim=0; idim<2; idim++ )
    {
	const bool isdim0 = idim == 0;
	FileMultiString fms( dimName( isdim0 ) ); fms += size( isdim0 );
	iop.set( IOPar::compKey("Dimension",idim), fms );
    }

    const FlatPosData& pd = posData();
    iop.set( "Positions.Dim0", pd.range(true).start, pd.range(true).stop, 
	    		       pd.range(true).step );
    iop.set( "Positions.Dim1", pd.range(false).start, pd.range(false).stop, 
	    		       pd.range(false).step );
    iop.setYN( "Positions.Irregular", pd.isIrregular() );
    if ( sz0 < 1 || sz1 < 1 ) return;

    Coord3 c( getCoord(0,0) );
    iop.set( "Coord(0,0)", c.x, c.y, c.z );
    c = getCoord( sz0-1, sz1-1 );
    iop.set( "Coord(sz0-1,sz1-1)", c.x, c.y, c.z );
}


int FlatDataPack::size( bool dim0 ) const
{
    return data().info().getSize( dim0 ? 0 : 1 );
}


MapDataPack::MapDataPack( const char* cat, const char* nm, Array2D<float>* arr )
    : FlatDataPack(cat,arr)
    , isposcoord_(false)
    , xyrotarr2d_(0)
    , xyrotposdata_(*new FlatPosData)
    , axeslbls_(4,"")
{
    setName( nm );
}


MapDataPack::~MapDataPack()
{
    delete xyrotarr2d_;
    delete &xyrotposdata_;
}


void MapDataPack::getAuxInfo( int idim0, int idim1, IOPar& par ) const
{
    const Coord3 pos = getCoord( idim0, idim1 );
    if ( isposcoord_ )
    {
	const BinID bid = SI().transform( Coord(pos.x,pos.y) );
	par.set( axeslbls_[2], bid.inl );
	par.set( axeslbls_[3], bid.crl );
    }
    else
    {
	const Coord pos2d = SI().transform( BinID(mNINT32(pos.x),mNINT32(pos.y)) );
	par.set( axeslbls_[0], pos2d.x );
	par.set( axeslbls_[1], pos2d.y );
    }
}


float MapDataPack::getValAtIdx( int idx, int idy ) const
{
    const int nrrows = arr2d_->info().getSize(0);
    const int nrcols = arr2d_->info().getSize(1);
    return ( idx>=0 && idy>=0 && idx<nrrows && idy<nrcols )
	     ? arr2d_->get( idx, idy ) : mUdf(float);
}


void MapDataPack::setPosCoord( bool isposcoord )
{
    isposcoord_ = isposcoord;
}


void MapDataPack::setProps( StepInterval<double> inlrg, 
			    StepInterval<double> crlrg,
			    bool isposcoord, BufferStringSet* dimnames )
{
    posdata_.setRange( true, inlrg );
    posdata_.setRange( false, crlrg );
    if ( dimnames )
    {
	for ( int setidx=0; setidx<dimnames->size(); setidx+=2 )
	    setDimNames( dimnames->get(setidx),
			 dimnames->get(setidx+1), !setidx );
    }

    setPosCoord( isposcoord );
}


void MapDataPack::setRange( StepInterval<double> dim0rg,
			    StepInterval<double> dim1rg, bool forxy )
{
    FlatPosData& posdata = forxy ? xyrotposdata_ : posdata_;
    posdata.setRange( true, dim0rg );
    posdata.setRange( false, dim1rg );
}


void MapDataPack::initXYRotArray( TaskRunner* tr )
{
    MapDataPackXYRotater rotator( *this );
    TaskRunner::execute( tr, rotator );
}


Array2D<float>& MapDataPack::data()
{
    Threads::MutexLocker lock( initlock_ );
    if ( isposcoord_ && !xyrotarr2d_ )
	initXYRotArray( 0 );
    
    return isposcoord_ ? *xyrotarr2d_ : *arr2d_;
}


FlatPosData& MapDataPack::posData()
{
    return isposcoord_ ? xyrotposdata_ : posdata_;
}


void MapDataPack::setDimNames( const char* xlbl, const char* ylbl, bool forxy )
{
    if ( forxy )
    {
	axeslbls_[0] = xlbl;
	axeslbls_[1] = ylbl;
    }
    else
    {
	axeslbls_[2] = xlbl;
	axeslbls_[3] = ylbl;
    }    
}


const char* MapDataPack::dimName( bool dim0 ) const
{
    return dim0 ? isposcoord_ ? axeslbls_[0].buf() : axeslbls_[2].buf()
		: isposcoord_ ? axeslbls_[1].buf() : axeslbls_[3].buf();
}


Array3D<float>& VolumeDataPack::data()
{ return *arr3d_; }


const Array3D<float>& VolumeDataPack::data() const
{ return const_cast<VolumeDataPack*>(this)->data(); }


const char* VolumeDataPack::dimName( char dim ) const
{
    if ( !dim ) return "X0";
    if ( dim==1 ) return "X1";

    return "X2";
}


double VolumeDataPack::getPos(char dim,int idx) const
{ return idx; }


void VolumeDataPack::dumpInfo( IOPar& par ) const
{
    DataPack::dumpInfo( par );
    par.set( "Dimensions", size(0), size(1), size(2) );
}


VolumeDataPack::VolumeDataPack( const char* categry,
				Array3D<float>* arr )
    : DataPack( categry )
    , arr3d_(arr ? arr : new Array3DImpl<float>(0,0,0))
{}



VolumeDataPack::VolumeDataPack( const char* cat )
    : DataPack(cat)
    , arr3d_(0)
{}


float VolumeDataPack::nrKBytes() const
{
    static const float kbfac = ((float)sizeof(float)) / 1024.0;
    float ret = size(0) * kbfac;
    return size( 1 ) * ret * size( 2 );
}


VolumeDataPack::~VolumeDataPack()
{ delete arr3d_; }


int VolumeDataPack::size( char dim ) const
{ return arr3d_ ? arr3d_->info().getSize( dim ) : 0; }





CubeDataPack::CubeDataPack( const char* cat, Array3D<float>* arr )
    : VolumeDataPack(cat)
    , cs_(*new CubeSampling(true))
{
    init();
}


CubeDataPack::CubeDataPack( const char* cat )
    : VolumeDataPack(cat)
    , cs_(*new CubeSampling(true))
{
    // We cannot call init() here: size() does not dispatch virtual here
    // Subclasses with no arr3d_ will have to do a position init 'by hand'
}


void CubeDataPack::init()
{
    if ( !arr3d_ ) return;
    cs_.hrg.stop.inl = cs_.hrg.start.inl + (size(0)-1) * cs_.hrg.step.inl;
    cs_.hrg.stop.crl = cs_.hrg.start.crl + (size(1)-1) * cs_.hrg.step.crl;
    cs_.zrg.stop = cs_.zrg.start + (size(2)-1) * cs_.zrg.step;
}


CubeDataPack::~CubeDataPack()
{
    delete arr3d_;
    delete &cs_;
}


Coord3 CubeDataPack::getCoord( int i0, int i1, int i2 ) const
{
    Coord c( SI().transform(cs_.hrg.atIndex(i0,i1)) );
    return Coord3( c.x, c.y, cs_.zAtIndex(i2) );
}


void CubeDataPack::dumpInfo( IOPar& iop ) const
{
    VolumeDataPack::dumpInfo( iop );

    const CubeSampling& cs = sampling();
    iop.set( "Positions.inl", cs.hrg.start.inl, cs.hrg.stop.inl,
	    		      cs.hrg.step.inl );
    iop.set( "Positions.crl", cs.hrg.start.crl, cs.hrg.stop.crl,
	    		      cs.hrg.step.crl );
    iop.set( "Positions.z", cs.zrg.start, cs.zrg.stop, cs.zrg.step );
}
