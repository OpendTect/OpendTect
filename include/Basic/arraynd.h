#ifndef arraynd_h
#define arraynd_h
/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: arraynd.h,v 1.6 2000-05-25 16:08:14 bert Exp $
________________________________________________________________________

*/

#include <gendefs.h>
template <class T> class TypeSet;


class ArrayNDSize
{
public:

    virtual ArrayNDSize* clone() const		= 0;
    virtual		~ArrayNDSize()		{} 

    virtual int		getNDim() const		{ return 1; }
    virtual int		getSize(int dim) const	= 0;
    virtual bool	setSize(int dim,int sz) = 0;
 
    inline unsigned long  getTotalSz() const	{ return totalSz; }
    virtual unsigned long getArrayPos(const TypeSet<int>&) const	= 0;
    virtual bool	  validPos(const TypeSet<int>&) const		= 0;

protected:

    unsigned long 		totalSz;

    virtual unsigned long	calcTotalSz()	{ return getSize(0); }

};

inline bool operator ==( const ArrayNDSize& a1, const ArrayNDSize& a2 )
{
    int nd = a1.getNDim();
    if ( nd != a2.getNDim() ) return NO;
    for ( int idx=0; idx<nd; idx++ )
	if ( a1.getSize(idx) != a2.getSize(idx) ) return NO;
    return YES;
}

inline bool operator !=( const ArrayNDSize& a1, const ArrayNDSize& a2 )
{ return !(a1 == a2); }


class Array1DSize : public ArrayNDSize
{
public:

    int				getNDim() const			{ return 1; }

    virtual unsigned long	getArrayPos(int) const		= 0;
    virtual bool		validPos(int) const		= 0;

};


class Array2DSize : public ArrayNDSize
{
public:

    int				getNDim() const			{ return 2; }

    virtual unsigned long	getArrayPos(int,int) const	= 0;
    virtual bool		validPos(int,int) const		= 0;

};


class Array3DSize : public ArrayNDSize
{
public:

    int				getNDim() const			{ return 3; }

    virtual unsigned long	getArrayPos(int, int, int) const= 0;
    virtual bool		validPos(int,int,int) const	= 0;

};


template <class T> class ArrayND
{
public:

    virtual			~ArrayND()	{}

    virtual T		getValOff( unsigned long off ) const
				{
				    T res = *(T*)(getData()+off);
				    unlockData();
				    return res;
				}
    virtual void		setValOff(unsigned long off, T val)
                                {
				    *(T*)(getData()+off) = val;
				    dataUpdated();
				    unlockData();
				}

    virtual T                getVal( const TypeSet<int>& pos ) const
                                { return getValOff( size().getArrayPos(pos) ); }
    virtual void		setVal( const TypeSet<int>& pos, T val )
				{ setValOff( size().getArrayPos( pos ), val ); }

    virtual T*		getData() const			= 0;
    virtual void		dataUpdated()			{}
    virtual void		unlockData() const		{}

    virtual const ArrayNDSize&	size() const			= 0;

    class Proxy
    {
    public:
			Proxy( ArrayND<T>& array_, unsigned long off_ )
			    : array( array_ )
			    , off( off_ )				{}	

	Proxy&		operator=( const Proxy& n )
			{ array.setValOff( off, n ); return *this; }
	Proxy&		operator=( T n  )
			{ array.setValOff( off, n ); return *this; }
	Proxy&		operator+=( T n )
			{
			    array.setValOff( off, array.getValOff(off)+n);
			    return *this;
			}
	Proxy&		operator-=( T n )
			{
			    array.setValOff( off, array.getValOff(off)-n);
			    return *this;
			}
	Proxy&		operator*=( T n )
			{
			    array.setValOff( off, array.getValOff(off)*n);
			    return *this;
			}
	Proxy&		operator/=( T n )
			{
			    array.setValOff( off, array.getValOff(off)/n);
			    return *this;
			}

			operator T() const
			{ return array.getValOff( off ); }
    private:

	ArrayND<T>&	array;
	unsigned long	off;

    };			
};


template <class Type> class Array1D : public ArrayND<Type>
{
public: 

    virtual void        setVal( int p, Type val )
                        { setValOff(size().getArrayPos(p),val); }
    virtual Type        getVal( int p ) const
			{ return getValOff(size().getArrayPos(p)); }

    Type		operator() (int pos) const
			{ return getVal( pos ); }
    ArrayND<Type>::Proxy operator() (int pos)
			{ return ArrayND<Type>::Proxy( *this, pos ); }

    virtual const Array1DSize& size() const = 0;
};



template <class Type> class Array2D : public ArrayND<Type>
{
public: 

    virtual void        setVal( int p0, int p1, Type val )
                        { setValOff(size().getArrayPos(p0,p1),val); }
    virtual Type        getVal( int p0, int p1 ) const
			{ return getValOff(size().getArrayPos(p0,p1)); }

    Type		operator() (int p0, int p1) const
			{ return getValOff(size().getArrayPos(p0,p1)); }
    ArrayND<Type>::Proxy operator() (int p0, int p1)
			{
			    return ArrayND<Type>::Proxy(*this,
						size().getArrayPos(p0,p1));
			}

    virtual const Array2DSize& size() const = 0;

};


template <class Type> class Array3D : public ArrayND<Type>
{
public: 

    virtual void        setVal( int p0, int p1, int p2, Type val )
                        { setValOff(size().getArrayPos(p0,p1,p2),val); }
    virtual Type        getVal( int p0, int p1, int p2 ) const
			{ return getValOff(size().getArrayPos(p0,p1,p2)); }

    Type		operator() (int p0, int p1, int p2) const
			{ return getValOff(size().getArrayPos(p0,p1,p2)); }
    ArrayND<Type>::Proxy operator() (int p0, int p1, int p2)
			{
			    return ArrayND<Type>::Proxy(*this,
						size().getArrayPos(p0,p1,p2));
			}

    virtual const Array3DSize& size() const = 0;
};


#endif
