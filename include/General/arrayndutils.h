#ifndef arrayndutils_h
#define arrayndutils_h

/*@+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: arrayndutils.h,v 1.1 2000-01-13 15:16:54 bert Exp $
________________________________________________________________________


@$*/
#include <arraynd.h>
#include <enums.h>
#include <databuf.h>
#include <arrayndimpl.h>
#include <mathfunc.h>
#include <math.h>

/*
removeBias( ) - removes the DC component from an ArrayND. If no output is
		given, removeBias( ) will store the result in the input
		ArrayND.
*/


template <class T>
inline bool removeBias( ArrayND<T>* in, ArrayND<T>* out_ = 0)
{
    ArrayND<T>* out = out_ ? out_ : in; 

    T avg = 0;

    if ( out_ && in->size() != out_->size() ) return false;

    const int sz = in->size().getTotalSz();

    T* inpptr = in->getData();

    for ( int idx=0; idx<sz; idx++ )
	avg += inpptr[idx]; 

    avg /= sz;

    T* outptr = out->getData();

    for ( int idx=0; idx<sz; idx++ )
	outptr[idx] = inpptr[idx] - avg;

    out->dataUpdated();

    return true;
}

/* ArrayNDIter is an object that is able to iterate through all samples in a 
   ArrayND. It will stand on the first position when initiated, and move to
   the second at the fist call to next(). next() will return false when
   no more positions are avaliable
*/

class ArrayNDIter
{
public:
				ArrayNDIter( const ArrayNDSize& );

    bool			next();

    const TypeSet<int>&		getPos() const { return position; }
    int				operator[](int) const;

protected:
    bool			inc(int);

    TypeSet<int>		position;
    const ArrayNDSize&		sz;
};

/* ArrayNDWindow will taper the N-dimentional ArrayND with a windowFunction.
   Usage is straightforwar- construct and use. If apply()'s second argument is
   omitted, the result will be placed in the input array. apply() will return
   false if input-, output- and window-size are not equal.
   The only requirement on the windowfunction is that it should give full taper
   at x=+-1 and no taper when x=0. Feel free to implement more functions!!
*/

class ArrayNDWindow
{
public:
    enum WindowType	{ Box, Hamming };
			DeclareEnumUtils(WindowType);

			ArrayNDWindow( const ArrayNDSize&,
					ArrayNDWindow::WindowType = Box );

			~ArrayNDWindow();

    bool		setType( ArrayNDWindow::WindowType );
    bool		setType( const char* );

    bool		resize( const ArrayNDSize& );

    template <class Type> bool	apply(  ArrayND<Type>* in,
					ArrayND<Type>* out=0) const;

protected:

    DataBuffer*			window;
    ArrayNDSizeImpl		size;
    WindowType			type;

    bool			buildWindow( );

    class BoxWindow : public MathFunction
    {
    public:
	float	getValue( double x ) const
		{ return fabs(x) > 1 ? 0 : 1; }
    };

    class HammingWindow : public MathFunction
    {
    public:
	float	getValue( double x ) const
		{
		    double rx = fabs( x );
		    if ( rx > 1 )
			return 0;

		    return 0.54 + 0.46 * cos( M_PI * rx );
		}
    };
};
   

template <class Type>
inline bool ArrayNDWindow::apply( ArrayND<Type>* in, ArrayND<Type>* out_) const
{
    ArrayND<Type>* out = out_ ? out_ : in; 

    if ( out_ && in->size() != out_->size() ) return false;

    if ( in->size() != size) return false;

    unsigned long totalSz = size.getTotalSz();

    Type* inData = in->getData();
    Type* outData = out->getData();

    int bytesPerSample = window->bytesPerSample();

    for(unsigned long idx = 0; idx < totalSz; idx++)
        outData[idx] = inData[idx] *
                *((float*)(window->data+bytesPerSample*idx ));

    out->dataUpdated();

    return true;
}
 
#endif
