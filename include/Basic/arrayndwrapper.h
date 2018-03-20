#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2007
________________________________________________________________________

-*/

#include "arraynd.h"

/*!\brief Access tool to another array with a lower number of dimensions */

//TODO: Write more info
mClass(Basic) ArrayNDWrapper
{
public:

			mTypeDefArrNDTypes;

    void		setDimMap(DimIdxType srcdim,DimIdxType targetdim)
			{ dimmap_[srcdim] = targetdim; }

    virtual void	init()		= 0;
    virtual bool	isOK() const	= 0;

protected:
			ArrayNDWrapper(const ArrayNDInfo& info)
			{ dimmap_.setSize( info.nrDims(), 0 ); }

    TypeSet<DimIdxType>	dimmap_;

};



/*!
\brief Subclass of ArrayNDWrapper.
*/

template <class T>
mClass(Basic) Array3DWrapper : public Array3D<T>, public ArrayNDWrapper
{
public:
			mTypeDefArrNDTypes;

			Array3DWrapper(ArrayND<T>&);
			~Array3DWrapper();

    void		init();
    bool		isOK() const;

    void		set(IdxType,IdxType,IdxType,T);
    T			get(IdxType,IdxType,IdxType) const;

    const Array3DInfo&	info() const		{ return info_; }

protected:

    Array3DInfo&	info_;
    ArrayND<T>&		srcarr_;
};



template <class T>
Array3DWrapper<T>::Array3DWrapper( ArrayND<T>& arr )
    : ArrayNDWrapper(arr.info())
    , srcarr_(arr)
    , info_(*new Array3DInfoImpl(1,1,1))
{
}

template <class T>
Array3DWrapper<T>::~Array3DWrapper()
{ delete &info_; }


template <class T>
void Array3DWrapper<T>::init()
{
    for ( DimIdxType idx=0; idx<dimmap_.size(); idx++ )
	info_.setSize( dimmap_[idx], srcarr_.getSize(idx) );
}


template <class T>
bool Array3DWrapper<T>::isOK() const
{ return srcarr_.nrDims() <  3; }


template <class T>
void Array3DWrapper<T>::set( IdxType i0, IdxType i1, IdxType i2, T val )
{
    IdxType pos3d[] = { i0, i1, i2 };
    TypeSet<IdxType> posnd;
    for ( int idx=0; idx<dimmap_.size(); idx++ )
	posnd += pos3d[ dimmap_[idx] ];

    srcarr_.setND( posnd.arr(), val );
}


template <class T>
T Array3DWrapper<T>::get( IdxType i0, IdxType i1, IdxType i2 ) const
{
    IdxType pos3d[] = { i0, i1, i2 };
    TypeSet<IdxType> posnd;
    for ( int idx=0; idx<dimmap_.size(); idx++ )
	posnd += pos3d[ dimmap_[idx] ];

    return srcarr_.getND( posnd.arr() );
}
