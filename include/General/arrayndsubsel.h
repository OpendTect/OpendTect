#ifndef arrayndsubsel_h
#define arrayndsubsel_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		Nov 2005
 RCS:		$Id: arrayndsubsel.h,v 1.3 2005-11-29 21:35:34 cvskris Exp $
________________________________________________________________________

-*/


#include "arraynd.h"

/*! Makes a subselection of an Array3D cube. */


template <class T>
class Array3DSubSelection : public Array3D<T>
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

    int			start[3];
    Array3DInfoImpl	info_;
    Array3D<T>&		src;
};


#define mSetupDim( dim ) \
    start[dim] = s##dim; \
    info_.setSize( dim, mMIN( sz##dim, src.info().getSize(dim)-s##dim) );




template <class T>
Array3DSubSelection<T>::Array3DSubSelection( int s0, int s1, int s2,
					     int sz0, int sz1, int sz2,
					     Array3D<T>& data )
    : src( data )
{
    mSetupDim(0);
    mSetupDim(1);
    mSetupDim(2);
}


template <class T>
void Array3DSubSelection<T>::set( int s0, int s1, int s2, T val )
{
    s0 += start[0];
    s1 += start[1];
    s2 += start[2];

    if ( !src.info().validPos( s0, s1, s2 ) )
	pErrMsg("Position is outside of src" );
    else
	src.set( s0, s1, s2, val );
}


template <class T>
T Array3DSubSelection<T>::get( int s0, int s1, int s2 ) const
{
    s0 += start[0];
    s1 += start[1];
    s2 += start[2];

    return src.info().validPos( s0, s1, s2 )
	? src.get( s0, s1, s2 ) : mUdf(T);
}


template <class T>
bool Array3DSubSelection<T>::isOK() const
{
    for ( int dim=info_.getNDim()-1; dim>=0; dim-- )
    {
	if ( start[dim]<0 || start[dim]>=src.info().getSize(dim) ||
	     info_.getSize(dim)<=0 )
	    return false;
    }

    return true;
}


template <class T>
const Array3DInfo& Array3DSubSelection<T>::info() const
{ return info_; }



#endif


