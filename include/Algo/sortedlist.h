#ifndef sortedlist_h
#define sortedlist_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		19-4-2000
 Contents:	Array sorting
 RCS:		$Id: sortedlist.h,v 1.3 2002-02-22 11:21:00 kristofer Exp $
________________________________________________________________________

-*/

#include "gendefs.h"

#ifndef Vector_H
#include <Vector.h>
#endif

/*!\brief
  A SortedList is a list where all objects are stored in ascending order.
  The objects should be capable of doing <,> and ==. If allowmultiples
  is true, multiple objects with the same value are allowed in the list. 

  A SortedList can be used together with all other lists that have a []
  operator, such as TypeSets.
  */

template <class T>
class SortedList
{
public:
    			SortedList(bool allowmultiples_)
			    : allowmultiples( allowmultiples_ ) {}

    int 		size() const { return typs.size(); }
    const T&		operator[]( int idx ) const { return (T&)typs[idx]; }
    int			indexOf( const T& ) const;
    			/*!< Returns -1 if not fount */

    SortedList<T>&	operator +=( const T& );
    SortedList<T>&	operator -=( const T& );

    template <class U> void		intersect( const U& );
					/*!< Remove all entries not present
					     in both lists. U should be capable
					     of doing an indexOf().
					 */

    template <class U> SortedList<T>&	copy( const U& array );

    template <class U> SortedList<T>&	operator =( const U& array );

    template <class U> SortedList<T>&	operator +=( const U& array );

    template <class U> SortedList<T>&	operator -=( const U& array );

    void				erase();
    void				remove( int idx );

private:
    int			getPos( const T& ) const;
    			/*!< If not found, it will return position of the
			     item just above
			 */

    bool		allowmultiples;
    Vector<T>		typs;
};


template <class T> inline
int SortedList<T>::getPos( const T& typ ) const
{
    int sz = size();
    if ( !sz ) return 0;

    int start = 0;
    int stop = sz-1;

    T startval = (*this)[start];
    T stopval = (*this)[stop];

    if ( typ > stopval ) return sz;

    while ( stopval > startval )
    {
	int middle = (start+stop)>>2;
	T middleval = (*this)[middle];

	if ( middleval > typ )
	{
	    stopval = middleval;
	    stop = middle;
	}
	else
	{
	    startval = middleval;
	    start = middle;
	}
    }

    return start;
}


template <class T> inline
int SortedList<T>::indexOf( const T& typ ) const
{
    if ( !typs.size() ) return -1;

    int pos = getPos( typ );

    if ( (*this)[pos]!=typ )
        return -1;

    return pos;
}


template <class T> inline
SortedList<T>&	SortedList<T>::operator +=( const T& nv )
{
    int newpos = getPos( nv );

    if ( newpos == size() )
    {
	typs.push_back( nv );
	return *this;
    }

    if ( !allowmultiples && (*this)[newpos] == nv )
	return *this;

    typs.insert( newpos, nv );
    return *this;
}


template <class T> inline
SortedList<T>&	SortedList<T>::operator -=( const T& nv )
{
    int sz = size();
    if ( !sz ) return *this;

    int pos = indexOf( nv );

    if ( pos == -1 ) return *this;

    typs.remove( pos );
    return *this;
}


template <class T> template <class U> inline
void  SortedList<T>::intersect( const U& b )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( b.indexOf( (*this)[idx]) == -1 )
	{
	    remove( idx );
	    idx--;
	}
    }
}


template <class T> template <class U> inline
SortedList<T>& SortedList<T>::copy( const U& array )
{
    erase();
    int sz = array.size();
    for ( int idx=0; idx<sz; idx++ )
	(*this) += array[idx];
    return *this;
}


template <class T> template <class U> inline
SortedList<T>&	SortedList<T>::operator =( const U& array )
{ return copy(ts); }


template <class T> template <class U> inline
SortedList<T>&  SortedList<T>::operator +=( const U& array )
{
    int sz = array.size();
    for ( int idx=0; idx<sz; idx++ )
	(*this) += array[idx];
    return *this;
}


template <class T> template <class U> inline
SortedList<T>&  SortedList<T>::operator -=( const U& array )
{
    int sz = array.size();
    for ( int idx=0; idx<sz; idx++ )
	(*this) -= array[idx];
    return *this;
}


template <class T> inline
void SortedList<T>::erase() { typs.erase(); }


template <class T> inline
void  SortedList<T>::remove( int pos )
{
    typs.remove( pos );
}

#endif
