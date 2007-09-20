/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: arrayndutils.cc,v 1.19 2007-09-20 14:58:43 cvskris Exp $";

#include "arrayndutils.h"

#include "windowfunction.h"


DefineEnumNames( ArrayNDWindow, WindowType, 0, "Windowing type")
{ "Box", "Hamming", "Hanning", "Blackman", "Bartlett", "CosTaper",
  "CosTaper5", "CosTaper10", "CosTaper20", 0 };


ArrayNDWindow::ArrayNDWindow( const ArrayNDInfo& sz_, bool rectangular_,
				ArrayNDWindow::WindowType type_,float paramval )
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


bool ArrayNDWindow::setType( ArrayNDWindow::WindowType newt, float )
{
    ArrayNDWindow::WindowType oldt = type;

    type = newt;
    bool res = buildWindow();

    if ( !res ) type = oldt;

    return res;
}


bool ArrayNDWindow::setType( const char* t, float ) 
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
    float variable =0;

    WindowFunction* windowfunc = 0;

    switch ( type )
    {
    case ArrayNDWindow::Box:
	windowfunc = new BoxWindow;
    break;
    case ArrayNDWindow::Hamming:
	windowfunc = new HammingWindow;
    break;
    case ArrayNDWindow::Hanning:
	windowfunc = new HanningWindow;
    break;
    case ArrayNDWindow::Blackman:
	windowfunc = new BlackmanWindow;
    break;
    case ArrayNDWindow::Bartlett:
	windowfunc = new BartlettWindow;
    break;
    case ArrayNDWindow::CosTaper:
	variable = windowfunc->getVariable();
	windowfunc->setVariable( 1-variable );
    case ArrayNDWindow::CosTaper5:
        windowfunc = new CosTaperWindow();
	windowfunc->setVariable( 0.95 );
    break;
    case ArrayNDWindow::CosTaper10:
        windowfunc = new CosTaperWindow();
	windowfunc->setVariable( 0.90 );
    break;
    case ArrayNDWindow::CosTaper20:
        windowfunc = new CosTaperWindow();
	windowfunc->setVariable( 0.80 );
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
