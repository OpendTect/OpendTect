/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: arrayndutils.cc,v 1.10 2002-02-22 10:10:19 bert Exp $";

#include <arrayndutils.h>


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

    MathFunction<float>* windowfunc = 0;

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
		float distval = ((float) (position[idx] - halfsz) / halfsz);
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
