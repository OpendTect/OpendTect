/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: arrayndutils.cc,v 1.14 2005-04-01 10:10:19 cvsbert Exp $";

#include "arrayndutils.h"
#include "cubedata.h"
#include "arrayndimpl.h"


DefineEnumNames( ArrayNDWindow, WindowType, 0, "Windowing type")
{ "Box", "Hamming", "Hanning", "Blackman", "Barlett",
  "CosTaper5", "CosTaper10", "CosTaper20", 0 };


ArrayNDWindow::ArrayNDWindow( const ArrayNDInfo& sz_, bool rectangular_,
				ArrayNDWindow::WindowType type_ )
    : size ( sz_ )
    , type ( type_ )
    , rectangular( rectangular_ )
    , window ( 0 )
{
    buildWindow();
}


ArrayNDWindow::~ArrayNDWindow()
{
    delete [] window;
}


bool ArrayNDWindow::setType( ArrayNDWindow::WindowType newt )
{
    ArrayNDWindow::WindowType oldt = type;

    type = newt;
    bool res = buildWindow();

    if ( !res ) type = oldt;

    return res;
}


bool ArrayNDWindow::setType( const char* t ) 
{
    if (  !strcmp( t, ArrayNDWindow::WindowTypeNames[ArrayNDWindow::Box]) )
	return setType( ArrayNDWindow::Box );
    if (  !strcmp( t, ArrayNDWindow::WindowTypeNames[ArrayNDWindow::Hamming]))
	return setType( ArrayNDWindow::Hamming );

    return false;
}


bool ArrayNDWindow::buildWindow()
{
    unsigned long totalsz = size.getTotalSz();
    window = new float[totalsz];  
    const int ndim = size.getNDim();
    ArrayNDIter position( size );

    FloatMathFunction* windowfunc = 0;

    switch ( type )
    {
    case ArrayNDWindow::Box:
	windowfunc = new ArrayNDWindow::BoxWindow;
    break;
    case ArrayNDWindow::Hamming:
	windowfunc = new ArrayNDWindow::HammingWindow;
    break;
    case ArrayNDWindow::Hanning:
	windowfunc = new ArrayNDWindow::HanningWindow;
    break;
    case ArrayNDWindow::Blackman:
	windowfunc = new ArrayNDWindow::BlackmanWindow;
    break;
    case ArrayNDWindow::Barlett:
	windowfunc = new ArrayNDWindow::BarlettWindow;
    break;
    case ArrayNDWindow::CosTaper5:
	windowfunc = new ArrayNDWindow::CosTaperWindow(5);
    break;
    case ArrayNDWindow::CosTaper10:
	windowfunc = new ArrayNDWindow::CosTaperWindow(10);
    break;
    case ArrayNDWindow::CosTaper20:
	windowfunc = new ArrayNDWindow::CosTaperWindow(20);
    break;
    default:
	return false;
    }

    if ( !rectangular )
    {
	for ( unsigned long off=0; off<totalsz; off++ )
	{
	    float dist = 0;    

	    for ( int idx=0; idx<ndim; idx++ )
	    {
		int sz =  size.getSize(idx);
		int halfsz = sz / 2;
		float distval = (halfsz==0) ? 0 :
		    		( (float) (position[idx] - halfsz) / halfsz );
		dist += distval * distval;
	    }

	    dist = sqrt( dist );

	    window[off] = windowfunc->getValue( dist );
	    position.next();
	}	
    }
    else
    {
	for ( unsigned long off=0; off<totalsz; off++ )
	{
	    float windowval = 1;

	    for ( int idx=0; idx<ndim; idx++ )
	    {
		int sz =  size.getSize(idx);
		int halfsz = sz / 2;
		float distval = ((float) (position[idx] - halfsz) / halfsz);
		windowval *= windowfunc->getValue( distval );
	    }

	    window[off] = windowval;
	    position.next();
	}	
    }
	

    delete windowfunc;
    return true;
}


CubeData::CubeData( const CubeSampling& cs, CubeSampling::Dir d )
    	: dir_(d)
{
#define mMkDim(nr) \
    const CubeSampling::Dir dir##nr = direction( dir_, nr ); \
    int dim##nr = cs.size( dir##nr ); if ( dim##nr < 1 ) dim##nr = 1

    mMkDim( 0 ); mMkDim( 1 ); mMkDim( 2 );
    vals_ = new Array3DImpl<float>( dim0, dim1, dim2 );
}


CubeData::CubeData( Array3D<float>* a3d, CubeSampling::Dir d )
    	: dir_(d)
    	, vals_(a3d)
{
}


CubeData::CubeData( const CubeData& cd )
    	: vals_(0)
{
    *this = cd;
}


CubeData& CubeData::operator =( const CubeData& cd )
{
    if ( this != &cd )
    {
	dir_ = cd.dir_;
	delete vals_;
	vals_ = new Array3DImpl<float>( *cd.vals_ );
    }
    return *this;
}
