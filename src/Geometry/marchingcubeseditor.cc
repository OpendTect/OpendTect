/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : August 2007
-*/

static const char* rcsID = "$Id: marchingcubeseditor.cc,v 1.1 2007-09-04 18:02:28 cvskris Exp $";

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
    , implicitsurface_( 0 )
    , originalsurface_( 0 )
    , factor_( 0 )
    , prevfactor_( 0 )
    , threshold_( mUdf(float) )
{ }


MarchingCubesSurfaceEditor::~MarchingCubesSurfaceEditor()
{
    delete kernel_;
    delete implicitsurface_;
    delete originalsurface_;
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
    

void MarchingCubesSurfaceEditor::affectedVolume(Interval<int>& xrg,
				      Interval<int>& yrg,
				      Interval<int>& zrg) const
{
    xrg = kernelXRange(); yrg = kernelYRange(); zrg = kernelZRange();
}


int MarchingCubesSurfaceEditor::nrTimes() const
{
    return (kernelXRange().width()+1) * (kernelYRange().width()+1) *
	   (kernelZRange().width()+1);
}


#define mErrRet \
{ \
    delete kernel_; \
    kernel_ = 0; \
    delete implicitsurface_; \
    implicitsurface_ = 0; \
 \
    delete originalsurface_; \
    originalsurface_ = 0; \
 \
    return false; \
}


bool MarchingCubesSurfaceEditor::doPrepare( int nrthreads )
{
    if ( !kernel_ )
    {	
	kernel_ = new Array3DImpl<unsigned char>( kernelXRange().width()+1,
						  kernelYRange().width()+1,
						  kernelZRange().width()+1 );
	if ( !kernel_->isOK() || !computeKernel( *kernel_ ) )
	    mErrRet;
    }

    if ( !implicitsurface_ )
    {
	implicitsurface_ = new Array3DImpl<int>( kernelXRange().width()+1,
						 kernelYRange().width()+1,
						 kernelZRange().width()+1 );
	if ( !implicitsurface_->isOK() )
	    mErrRet;

	MarchingCubes2Implicit mc2i( surface_, *implicitsurface_,
	    kernelXRange().start, kernelYRange().start, kernelZRange().start );

	if ( !mc2i.compute() )
	    mErrRet;

	threshold_ = mc2i.threshold();

	originalsurface_ = new Array3DImpl<int>( kernelXRange().width()+1,
						 kernelYRange().width()+1,
						 kernelZRange().width()+1 );
	if ( !originalsurface_->isOK() )
	    mErrRet;
    }

    return true;
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
	    surface_.setVolumeData( kernelXRange().start, kernelYRange().start,
				    kernelZRange().start,  convarr, threshold_);
    }

    return true;
}


bool MarchingCubesSurfaceEditor::doWork( int start, int stop, int )
{
    const int difffactor = prevfactor_-factor_;
    if ( !difffactor ) return true;

    int* implicitptr = implicitsurface_->getData();
    const int* stopptr = implicitsurface_->getData()+stop;
    const int* origptr = originalsurface_->getData();
    unsigned const char* kernelptr = kernel_->getData();

    implicitptr += start;
    origptr += start;
    kernelptr += start;

    while ( implicitptr<=stopptr )
    {
	*implicitptr = *origptr + (difffactor*(*kernelptr)>>8);
	kernelptr++;
	implicitptr++;
	origptr++;
    }

    return true;
}


void MarchingCubesSurfaceEditor::reportShapeChange( bool kernelchange ) 
{
    if ( kernel_ )
    {
	if ( kernelchange ||
	     kernelXRange().width()+1!=kernel_->info().getSize(mX) ||
	     kernelYRange().width()+1!=kernel_->info().getSize(mY) ||
	     kernelZRange().width()+1!=kernel_->info().getSize(mZ) )
	{
	    delete kernel_;
	    kernel_ = 0;
	}
    }

    shapeChange.trigger();
}
