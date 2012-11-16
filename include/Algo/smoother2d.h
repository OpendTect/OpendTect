#ifndef smoother2d_h
#define smoother2d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Feb 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "task.h"
#include "convolve2d.h"
#include "arrayndimpl.h"
#include "iopar.h"
#include "windowfunction.h"

/*!Smothes a 2d signal with an an operator. */


template <class T>
class Smoother2D : public Task
{
public:

				Smoother2D();
				~Smoother2D();

    void			setInput(const Array2D<T>&,bool hasudf);
    void			setOutput(Array2D<T>&);
   		 		/*!Must be at least the size of input.*/
    bool			setWindow(const char* nm,float param,
	    				  int sz0,int sz1);
    int				getWindowSize(int dim) const;
    const char*			getWindowName() const;
    float			getWindowParam() const;

    inline void			fillPar(IOPar&) const;
    inline bool			usePar(const IOPar&);

    inline void			setProgressMeter(ProgressMeter* pm);
    inline void			enableNrDoneCounting(bool yn);
    inline bool			execute();
    inline void			enableWorkControl(bool);
    inline void			controlWork(Task::Control);
    inline Task::Control	getState() const;

    static const char*		sKeyWinFunc() { return "Window function"; }
    static const char*		sKeyWinParam() { return "Window parameter"; }
    static const char*		sKeyWinSize() { return "Window size"; }

protected:

    Convolver2D<T>		convolver_;
    bool			hasudf_;

    Array2DImpl<T>*		window_;
    int				windowsz0_, windowsz1_;
    BufferString		windowname_;
    float			windowparam_;
};


template <class T> inline
Smoother2D<T>::Smoother2D()
    : windowparam_( mUdf(float) )
    , window_( 0 )
{
    convolver_.setNormalize( true );
    convolver_.setCorrelate( false );
}


template <class T> inline
Smoother2D<T>::~Smoother2D()
{ delete window_; }


template <class T> inline
int Smoother2D<T>::getWindowSize(int dim) const
{ return window_->info().getSize( dim ); }


template <class T> inline
const char* Smoother2D<T>::getWindowName() const
{ return windowname_.buf(); }


template <class T> inline
float Smoother2D<T>::getWindowParam() const
{ return windowparam_; }


template <class T> inline
void Smoother2D<T>::setInput( const Array2D<T>& ni, bool hasudf )
{
    const Array2D<float>* input = convolver_.getX();

    if ( !input || hasudf_!=hasudf || input->info()!=ni.info() )
    { delete window_; window_ = 0; }

    convolver_.setX( ni, hasudf );
    hasudf_ = hasudf;
}


template <class T> inline
void Smoother2D<T>::setOutput( Array2D<T>& no )
{
    convolver_.setZ( no );
}


template <class T> inline
bool Smoother2D<T>::setWindow( const char* nm, float param,
			       int sz0, int sz1 )
{
    windowname_ = nm;
    windowparam_ = param;
    windowsz0_ = sz0;
    windowsz1_ = sz1;

    delete window_; window_ = 0;

    return true;
}


template <class T> inline
void Smoother2D<T>::fillPar( IOPar& par ) const
{
    par.set( sKeyWinFunc(), windowname_ );
    if ( !mIsUdf(windowparam_) )
	par.set( sKeyWinParam(), windowparam_ );

    par.set(sKeyWinSize(),window_->info().getSize(0),window_->info().getSize(1));
}


template <class T> inline
bool Smoother2D<T>::usePar( const IOPar& par )
{
    int sz0, sz1;
    if ( !par.get(sKeyWinSize(), sz0, sz1 ) )
	return false;

    const char* wn = par.find( sKeyWinFunc() );
    float var = mUdf(float);
    par.get( sKeyWinParam(), var );

    return setWindow( wn, var, sz0, sz1 );
}


#define mImplSetFunc( func, vartype ) \
template <class T> inline void Smoother2D<T>::func( vartype var ) \
{ convolver_.func( var ); }

mImplSetFunc( setProgressMeter, ProgressMeter* );
mImplSetFunc( enableNrDoneCounting, bool );
mImplSetFunc( enableWorkControl, bool);
mImplSetFunc( controlWork, Task::Control);

template <class T> inline
bool Smoother2D<T>::execute()
{
    const Array2D<float>* input = convolver_.getX();
    if ( !input )
	return false;

    if ( !window_ )
    {
	PtrMan<WindowFunction> wf = WINFUNCS().create( windowname_ );
	if ( !wf )
	    return false;

	if ( wf->hasVariable() &&
		(mIsUdf(windowparam_) || !wf->setVariable( windowparam_ ) ) )
	    return false;

	if ( windowsz0_<=0 || windowsz1_<=0 )
	    return false;

	//Can we setup for fft? If so, all volumes should have same size
	if ( typeid(T)==typeid(float) && input->getData() && !hasudf_ )
	    window_ = new Array2DImpl<T>( input->info() );
	else
	    window_ = new Array2DImpl<T>( windowsz0_, windowsz1_ );

	if ( !window_->isOK() )
	    return false;

	const float hwinsz0 = ((float)windowsz0_)/2;
	const float hwinsz1 = ((float)windowsz1_)/2;
	Coord pos;

	const int sz0 = window_->info().getSize( 0 );
	const int sz1 = window_->info().getSize( 1 );
	const int hsz0 = sz0/2;
	const int hsz1 = sz1/2;

	double weightsum = 0;

	for ( int idx0=0; idx0<sz0; idx0++ )
	{
	    const float pos0 = mCast( float, idx0>hsz0 ? idx0-sz0 : idx0 );
	    pos[0] = pos0/hwinsz0;
	    for ( int idx1=0; idx1<sz1; idx1++ )
	    {
		const float pos1 = mCast( float, idx1>hsz1 ? idx1-sz1 : idx1 );
		pos[1] = pos1/hwinsz1;
		const float weight = wf->getValue( (float) pos.abs() );
		window_->set( idx0, idx1, weight );
		weightsum += weight;
	    }
	}

	if ( weightsum>1 )
	    mPointerOperation( float, window_->getData(), /= (float)weightsum,
		        window_->info().getTotalSz(), ++ );

	convolver_.setY( *window_, false );
    }

    return convolver_.execute();
}


template <class T> inline
Task::Control Smoother2D<T>::getState() const
{ return convolver_.getState(); }

#endif
