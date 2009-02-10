/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2007
-*/

static const char* rcsID = "$Id: datapackbase.cc,v 1.1 2009-02-10 15:22:13 cvsbert Exp $";

#include "datapackbase.h"
#include "arrayndimpl.h"
#include "flatposdata.h"
#include "cubesampling.h"
#include "interpol2d.h"
#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"
#include "separstr.h"


Coord PointDataPack::coord( int idx ) const
{
    return SI().transform( binID(idx) );
}


FlatDataPack::FlatDataPack( const char* cat, Array2D<float>* arr )
    : DataPack(cat)
    , arr2d_(arr ? arr : new Array2DImpl<float>(0,0))
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
    return Coord3( posdata_.range(true).atIndex(i0),
		   posdata_.range(false).atIndex(i1), 0 );
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
    iop.set( sKey::Type, "Flat" );
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
    , isposcoord_(true)
    , xyrotarr2d_(0)
    , mainisset1_(true)
    , fakeposdata_(*new FlatPosData)
    , axeslbls_(4,"")
{
    setName( nm );
}


MapDataPack::~MapDataPack()
{
    delete xyrotarr2d_;
    delete &fakeposdata_;
}


Coord MapDataPack::get2DCoord( int i0, int i1 ) const
{
    const float x = posdata_.range(true).atIndex(i0);
    const float y = posdata_.range(false).atIndex(i1);
    if ( isposcoord_ ) return Coord( x, y );

    const BinID bid( mNINT(x), mNINT(y) );
    return SI().transform( bid );
}


void MapDataPack::getAuxInfo( int idim0, int idim1, IOPar& par ) const
{
    Coord pos = get2DCoord( idim0, idim1 );
    par.set( "X", pos.x );
    par.set( "Y", pos.y );
}


float MapDataPack::getValAtIdx( int idx, int idy ) const
{
    const int nrrows = arr2d_->info().getSize(0);
    const int nrcols = arr2d_->info().getSize(1);
    return ( idx>=0 && idy>=0 && idx<nrrows && idy<nrcols )
	     ? arr2d_->get( idx, idy ) : mUdf(float);
}


void MapDataPack::createXYRotArray()
{
    isposcoord_ = false; //tmp disabled to avoid endless loop with posData()
    float anglenorth = SI().computeAngleXInl();
    int inlsz = arr2d_->info().getSize(0);
    int crlsz = arr2d_->info().getSize(1);
    float truelength = inlsz * cos(anglenorth) + crlsz * sin(anglenorth);
    float truewidth = inlsz * sin(anglenorth) + crlsz * cos(anglenorth);
    int length = mNINT( truelength );
    int width = mNINT( truewidth );
    HorSampling hsamp;
    StepInterval<double> tmpirg ( posData().range(true) );
    StepInterval<double> tmpcrg ( posData().range(false) );
    hsamp.set( StepInterval<int>((int)tmpirg.start,(int)tmpirg.stop,
				 (int)tmpirg.step),
	       StepInterval<int>((int)tmpcrg.start,(int)tmpcrg.stop,
		   		 (int)tmpcrg.step) );
    Coord spt1 = SI().transform( BinID(hsamp.start.inl,hsamp.start.crl) );
    Coord spt2 = SI().transform( BinID(hsamp.start.inl,hsamp.stop.crl) );
    Coord spt3 = SI().transform( BinID(hsamp.stop.inl,hsamp.start.crl) );
    Coord spt4 = SI().transform( BinID(hsamp.stop.inl,hsamp.stop.crl) );
    Coord startpt( mMIN( mMIN( spt1.x, spt2.x ), mMIN( spt3.x, spt4.x ) ),
	   	   mMIN( mMIN( spt1.y, spt2.y ), mMIN( spt3.y, spt4.y ) ) );
    Coord stoppt( mMAX( mMAX( spt1.x, spt2.x ), mMAX( spt3.x, spt4.x ) ),
	   	  mMAX( mMAX( spt1.y, spt2.y ), mMAX( spt3.y, spt4.y ) ) );
    float xstep = (stoppt.x - startpt.x)/length;
    float ystep = (stoppt.y - startpt.y)/width;
    
    delete xyrotarr2d_;
    xyrotarr2d_ = new Array2DImpl<float>( length+1, width+1 );
    BinID toreach00;
    for ( int idx=0; idx<=length; idx++ )
    {
	for ( int idy=0; idy<=width; idy++ )
	{
	    Coord coord( startpt.x + idx*xstep, startpt.y + idy*ystep );
	    float val = mUdf(float );
	    BinID approxbid = SI().transform(coord);
	    if ( hsamp.includes( approxbid ) )
	    {
		Coord approxcoord = SI().transform( approxbid );
		float diffx = ( coord.x - approxcoord.x ) / xstep;
		float diffy = ( coord.y - approxcoord.y ) / ystep;
		toreach00.inl = diffx>=0 ? 0 : -1;
		toreach00.crl = diffy>=0 ? 0 : -1;
		int id0v00 = (approxbid.inl - hsamp.start.inl)/hsamp.step.inl
		    	     + toreach00.inl;
		int id1v00 = (approxbid.crl - hsamp.start.crl)/hsamp.step.crl
		    	     + toreach00.crl;
		float val00 = getValAtIdx( id0v00, id1v00 );
		float val01 = getValAtIdx( id0v00 , id1v00+1 );
		float val10 = getValAtIdx( id0v00+1, id1v00 );
		float val11 = getValAtIdx( id0v00+1, id1v00+1 );
		if ( diffx<0 ) diffx = 1 + diffx;
		if ( diffy<0 ) diffy = 1 + diffy;
		val = Interpolate::linearReg2DWithUdf( val00, val01, val10,
						       val11, diffx, diffy );
	    }
	    xyrotarr2d_->set( idx, idy, val );
	}
    }
    fakeposdata_.setRange( true, 
	    		   StepInterval<double>( startpt.x, stoppt.x, xstep ) );
    fakeposdata_.setRange( false,
	    		   StepInterval<double>( startpt.y, stoppt.y, ystep ) );
    isposcoord_ = true; //we now have both inl/crl and X/Y positioning
}


