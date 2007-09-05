/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : August 2007
-*/

static const char* rcsID = "$Id: marchingcubeseditor.cc,v 1.2 2007-09-05 15:24:13 cvsyuancheng Exp $";

#include "marchingcubeseditor.h"
#include "marchingcubes.h"

#include "arrayndimpl.h"
#include "ranges.h"

#define mX	0
#define mY	1
#define mZ	2


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


bool MarchingCubesSurfaceEditor::setKernel( Array3D<unsigned char>* arr,
					    int xpos, int ypos, int zpos )
{
    if ( !arr || arr->isOK() )
	mErrRet;

    delete kernel_;
    kernel_ = 0;

    delete changedsurface_;
    changedsurface_ = 0;

    delete originalsurface_;
    originalsurface_ = 0;

    xorigin_ = xpos;
    yorigin_ = ypos;
    zorigin_ = zpos;
    kernel_ = arr;

    changedsurface_ = new Array3DImpl<int>( kernel_->info() );
    if ( !changedsurface_->isOK() )
	mErrRet;

    originalsurface_ = new Array3DImpl<int>( kernel_->info() );
    if ( !originalsurface_->isOK() )
	mErrRet;

    MarchingCubes2Implicit mc2i( surface_, *originalsurface_,
	xorigin_, yorigin_, zorigin_ );

    if ( !mc2i.compute() )
	mErrRet;

    threshold_ = mc2i.threshold();

    return true;
}


bool MarchingCubesSurfaceEditor::setFactor( int nf )
{
    if ( factor_==prevfactor_==nf )
	return true;

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


int MarchingCubesSurfaceEditor::nrTimes() const
{
    if ( !kernel_ )
	return 0;

    return ( kernel_->info().getSize(mX) ) * 
	   ( kernel_->info().getSize(mY) ) *
	   ( kernel_->info().getSize(mZ) );
}


bool MarchingCubesSurfaceEditor::doPrepare( int nrthreads )
{
    return kernel_ && changedsurface_ && !originalsurface_ &&
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
	    new ArrayValueSeries<float,int>(originalsurface_->getData(),false);

	if ( convarr.setStorage( valseries ) )
	    surface_.setVolumeData( xorigin_, yorigin_, zorigin_,  
		    		    convarr, threshold_ );
    }

    return true;
}


bool MarchingCubesSurfaceEditor::doWork( int start, int stop, int )
{
    int* changedsurfptr = changedsurface_->getData();
    const int* stopptr = changedsurface_->getData()+stop;
    const int* origptr = originalsurface_->getData();
    unsigned const char* kernelptr = kernel_->getData();

    changedsurfptr += start;
    origptr += start;
    kernelptr += start;

    while ( changedsurfptr<=stopptr )
    {
	*changedsurfptr = *origptr + (factor_*(*kernelptr)>>8);
	kernelptr++;
	changedsurfptr++;
	origptr++;
    }

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
