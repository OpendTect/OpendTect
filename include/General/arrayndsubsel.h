#ifndef arrayndsubsel_h
#define arrayndsubsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Nov 2005
 RCS:		$Id$
________________________________________________________________________

-*/


#include "arraynd.h"

/*!
\brief Makes a subselection of an Array2D cube.
*/

template <class T>
mClass(General) Array2DSubSelection : public Array2D<T>
{
public:
    		Array2DSubSelection( int start0, int start1,
				     int sz0, int sz1,
				     Array2D<T>& data );
		/*!<\param start0	position of the subselection in data.
		    \param start1	position of the subselection in data.
		    \param sz0		size of the subselection.
		    \param sz1		size of the subselection.
		    \param data		The array in which the subselection
		    			is done. */

    T		get( int, int ) const;
    void	set( int, int, T );
    bool	isOK() const;

    const Array2DInfo&	info() const;

protected:

    int			start_[2];
    Array2DInfoImpl	info_;
    Array2D<T>&		src_;
};


/*!
Makes a subselection of an Array3D cube.
*/

template <class T>
mClass(General) Array3DSubSelection : public Array3D<T>
{
public:
    		Array3DSubSelection( int start0, int start1, int start2,
				     int sz0, int sz1, int sz2,
				     Array3D<T>& data );
		/*!<\param start0	position of the subselection in data.
		    \param start1	position of the subselection in data.
		    \param start2	position of the subselection in data.
		    \param sz0		size of the subselection.
		    \param sz1		size of the subselection.
		    \param sz2		size of the subselection.
		    \param data		The array in which the subselection
		    			is done. */

    T		get( int, int, int ) const;
    void	set( int, int, int, T );
    bool	isOK() const;

    const Array3DInfo&	info() const;

protected:

    int			start_[3];
    Array3DInfoImpl	info_;
    Array3D<T>&		src_;
};


#define mSetupDim( dim ) \
    start_[dim] = s##dim; \
    info_.setSize( dim, mMIN( sz##dim, src_.info().getSize(dim)-s##dim) );


template <class T>
Array2DSubSelection<T>::Array2DSubSelection( int s0, int s1, 
					     int sz0, int sz1, 
					     Array2D<T>& data )
    : src_( data )
{
    mSetupDim(0); mSetupDim(1);
}


template <class T>
void Array2DSubSelection<T>::set( int s0, int s1, T val )
{
    s0 += start_[0];
    s1 += start_[1];

    if ( !src_.info().validPos( s0, s1 ) )
	pErrMsg("Position is outside of src" );
    else
	src_.set( s0, s1, val );
}


template <class T>
T Array2DSubSelection<T>::get( int s0, int s1 ) const
{
    s0 += start_[0];
    s1 += start_[1];

    return src_.info().validPos( s0, s1 )
	? src_.get( s0, s1 ) : mUdf(T);
}


template <class T>
bool Array2DSubSelection<T>::isOK() const
{
    if ( !src_.isOK() )
	return false;

    for ( int dim=info_.getNDim()-1; dim>=0; dim-- )
    {
	if ( start_[dim]<0 || start_[dim]>=src_.info().getSize(dim) ||
	     info_.getSize(dim)<=0 )
	    return false;
    }

    return true;
}


template <class T>
const Array2DInfo& Array2DSubSelection<T>::info() const
{ return info_; }


template <class T>
Array3DSubSelection<T>::Array3DSubSelection( int s0, int s1, int s2,
					     int sz0, int sz1, int sz2,
					     Array3D<T>& data )
    : src_( data )
{
    mSetupDim(0);
    mSetupDim(1);
    mSetupDim(2);
}


template <class T>
void Array3DSubSelection<T>::set( int s0, int s1, int s2, T val )
{
    s0 += start_[0];
    s1 += start_[1];
    s2 += start_[2];

    if ( !src_.info().validPos( s0, s1, s2 ) )
	pErrMsg("Position is outside of src" );
    else
	src_.set( s0, s1, s2, val );
}


template <class T>
T Array3DSubSelection<T>::get( int s0, int s1, int s2 ) const
{
    s0 += start_[0];
    s1 += start_[1];
    s2 += start_[2];

    return src_.info().validPos( s0, s1, s2 )
	? src_.get( s0, s1, s2 ) : mUdf(T);
}


template <class T>
bool Array3DSubSelection<T>::isOK() const
{
    if ( !src_.isOK() )
	return false;

    for ( int dim=info_.getNDim()-1; dim>=0; dim-- )
    {
	if ( start_[dim]<0 || start_[dim]>=src_.info().getSize(dim) ||
	     info_.getSize(dim)<=0 )
	    return false;
    }

    return true;
}


template <class T>
const Array3DInfo& Array3DSubSelection<T>::info() const
{ return info_; }



#endif


