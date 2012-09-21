/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : August 2007
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "marchingcubeseditor.h"
#include "marchingcubes.h"

#include "arrayndimpl.h"
#include "ranges.h"

#define mX	0
#define mY	1
#define mZ	2
#define mMaxFactor 2147483


MarchingCubesSurfaceEditor::MarchingCubesSurfaceEditor(
	MarchingCubesSurface& surface )
    : surface_( surface )
    , shapeChange( this )
    , kernel_( 0 )
    , changedsurface_( 0 )
    , originalsurface_( 0 )
    , factor_( 0 )
    , prevfactor_( 0 )
    , threshold_( mUdf(float) )
    , xorigin_( 0 )
    , yorigin_( 0 )
    , zorigin_( 0 )		   				   
    , centernormal_( 1, 0, 0 )
{ }


MarchingCubesSurfaceEditor::~MarchingCubesSurfaceEditor()
{
    delete kernel_;
    delete changedsurface_;
    delete originalsurface_;
}

#define mErrRet \
{ \
    delete kernel_; \
    kernel_ = 0; \
    delete changedsurface_; \
    changedsurface_ = 0; \
 \
    delete originalsurface_; \
    originalsurface_ = 0; \
 \
    return false; \
}


bool MarchingCubesSurfaceEditor::setKernel( const Array3D<unsigned char>& arr,
					    int xpos, int ypos, int zpos )
{
    if ( !arr.isOK() )
	mErrRet;

    delete kernel_;
    kernel_ = 0;

    delete changedsurface_;
    changedsurface_ = 0;

    delete originalsurface_;
    originalsurface_ = 0;

    xorigin_ = xpos-1;
    yorigin_ = ypos-1;
    zorigin_ = zpos-1;

    kernel_ = new Array3DImpl<unsigned char>( arr.info().getSize(mX)+2,
	    				      arr.info().getSize(mY)+2, 
	       				      arr.info().getSize(mZ)+2 );

    if ( !kernel_ || !kernel_->isOK() )
	mErrRet;

    memset( kernel_->getData(), 0, 
	    sizeof(unsigned char) * kernel_->info().getTotalSz() );

    for ( int idz=0; idz<arr.info().getSize(mZ); idz++ )
    {
	for ( int idy=0; idy<arr.info().getSize(mY); idy++ )
	{
	    for ( int idx=0; idx<arr.info().getSize(mX); idx++ )
	    {
		kernel_->set( idx+1, idy+1, idz+1, arr.get(idx,idy,idz));
	    }
	}
    }

    changedsurface_ = new Array3DImpl<int>( kernel_->info() );
    if ( !changedsurface_->isOK() )
	mErrRet;

    originalsurface_ = new Array3DImpl<int>( kernel_->info() );
    if ( !originalsurface_->isOK() )
	mErrRet;

    MarchingCubes2Implicit mc2i( surface_, *originalsurface_, 
	    			 xorigin_, yorigin_, zorigin_, false );
    if ( !mc2i.execute() )
	mErrRet;

    Interval<int> xrg(0, kernel_->info().getSize(mX)+1 );
    Interval<int> yrg(0, kernel_->info().getSize(mY)+1 );
    Interval<int> zrg(0, kernel_->info().getSize(mZ)+1 );
    int centval = originalsurface_->get(xrg.center(),yrg.center(),zrg.center());
    int dx = originalsurface_->get(xrg.center()+1,yrg.center(),zrg.center())-
		centval;
    int dy = originalsurface_->get(xrg.center(),yrg.center()+1,zrg.center())-
		centval;
    int dz = originalsurface_->get(xrg.center(),yrg.center(),zrg.center()+1)-
		centval;
    Coord3 direction( dx, dy, dz );
    centernormal_ = direction.normalize();

    threshold_ = mc2i.threshold();
    factor_ = 0;
    return true;
}


const Coord3& MarchingCubesSurfaceEditor::getCenterNormal() const
{
    return centernormal_;
}


bool MarchingCubesSurfaceEditor::setFactor( int nf )
{
    if ( factor_==nf && prevfactor_==nf )
	return true;

    if ( nf>mMaxFactor || nf<-mMaxFactor )
	return false;

    factor_ = nf;
    if ( !execute() )
	return false;

    prevfactor_ = factor_;
    return true;
}
    

bool MarchingCubesSurfaceEditor::affectedVolume(Interval<int>& xrg,
				      Interval<int>& yrg,
				      Interval<int>& zrg) const
{
    if ( !kernel_ )
	return false;

    xrg =  Interval<int>(xorigin_, xorigin_+kernel_->info().getSize(mX)-1 );
    yrg =  Interval<int>(yorigin_, yorigin_+kernel_->info().getSize(mY)-1 );
    zrg =  Interval<int>(zorigin_, zorigin_+kernel_->info().getSize(mZ)-1 );

    return true;
}


od_int64 MarchingCubesSurfaceEditor::nrIterations() const
{
    if ( !kernel_ )
	return 0;

    return ( kernel_->info().getSize(mX) ) * 
	   ( kernel_->info().getSize(mY) ) *
	   ( kernel_->info().getSize(mZ) );
}


bool MarchingCubesSurfaceEditor::doPrepare( int nrthreads )
{
    return kernel_ && changedsurface_ && originalsurface_ &&
	 kernel_->isOK() && changedsurface_->isOK() &&
	 originalsurface_->isOK();
}

#undef mErrRet


bool MarchingCubesSurfaceEditor::doFinish( bool success )
{
    if ( success && originalsurface_ )
    {
	Array3DImpl<float> convarr( originalsurface_->info() );
	ValueSeries<float>* valseries =
	  new ArrayValueSeries<float,int>(changedsurface_->getData(),false,
		    changedsurface_->info().getTotalSz() );

	if ( convarr.setStorage( valseries ) )
	    surface_.setVolumeData( xorigin_, yorigin_, zorigin_,  
		    		    convarr, threshold_ );
    }

    return true;
}


bool MarchingCubesSurfaceEditor::doWork( od_int64 start, od_int64 stop, int )
{
    const int* origptr = originalsurface_->getData()+start;
    unsigned const char* kernelptr = kernel_->getData()+start;

    mPointerOperation( int, changedsurface_->getData()+start,
	    = *origptr + (factor_*(*kernelptr)>>8), stop-start+1,
	    ++; kernelptr++; origptr++; addToNrDone(1) );

    return true;
}


void MarchingCubesSurfaceEditor::reportShapeChange( bool kernelchange ) 
{
    if ( kernel_ )
    {
	if ( kernelchange )
	{
	    delete kernel_;
	    kernel_ = 0;
	}
    }

    shapeChange.trigger();
}