void MapDataPack::setPropsAndInit( StepInterval<double> dim0rg,
				   StepInterval<double> dim1rg,
				   bool isposcoord, BufferStringSet* dimnames )
{
    setPosCoord( isposcoord );
    posData().setRange( true, dim0rg );
    posData().setRange( false, dim1rg );
    if ( dimnames )
    {
	for ( int setidx=0; setidx<dimnames->size(); setidx+=2 )
	    setDimNames( dimnames->get(setidx),
			 dimnames->get(setidx+1), !setidx );
    }

    if ( !isposcoord && mainisset1_ )
	setPosCoord( true );
}


Array2D<float>& MapDataPack::data()
{
    if ( mainisset1_ && !xyrotarr2d_ )
	createXYRotArray();

    return mainisset1_ && xyrotarr2d_ ? *xyrotarr2d_ : *arr2d_;
}


FlatPosData& MapDataPack::posData()
{
    if ( isposcoord_ && !xyrotarr2d_ )
	createXYRotArray();

    return mainisset1_ && xyrotarr2d_ ? fakeposdata_ : posdata_;
}


void MapDataPack::setDimNames( const char* x1lbl, const char* x2lbl, bool set1 )
{
    if ( set1 )
    {
	axeslbls_[0] = x1lbl;
	axeslbls_[1] = x2lbl;
    }
    else
    {
	axeslbls_[2] = x1lbl;
	axeslbls_[3] = x2lbl;
    }    
}


const char* MapDataPack::dimName( bool dim0 ) const
{
    return dim0 ? mainisset1_ ? axeslbls_[0].buf() : axeslbls_[1].buf()
		: mainisset1_ ? axeslbls_[2].buf() : axeslbls_[3].buf();
}


CubeDataPack::CubeDataPack( const char* cat, Array3D<float>* arr )
    : DataPack(cat)
    , arr3d_(arr ? arr : new Array3DImpl<float>(0,0,0))
    , cs_(*new CubeSampling(true))
{
    init();
}


CubeDataPack::CubeDataPack( const char* cat )
    : DataPack(cat)
    , arr3d_(0)
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


float CubeDataPack::nrKBytes() const
{
    static const float kbfac = ((float)sizeof(float)) / 1024.0;
    float ret = size(0) * kbfac;
    return size( 1 ) * ret * size( 2 );
}


void CubeDataPack::dumpInfo( IOPar& iop ) const
{
    DataPack::dumpInfo( iop );
    iop.set( sKey::Type, "Cube" );
    iop.set( "Dimensions", size(0), size(1), size(2) );

    const CubeSampling& cs = sampling();
    iop.set( "Positions.inl", cs.hrg.start.inl, cs.hrg.stop.inl,
	    		      cs.hrg.step.inl );
    iop.set( "Positions.crl", cs.hrg.start.crl, cs.hrg.stop.crl,
	    		      cs.hrg.step.crl );
    iop.set( "Positions.z", cs.zrg.start, cs.zrg.stop, cs.zrg.step );
}


int CubeDataPack::size( int dim ) const
{
    return data().info().getSize( dim );
}

