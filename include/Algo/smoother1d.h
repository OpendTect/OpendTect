#ifndef smoother1d_h
#define smoother1d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		May 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "task.h"
#include "valseries.h"
#include "genericnumer.h"
#include "sets.h"
#include "iopar.h"
#include "bufstring.h"
#include "windowfunction.h"

/*!Smothes a 1d signal with an an operator. */


template <class T>
class Smoother1D : public ParallelTask
{
public:

    				Smoother1D();
    				Smoother1D(const Smoother1D&);

    bool			operator==(const Smoother1D<T>&) const;

    void			setInput(const T*,int sz);
    void			setOutput(T*);
    				/*!Must be at least the size of input.*/
    bool			setWindow(const char* nm,float param,
	    				  int lenght );
    int				getWindowLength() const {return window_.size();}
    const char*			getWindowName() const{return windowname_.buf();}
    float			getWindowParam() const{return windowparam_;}

    inline void			fillPar(IOPar&) const;
    inline bool			usePar(const IOPar&);

protected:

    static const char*		sKeyWinFunc() { return "Window function"; }
    static const char*		sKeyWinParam() { return "Window parameter"; }
    static const char*		sKeyWinLen() { return "Window length"; }

    inline od_int64		nrIterations() const	{ return size_; }
    inline bool			doPrepare(int);
    inline bool			doWork(od_int64 start,od_int64 stop,int);

    TypeSet<T>			window_;
    BufferString		windowname_;
    float			windowparam_;

    const T*			input_;
    T*				output_;
    od_int64			size_;

    int				firstdefined_;
    int				lastdefined_;
};


template <class T> inline
Smoother1D<T>::Smoother1D()
    : input_( 0 )
    , output_( 0 )
    , windowparam_( mUdf(float) )
    , firstdefined_( -1 )
    , lastdefined_( -1 )
    , size_( -1 )
{}


template <class T> inline
Smoother1D<T>::Smoother1D( const Smoother1D<T>& b )
    : input_( 0 )
    , output_( 0 )
    , windowparam_( b.windowparam_ )
    , window_( b.window_ )
    , windowname_( b.windowname_ )
    , firstdefined_( -1 )
    , lastdefined_( -1 )
    , size_( -1 )
{ }


template <class T> inline
void Smoother1D<T>::setInput( const T* ni , int sz )
{
    input_ = ni;
    size_ = sz;
}


template <class T> inline
void Smoother1D<T>::setOutput( T* ni )
{
    output_ = ni;
}


template <class T> inline
bool Smoother1D<T>::operator==( const Smoother1D<T>& b ) const
{
    return window_.size()==b.window_.size() &&
	   windowname_==b.windowname_ &&
	   mIsEqual(windowparam_, b.windowparam_, 1e-3 );
}


template <class T> inline
bool Smoother1D<T>::setWindow( const char* nm, float param, int length )
{
    PtrMan<WindowFunction> wf = WinFuncs().create( nm );
    if ( !wf )
	return false;

    if ( wf->hasVariable() && !wf->setVariable( param ) )
	return false;

    if ( length<0 )
	return false;

    window_.setSize( length );
    const double step = 2.0/(length-1);
    for ( int idx=0; idx<length; idx++ )
	window_[idx] = wf->getValue( step*idx-1 );

    windowname_ = nm;
    windowparam_ = wf->hasVariable() ? param : 1e30;

    return true;
}


template <class T> inline
void Smoother1D<T>::fillPar( IOPar& par ) const
{
    par.set( sKeyWinFunc(), windowname_ );
    if ( !mIsUdf(windowparam_) )
	par.set( sKeyWinParam(), windowparam_ );
    par.set( sKeyWinLen(), window_.size() );
}


template <class T> inline
bool Smoother1D<T>::usePar( const IOPar& par )
{
    int sz;
    if ( !par.get(sKeyWinLen(), sz ) )
	return false;

    const char* wn = par.find( sKeyWinFunc() );
    float var = mUdf(float);
    par.get( sKeyWinParam(), var );

    return setWindow( wn, var, sz );
}


template <class T> inline
bool Smoother1D<T>::doPrepare(int)
{
    if ( !input_ || !output_ || !window_.size() )
	return false;

    firstdefined_ = -1;
    lastdefined_ = -1;
    for ( int idx=0; idx<size_; idx++ )
    {
	if ( !mIsUdf(input_[idx]) )
	{
	    firstdefined_=idx;
	    break;
	}
    }

    if ( firstdefined_!=-1 )
    {
	for ( int idx=size_-1; idx>=0; idx-- )
	{
	    if ( !mIsUdf(input_[idx]) )
	    {
		lastdefined_=idx;
		break;
	    }
	}
    }

    return true;
}


template <class T> inline
bool Smoother1D<T>::doWork(od_int64 start,od_int64 stop,int)
{
    const float* window = window_.arr();
    const int windowsize = window_.size();
    const int hwinsize = windowsize/2;

    for ( int outidx=start; outidx<=stop; outidx++, addToNrDone(1) )
    {
	if ( firstdefined_==-1 || outidx<firstdefined_ || outidx>lastdefined_ )
	{
	    output_[outidx] = mUdf(T);
	    continue;
	}

	int sumstart = outidx-hwinsize;
	int sumstop = outidx+windowsize-hwinsize-1;
	int winstart = 0;
	if ( sumstart<0 )
	{
	    winstart = -sumstart;
	    sumstart = 0;
	}

	if ( sumstop>=size_ )
	    sumstop = size_-1;

	double sum = 0;
	double weightsum = 0;
	for ( int sumidx=sumstart, winidx=winstart;
	      sumidx<=sumstop; sumidx++, winidx++ )
	{
	    double val = input_[sumidx];
	    if ( mIsUdf(val) )
		continue;

	    sum += val * window[winidx];
	    weightsum += window[winidx];
	}

	output_[outidx] = weightsum ? sum/weightsum : mUdf(float);
    }

    return true;
}

#endif
