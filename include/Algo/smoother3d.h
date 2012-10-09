#ifndef smoother3d_h
#define smoother3d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Feb 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "task.h"
#include "convolve3d.h"
#include "arrayndimpl.h"
#include "iopar.h"
#include "windowfunction.h"

/*!Smothes a 3d signal with an an operator. */


template <class T>
class Smoother3D : public Task
{
public:

				Smoother3D();

    void			setInput(const Array3D<T>&);
    void			setOutput(Array3D<T>&);
   		 		/*!Must be at least the size of input.*/
    bool			setWindow(const char* nm,float param,
	    				  int sz0,int sz1,int sz2);
    int				getWindowSize(int dim) const;
    const char*			getWindowName() const;
    float			getWindowParam() const;

    inline void			fillPar(IOPar&) const;
    inline bool			usePar(const IOPar&);

    inline void			setProgressMeter( ProgressMeter* pm );
    inline void			enableNrDoneCounting( bool yn );
    inline bool			execute();
    inline void			enableWorkControl(bool);
    inline void			controlWork(Task::Control);
    inline Task::Control	getState() const;

    static const char*		sKeyWinFunc() { return "Window function"; }
    static const char*		sKeyWinParam() { return "Window parameter"; }
    static const char*		sKeyWinSize() { return "Window size"; }

protected:

    Convolver3D<T>		convolver_;

    Array3DImpl<T>		window_;
    BufferString		windowname_;
    float			windowparam_;
};


template <class T> inline
Smoother3D<T>::Smoother3D()
    : windowparam_( mUdf(float) )
    , window_( 1, 1, 1 )
{
    convolver_.setNormalize( true );
    convolver_.setCorrelate( false );
    window_.set( 0, 0, 0, 1 );
}


template <class T> inline
int Smoother3D<T>::getWindowSize(int dim) const
{ return window_.info().getSize( dim ); }


template <class T> inline
const char* Smoother3D<T>::getWindowName() const
{ return windowname_.buf(); }


template <class T> inline
float Smoother3D<T>::getWindowParam() const
{ return windowparam_; }


template <class T> inline
void Smoother3D<T>::setInput( const Array3D<T>& ni )
{
    convolver_.setX( ni, 0, 0, 0 );
}


template <class T> inline
void Smoother3D<T>::setOutput( Array3D<T>& no )
{
    convolver_.setZ( no );
}


template <class T> inline
bool Smoother3D<T>::setWindow( const char* nm, float param,
			       int sz0, int sz1, int sz2 )
{
    PtrMan<WindowFunction> wf = WinFuncs().create( nm );
    if ( !wf )
	return false;

    if ( wf->hasVariable() && !wf->setVariable( param ) )
	return false;

    if ( sz0<=0 || sz1<=0 || sz2<=0 )
	return false;

    window_.setSize( sz0, sz1, sz2 );
    if ( !window_.isOK() )
	return false;

    const int hsz0 = sz0/2; const int hsz1 = sz1/2; const int hsz2 = sz2/2;
    Coord3 pos;

    for ( int idx0=0; idx0<sz0; idx0++ )
    {
	pos[0] = hsz0 ? ((double)(idx0-hsz0))/hsz0 : 0;
	for ( int idx1=0; idx1<sz1; idx1++ )
	{
	    pos[1] = hsz1 ? ((double)(idx1-hsz1))/hsz1 : 0;
	    for ( int idx2=0; idx2<sz2; idx2++ )
	    {
		pos[2] = hsz2 ? ((double)(idx2-hsz2))/hsz2 : 0;

		window_.set( idx0, idx1, idx2, wf->getValue( pos.abs() ) );
	    }
	}
    }

    convolver_.setY( window_, hsz0, hsz1, hsz2 );

    windowname_ = nm;
    windowparam_ = wf->hasVariable() ? param : 1e30;

    return true;
}


template <class T> inline
void Smoother3D<T>::fillPar( IOPar& par ) const
{
    par.set( sKeyWinFunc(), windowname_ );
    if ( !mIsUdf(windowparam_) )
	par.set( sKeyWinParam(), windowparam_ );
    par.set( sKeyWinSize(), window_.info().getSize(0),window_.info().getSize(1),
			    window_.info().getSize(2) );
}


template <class T> inline
bool Smoother3D<T>::usePar( const IOPar& par )
{
    int sz0, sz1, sz2;
    if ( !par.get(sKeyWinSize(), sz0, sz1, sz2 ) )
	return false;

    const char* wn = par.find( sKeyWinFunc() );
    float var = mUdf(float);
    par.get( sKeyWinParam(), var );

    return setWindow( wn, var, sz0, sz1, sz2 );
}


#define mImplSetFunc( func, vartype ) \
template <class T> inline void Smoother3D<T>::func( vartype var ) \
{ convolver_.func( var ); }

mImplSetFunc( setProgressMeter, ProgressMeter* );
mImplSetFunc( enableNrDoneCounting, bool );
mImplSetFunc( enableWorkControl, bool);
mImplSetFunc( controlWork, Task::Control);

template <class T> inline
bool Smoother3D<T>::execute()
{
    if ( !window_.isOK() )
	return false;

    return convolver_.execute();
}


template <class T> inline
Task::Control Smoother3D<T>::getState() const
{ return convolver_.getState(); }

#endif
