/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

#include <arrayndutils.h>

ArrayNDIter::ArrayNDIter( const ArrayNDSize& sz_ )
    : sz ( sz_ )
    , position( sz_.getNDim(), 0 )
{ }

bool ArrayNDIter::next()
{
    return inc( position.size() - 1 );
}


int ArrayNDIter::operator[](int dim) const
{
    return position[dim];
}


bool ArrayNDIter::inc( int dim )
{
    position[dim] ++;

    if ( position[dim] >= sz.getSize(dim))
    {
	if ( dim )
	{
	    position[dim] = 0;
	    return inc( dim-1 );
	}
	else
	    return false;
    }

    return true;
}
	    

DefineEnumNames( ArrayNDWindow, WindowType, 1, "Window type")
{ "Box", "Hamming", 0 };


ArrayNDWindow::ArrayNDWindow( const ArrayNDSize& sz_,
				ArrayNDWindow::WindowType type_ )
    : size ( sz_ )
    , type ( type_ )
    , window ( 0 )
{
    buildWindow();
}


ArrayNDWindow::~ArrayNDWindow()
{ delete window; }


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


bool ArrayNDWindow::buildWindow( )
{
    unsigned long totalsz = size.getTotalSz();
    window = new DataBuffer(totalsz, sizeof(float), false);  
    const int bytespersample = sizeof(float);
    const int ndim = size.getNDim();
    ArrayNDIter position( size );

    MathFunction<float>* windowfunc = 0;

    if ( type == ArrayNDWindow::Hamming )
	windowfunc = new ArrayNDWindow::HammingWindow;
    else if ( type == ArrayNDWindow::Box ) 
	windowfunc = new ArrayNDWindow::BoxWindow;
    else
	return false;

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

	*((float*)(window->data+bytespersample*off ))
						=windowfunc->getValue( dist );
	position.next();
    }	

    delete windowfunc;
    return true;
}
