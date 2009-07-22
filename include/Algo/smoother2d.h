#ifndef smoother2d_h
#define smoother2d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Feb 2008
 RCS:		$Id: smoother2d.h,v 1.2 2009-07-22 16:01:12 cvsbert Exp $
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

    void			setInput(const Array2D<T>&);
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

    Array2DImpl<T>		window_;
    BufferString		windowname_;
    float			windowparam_;
};


template <class T> inline
Smoother2D<T>::Smoother2D()
    : windowparam_( mUdf(float) )
    , window_( 1, 1 )
{
    convolver_.setNormalize( true );
    convolver_.setCorrelate( false );
    window_.set( 0, 0, 1 );
}


template <class T> inline
int Smoother2D<T>::getWindowSize(int dim) const
{ return window_.info().getSize( dim ); }


template <class T> inline
const char* Smoother2D<T>::getWindowName() const
{ return windowname_.buf(); }


template <class T> inline
float Smoother2D<T>::getWindowParam() const
{ return windowparam_; }


template <class T> inline
void Smoother2D<T>::setInput( const Array2D<T>& ni )
{
    convolver_.setX( ni, 0, 0 );
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
    PtrMan<WindowFunction> wf = WinFuncs().create( nm );
    if ( !wf )
	return false;

    if ( wf->hasVariable() && !wf->setVariable( param ) )
	return false;

    if ( sz0<=0 || sz1<=0 )
	return false;

    window_.setSize( sz0, sz1 );
    if ( !window_.isOK() )
	return false;

    const int hsz0 = sz0/2; const int hsz1 = sz1/2;
    Coord pos;

    for ( int idx0=0; idx0<sz0; idx0++ )
    {
	pos[0] = hsz0 ? ((double)(idx0-hsz0))/hsz0 : 0;
	for ( int idx1=0; idx1<sz1; idx1++ )
	{
	    pos[1] = hsz1 ? ((double)(idx1-hsz1))/hsz1 : 0;
	    window_.set( idx0, idx1, wf->getValue( pos.abs() ) );
	}
    }

    convolver_.setY( window_, hsz0, hsz1 );

    windowname_ = nm;
    windowparam_ = wf->hasVariable() ? param : 1e30;

    return true;
}


template <class T> inline
void Smoother2D<T>::fillPar( IOPar& par ) const
{
    par.set( sKeyWinFunc(), windowname_ );
    if ( !mIsUdf(windowparam_) )
	par.set( sKeyWinParam(), windowparam_ );
    par.set(sKeyWinSize(),window_.info().getSize(0),window_.info().getSize(1));
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
    if ( !window_.isOK() )
	return false;

    return convolver_.execute();
}


template <class T> inline
Task::Control Smoother2D<T>::getState() const
{ return convolver_.getState(); }

#endif
