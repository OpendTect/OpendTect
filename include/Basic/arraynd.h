#ifndef arraynd_h
#define arraynd_h
/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: arraynd.h,v 1.10 2001-03-23 11:27:01 bert Exp $
________________________________________________________________________

An ArrayND is an array with a given number of dimensions and a size. The
ArrayND can be accessed via set() and get().

The ArrayND can give away a pointer to it's storage, but there is no
guarantee that it will. If no pointer is given, the user can copy the
ArrayND by constructing an ArrayNDImpl with the original array as an argument
to the constructor.

*/

#include <gendefs.h>
#include <arrayndinfo.h>

template <class T>
class ArrayND 
{
public:
				// Read specs
    virtual T	                get( const int* ) const			= 0;

    inline const T*		getData() const		{ return getData_(); }
    virtual const T*		get1D(const int*) const;
    virtual int			get1DDim() const;

				// Write specs
    virtual bool		isSettable() const { return true; }
    virtual void		set( const int*, T ) 			= 0;

    inline T*			getData();
    virtual T*			get1D( const int* i );

    virtual const ArrayNDInfo&	info() const				= 0;


protected:

 
    virtual const T*		getData_() const		{ return 0; }

};


template <class T>
class Array1D : public ArrayND<T>
{
public: 
    virtual void		set(int,T)				= 0;
    virtual T			get(int) const				= 0;
    void			set(const int* pos,T v) { set( pos[0], v ); }
    T	                	get(const int* pos) const {return get(pos[0]);}

    virtual const Array1DInfo&	info() const = 0;
};


template <class T>
class Array2D : public ArrayND<T>
{
public: 
    virtual void		set( int, int, T ) 			= 0;
    virtual T        		get( int p0, int p1 ) const		= 0;
    void			set(  const int* pos, T v )
				    { set( pos[0], pos[1], v);}
    T		                get( const int* pos ) const
				    { return get( pos[0], pos[1] ); }

    virtual const Array2DInfo&	info() const = 0;
};


template <class T> class Array3D : public ArrayND<T>
{
public: 

    virtual void		set( int, int, int, T ) 		= 0;
    virtual T        		get( int p0, int p1, int p2 ) const	= 0;
    void			set( const int* pos, T v )
				    { set( pos[0], pos[1], pos[2], v);}
    T		                get( const int* pos ) const
				    { return get( pos[0], pos[1], pos[2] ); }

    virtual const Array3DInfo&	info() const				= 0;
};

template <class T> inline
const T* ArrayND<T>::get1D( const int* i ) const
{
    const T* ptr = getData();
    if ( !ptr ) return 0;

    int ndim = info().getNDim();

    int pos[ndim];
    memcpy(pos,i,sizeof(int)*(ndim-1));

    pos[ndim-1] = 0;
    
    return &ptr[info().getMemPos( pos )];
}


template <class T> inline
int ArrayND<T>::get1DDim() const
{ return info().getNDim()-1; }


template <class T> inline
T* ArrayND<T>::getData()
{
    return isSettable() ? const_cast<T*>(((const ArrayND*)this)->getData_()): 0;
}


template <class T> inline
T* ArrayND<T>::get1D( const int* i )
{
    return isSettable() ? const_cast<T*>(((const ArrayND*)this)->get1D(i)) :0;
}

#endif
