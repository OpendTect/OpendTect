#ifndef arrayndwrapper_h
#define arrayndwrapper_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2007
 RCS:		$Id: arrayndwrapper.h,v 1.6 2012-08-03 20:56:09 cvskris Exp $
________________________________________________________________________

-*/


/*! \brief Access tool to another array with a lower number of dimensions
*/

//TODO: Write more info

class ArrayNDWrapper
{
public:
    void		setDimMap(int srcdim,int targetdim);
    virtual void	init()		= 0;
    virtual bool	isOK() const	= 0;

protected:
    			ArrayNDWrapper(const ArrayNDInfo&);
    TypeSet<int>	dimmap_;
};


ArrayNDWrapper::ArrayNDWrapper( const ArrayNDInfo& info )
{ dimmap_.setSize( info.getNDim(), 0 ); }

void ArrayNDWrapper::setDimMap( int srcdim, int targetdim )
{ dimmap_[srcdim] = targetdim; }



template <class T>
class Array3DWrapper : public Array3D<T>, public ArrayNDWrapper
{
public:
    			Array3DWrapper(ArrayND<T>&);

    void		init();
    bool		isOK() const;

    void		set(int,int,int,T);
    T			get(int,int,int) const;

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
void Array3DWrapper<T>::init()
{
    for ( int idx=0; idx<dimmap_.size(); idx++ )
	info_.setSize( dimmap_[idx], srcarr_.info().getSize(idx) );
}


template <class T>
bool Array3DWrapper<T>::isOK() const
{ return srcarr_.info().getNDim() <  3; }


template <class T>
void Array3DWrapper<T>::set( int i0, int i1, int i2, T val )
{
    int pos3d[] = { i0, i1, i2 };
    TypeSet<int> posnd;
    for ( int idx=0; idx<dimmap_.size(); idx++ )
	posnd += pos3d[ dimmap_[idx] ];

    srcarr_.setND( posnd.arr(), val );
}


template <class T>
T Array3DWrapper<T>::get( int i0, int i1, int i2 ) const
{
    int pos3d[] = { i0, i1, i2 };
    TypeSet<int> posnd;
    for ( int idx=0; idx<dimmap_.size(); idx++ )
	posnd += pos3d[ dimmap_[idx] ];

    return srcarr_.getND( posnd.arr() );
}

#endif

