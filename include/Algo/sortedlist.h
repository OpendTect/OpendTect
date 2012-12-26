#ifndef sortedlist_h
#define sortedlist_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		19-4-2000
 Contents:	Array sorting
 RCS:		$Id$
________________________________________________________________________

-*/

#include "gendefs.h"
#include "vectoraccess.h"

/*!
\ingroup Algo
\brief A SortedList is a list where all objects are stored in ascending order.
The objects should be capable of doing <,> and ==. If allowmultiples is true,
multiple objects with the same value are allowed in the list. 

  A SortedList can be used together with all other lists that have a []
  operator, such as TypeSets.
*/

template <class T>
class SortedList
{
public:
    			SortedList( bool allowmultiples_ )
			    : allowmultiples(allowmultiples_) {}

    int 		size() const			{ return tvec.size(); }
    const T&		operator[]( int idx ) const	{ return (T&)tvec[idx];}
    int			indexOf(const T&) const;
    			/*!< Returns -1 if not found */

    SortedList<T>&	operator +=(const T&);
    SortedList<T>&	operator -=(const T&);

    // The following functions handle external indexables: classes or
    // arrays - things that support the [] operator.

    template <class U> SortedList<T>&	copy(const U&);
    template <class U> SortedList<T>&	operator =(const U&);
    template <class U> SortedList<T>&	operator +=(const U&);
    template <class U> SortedList<T>&	operator -=(const U&);
    template <class U> void		intersect(const U&);
					/*!< Remove all entries not present
					     in both lists. */

    void		erase();
    void		remove( int idx );

    std::vector<T>&	vec()		{ return tvec.vec(); }
    const std::vector<T>& vec() const	{ return tvec.vec(); }
    T*			arr()		{ return size() ? &(*this)[0] : 0; }
    const T*		arr() const	{ return size() ? &(*this)[0] : 0; }

protected:

    int			getPos( const T& ) const;
    			/*!< If not found, it will return position of the
			     item just above, and size() if val is higher than
			     highest val
			 */

    bool		allowmultiples;
    VectorAccess<T,int>	tvec;

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
	if ( stop-start==1 )
	    return typ>startval ? stop : start;

	int middle = (start+stop)>>1;
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
    if ( !size() ) return -1;

    int pos = getPos( typ );

    if ( pos>=size() || pos<0 || (*this)[pos]!=typ )
        return -1;

    return pos;
}


template <class T> inline
SortedList<T>&	SortedList<T>::operator +=( const T& nv )
{
    int newpos = getPos( nv );

    if ( newpos == size() )
    {
	tvec.push_back( nv );
	return *this;
    }

    if ( !allowmultiples && (*this)[newpos] == nv )
	return *this;

    tvec.insert( newpos, nv );
    return *this;
}


template <class T> inline
SortedList<T>&	SortedList<T>::operator -=( const T& nv )
{
    int sz = size();
    if ( !sz ) return *this;

    int pos = indexOf( nv );

    if ( pos == -1 ) return *this;

    tvec.remove( pos );
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
{ return copy(array); }


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
void SortedList<T>::erase() { tvec.erase(); }


template <class T> inline
void  SortedList<T>::remove( int pos )
{
    tvec.remove( pos );
}

#endif
