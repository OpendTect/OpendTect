#ifndef sets_h
#define sets_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		April 1995
 Contents:	Sets of simple objects
 RCS:		$Id: sets.h,v 1.50 2008-09-12 14:12:33 cvshelene Exp $
________________________________________________________________________

-*/

#ifndef gendefs_H
#include "gendefs.h"
#endif
#ifndef vectoraccess_h
#include "vectoraccess.h"
#endif

/*!\brief Set of (small) copyable elements

The TypeSet is meant for simple types or small objects that have a copy
constructor. The `-=' function will only remove the first occurrence that
matches using the `==' operator. The requirement of the presence of that
operator is actually not that bad: at least you can't forget it.

Do not make TypeSet<bool>. Use the BoolTypeSet typedef. See vectoraccess.h for
details.

*/

template <class T>
class TypeSet
{
public:
    inline			TypeSet();
    inline			TypeSet( int nr, T typ );
    inline			TypeSet( const TypeSet<T>& t );
    inline virtual		~TypeSet();
    inline TypeSet<T>&		operator =( const TypeSet<T>& ts );

    inline virtual int		size() const;
    inline virtual void		setSize( int sz, T val=T() );
				/*!<\param val value assigned to added items
					   if size is increased. */
    inline virtual void		setCapacity( int sz );
				/*!<Allocates mem for sz,
				    does not change size.*/

    inline void			swap( int idx0, int idx1 );

    inline virtual bool		isEmpty() const;
    inline virtual bool		validIdx( int idx ) const;

    inline virtual T&		operator[]( int idx );
    inline virtual const T&	operator[]( int idx ) const;

    inline virtual int		indexOf( const T& typ, bool forward=true,
	    				 int start=-1 ) const;
    inline TypeSet<T>&		operator +=( const T& typ );
    inline TypeSet<T>&		operator -=( const T& typ );

    inline virtual TypeSet<T>&	copy( const TypeSet<T>& ts );
    inline virtual void		append( const TypeSet<T>& ts );

    virtual inline void		createUnion( const TypeSet<T>& ts );
				/*!<Adds all items in ts if they are not there
				    allready. */
    virtual inline void		createIntersection( const TypeSet<T>& ts );
				//!<Removes all items that are not present in ts
    virtual inline void		createDifference( const TypeSet<T>& ts );
				//!<Removes all items that are present in ts.
    				//!<Preserves order: uses remove, not removeFast

    inline virtual bool		addIfNew( const T& typ );
    virtual void		fill( const T& t );

    inline virtual void		erase();
    inline virtual void		remove( int idx );
    virtual inline void		removeFast( int idx );
				/*!<Moves the last item to the position of the
				    removed item, and removes the last item,
				    thus avoiding the memmove.  */
    inline virtual void		remove( int i1, int i2 );
    inline virtual void		insert( int idx, const T& typ );

				//! 3rd party access
    inline std::vector<T>&	vec();
    inline const std::vector<T>& vec() const;
    inline T*			arr();
    inline const T*		arr() const;

protected:

    VectorAccess<T>	tvec_;

};

//! We need this because STL has a crazy specialisation of the vector<bool>
typedef char BoolTypeSetType;
typedef TypeSet<BoolTypeSetType> BoolTypeSet;
//!< This sux, BTW.



template <class T>
inline bool operator ==( const TypeSet<T>& a, const TypeSet<T>& b )
{
    if ( a.size() != b.size() ) return false;

    const int sz = a.size();
    for ( int idx=0; idx<sz; idx++ )
	if ( !(a[idx] == b[idx]) ) return false;

    return true;
}

template <class T>
inline bool operator !=( const TypeSet<T>& a, const TypeSet<T>& b )
{ return !(a == b); }


//! append allowing a different type to be merged into set
template <class T,class S>
inline void append( TypeSet<T>& to, const TypeSet<S>& from )
{
    const int sz = from.size();
    to.setCapacity( sz + to.size() );
    for ( int idx=0; idx<sz; idx++ )
	to += from[idx];
}


//! copy from different possibly different type into set
//! Note that there is no optimisation for equal size, as in member function.
template <class T,class S>
inline void copy( TypeSet<T>& to, const TypeSet<S>& from )
{
    if ( &to == &from ) return;
    to.erase();
    append( to, from );
}


//! Sort TypeSet. Must have operator > defined for elements
template <class T>
inline void sort( TypeSet<T>& ts )
{
    T tmp; const int sz = ts.size();
    for ( int d=sz/2; d>0; d=d/2 )
	for ( int i=d; i<sz; i++ )
	    for ( int j=i-d; j>=0 && ts[j]>ts[j+d]; j-=d )
		{ tmp = ts[j]; ts[j] = ts[j+d]; ts[j+d] = tmp; }
}



/*!\brief Set of pointers to objects

The ObjectSet does not manage the objects, it is just a collection of
pointers to the the objects.

*/

template <class T>
class ObjectSet
{
public:
    inline 			ObjectSet();
    inline			ObjectSet( const ObjectSet<T>& t );
    inline virtual		~ObjectSet();
    inline ObjectSet<T>&	operator =( const ObjectSet<T>& os );

    inline void			allowNull( bool yn_=true );
    inline bool			nullAllowed() const;
    inline virtual int		size() const;

    inline virtual bool		isEmpty() const;
    inline virtual bool		validIdx( int idx ) const;

    inline virtual T*		operator[]( int idx );
    inline virtual const T*	operator[]( int idx ) const;
    inline virtual T*		operator[]( const T* t ) const;
    				//!< check + de-const;
    inline virtual int		indexOf( const T* ptr ) const;
    inline virtual ObjectSet<T>& operator +=( T* ptr );
    inline virtual ObjectSet<T>& operator -=( T* ptr );
    inline void			swap( int idx0, int idx1 );
    inline virtual T*		replace( int idx, T* newptr );
    inline virtual void		insertAt( T* newptr, int idx );
    inline virtual void		insertAfter( T* newptr, int idx );
    inline virtual void		copy( const ObjectSet<T>& os );
    inline virtual void		append( const ObjectSet<T>& os );

    inline virtual void		push(T* ptr);
    inline virtual T*		pop();

    inline virtual void		erase();
    virtual inline T*		remove( int idx );;
    				/*!<\returns the removed pointer. */
    virtual inline T*		removeFast( int idx );
				/*!<Moves the last pointer to the position of
				    the removed pointer, and removes the last
				    item, thus avoiding the memmove.
				    \returns the removed pointer.  */
    inline virtual void		remove( int i1, int i2 );

    inline void			e_rase();
				/*!< Not virtual. Only use if you know what
				     you're doing. */

protected:

    VectorAccess<void*>	ovec_;
    bool		allow0_;
};


//! empty the ObjectSet deleting all objects pointed to.
template <class T>
inline void deepErase( ObjectSet<T>& os )
{
    for ( int sz=os.size(), idx=0; idx<sz; idx++ )
	delete os[idx];
    os.e_rase(); // not os.erase() : it may be overloaded
}


//! empty the ObjectSet deleting all objects pointed to.
template <class T>
inline void deepEraseArr( ObjectSet<T>& os )
{
    for ( int sz=os.size(), idx=0; idx<sz; idx++ )
	delete [] os[idx];
    os.e_rase(); // not os.erase() : it may be overloaded
}


//! append copies of one set's objects to another ObjectSet.
template <class T,class S>
inline void deepAppend( ObjectSet<T>& to, const ObjectSet<S>& from )
{
    const int sz = from.size();
    for ( int idx=0; idx<sz; idx++ )
	to += from[idx] ? new T( *from[idx] ) : 0;
}


//! fill an ObjectSet with copies of the objects in the other set.
template <class T,class S>
inline void deepCopy( ObjectSet<T>& to, const ObjectSet<S>& from )
{
    if ( &to == &from ) return;
    deepErase(to);
    to.allowNull(from.nullAllowed());
    deepAppend( to, from );
}


//! Locate object in set
template <class T,class S>
inline int indexOf( const ObjectSet<T>& os, const S& val )
{
    for ( int idx=0; idx<os.size(); idx++ )
    {
	if ( *os[idx] == val )
	    return idx;
    }
    return -1;
}


//! Get const object in set
template <class T,class S>
inline const T* find( const ObjectSet<T>& os, const S& val )
{
    int idx = indexOf( os, val );
    return idx == -1 ? 0 : os[idx];
}


//! Get object in set
template <class T,class S>
inline T* find( ObjectSet<T>& os, const S& val )
{
    int idx = indexOf( os, val );
    return idx == -1 ? 0 : os[idx];
}


//! Sort ObjectSet. Must have operator > defined for elements
template <class T>
inline void sort( ObjectSet<T>& os )
{
    T* tmp; const int sz = os.size();
    for ( int d=sz/2; d>0; d=d/2 )
	for ( int i=d; i<sz; i++ )
	    for ( int j=i-d; j>=0 && *os[j]>*os[j+d]; j-=d )
		os.swap( j, j+d );
}

//TypeSet implementations
template <class T> inline
TypeSet<T>::TypeSet()
{}


template <class T> inline
TypeSet<T>::TypeSet( int nr, T typ )
{ setSize( nr, typ ); }


template <class T> inline
TypeSet<T>::TypeSet( const TypeSet<T>& t )
{ append( t ); }


template <class T> inline
TypeSet<T>::~TypeSet() {}


template <class T> inline
TypeSet<T>& TypeSet<T>::operator =( const TypeSet<T>& ts )
{ return copy(ts); }

template <class T> inline
int TypeSet<T>::size() const
{ return tvec_.size(); }


template <class T> inline
void TypeSet<T>::setSize( int sz, T val )
{ tvec_.setSize(sz,val); }


template <class T> inline
void TypeSet<T>::setCapacity( int sz )
{ tvec_.setCapacity( sz ); }


template <class T> inline
void TypeSet<T>::swap( int idx0, int idx1 )
{
    if ( !validIdx(idx0) || !validIdx(idx1) )
	return;

    T tmp = tvec_[idx0];
    tvec_[idx0] = tvec_[idx1];
    tvec_[idx1] = tmp;
}


template <class T> inline
bool TypeSet<T>::isEmpty() const
{ return tvec_.size() == 0; }


template <class T> inline
bool TypeSet<T>::validIdx( int idx ) const
{ return idx>=0 && idx<size(); }


template <class T> inline
T& TypeSet<T>::operator[]( int idx )
{ return tvec_[idx]; }


template <class T> inline
const T& TypeSet<T>::operator[]( int idx ) const
{ return tvec_[idx]; }


template <class T> inline
int TypeSet<T>::indexOf( const T& typ, bool forward, int start ) const
{
    const T* ptr = arr();
    if ( forward )
    {
	const unsigned int sz = size();
	if ( start<0 || start>=sz ) start = 0;
	for ( unsigned int idx=start; idx<sz; idx++ )
	    if ( ptr[idx] == typ ) return idx;
    }
    else
    {
	const unsigned int sz = size();
	if ( start<0 || start>=sz ) start = sz-1;
	for ( int idx=start; idx>=0; idx-- )
	    if ( ptr[idx] == typ ) return idx;
    }

    return -1;
}


template <class T> inline
TypeSet<T>& TypeSet<T>::operator +=( const T& typ )
{ tvec_.push_back(typ); return *this; }


template <class T> inline
TypeSet<T>& TypeSet<T>::operator -=( const T& typ )
{ tvec_.erase(typ); return *this; }


template <class T> inline
TypeSet<T>& TypeSet<T>::copy( const TypeSet<T>& ts )
{
    if ( &ts != this )
    {
	const unsigned int sz = size();
	if ( sz != ts.size() )
	    { erase(); append(ts); }
	else
	{
	    for ( unsigned int idx=0; idx<sz; idx++ )
		(*this)[idx] = ts[idx];
	}
    }

    return *this;
}


template <class T> inline
void TypeSet<T>::append( const TypeSet<T>& ts )
{
    const unsigned int sz = ts.size();
    if ( !sz ) return;

    setCapacity( sz+size() );

    for ( unsigned int idx=0; idx<sz; idx++ )
	*this += ts[idx];
}


template <class T>
inline void TypeSet<T>::createUnion( const TypeSet<T>& ts )
{
    const unsigned int sz = ts.size();
    const T* ptr = ts.arr();
    for ( unsigned int idx=0; idx<sz; idx++, ptr++ )
	addIfNew(*ptr);
}


template <class T>
inline void TypeSet<T>::createIntersection( const TypeSet<T>& ts )
{
    for ( unsigned int idx=0; idx<size(); idx++ )
    {
	if ( ts.indexOf((*this)[idx])!=-1 )
	    continue;
	removeFast( idx-- );
    }
}


template <class T>
inline void TypeSet<T>::createDifference( const TypeSet<T>& ts )
{
    const unsigned int sz = ts.size();
    for ( unsigned int idx=0; idx<sz; idx++ )
    {
	const T& typ = ts[idx];
	for ( int idy=0; idy<size(); idy++ )
	{
	    if ( tvec_[idy]==typ )
		remove(idy--);
	}
    }
}


template <class T>
inline void TypeSet<T>::removeFast( int idx )
{
    const int last = size()-1;
    if ( idx!=last )
	tvec_[idx]=tvec_[last];
    tvec_.remove(last);
}


template <class T> inline
bool TypeSet<T>::addIfNew( const T& typ )
{
    if ( indexOf(typ) < 0 ) { *this += typ; return true; }
    return false;
}


template <class T> inline
void TypeSet<T>::fill( const T& t )
{ tvec_.fill(t); }


template <class T> inline
void TypeSet<T>::erase()
{ tvec_.erase(); }


template <class T> inline
void TypeSet<T>::remove( int idx )
{ tvec_.remove(idx); }


template <class T> inline
void TypeSet<T>::remove( int i1, int i2 )
{ tvec_.remove(i1,i2); }


template <class T> inline
void TypeSet<T>::insert( int idx, const T& typ )
{ tvec_.insert(idx,typ);}


template <class T> inline
std::vector<T>&	 TypeSet<T>::vec()
{ return tvec_.vec(); }


template <class T> inline
const std::vector<T>& TypeSet<T>::vec() const
{ return tvec_.vec(); }


template <class T> inline
T* TypeSet<T>::arr()
{ return size() ? &(*this)[0] : 0; }


template <class T> inline
const T* TypeSet<T>::arr() const
{ return size() ? &(*this)[0] : 0; }


//ObjectSet implementation
template <class T> inline
ObjectSet<T>::ObjectSet() : allow0_(false)
{}


template <class T> inline
ObjectSet<T>::ObjectSet( const ObjectSet<T>& t )
{ *this = t; }


template <class T> inline
ObjectSet<T>::~ObjectSet()
{}


template <class T> inline
ObjectSet<T>& ObjectSet<T>::operator =( const ObjectSet<T>& os )
{ allow0_ = os.allow0_; copy(os); return *this; }


template <class T> inline
void ObjectSet<T>::allowNull( bool yn )
{ allow0_ = yn; }


template <class T> inline
bool ObjectSet<T>::nullAllowed() const
{ return allow0_; }


template <class T> inline
int ObjectSet<T>::size() const
{ return ovec_.size(); }


template <class T> inline
bool ObjectSet<T>::isEmpty() const
{ return ovec_.size() == 0; }


template <class T> inline
bool ObjectSet<T>::validIdx( int idx ) const
{ return idx>=0 && idx<size(); }


template <class T> inline
T* ObjectSet<T>::operator[]( int idx )
{ return (T*)ovec_[idx]; }


template <class T> inline
const T* ObjectSet<T>::operator[]( int idx ) const
{ return (const T*)ovec_[idx]; }


template <class T> inline
T* ObjectSet<T>::operator[]( const T* t ) const
{
    int idx = indexOf(t);
    return idx < 0 ? 0 : const_cast<T*>(t);
}


template <class T> inline
int ObjectSet<T>::indexOf( const T* ptr ) const
{
    for ( int idx=0; idx<size(); idx++ )
	if ( (const T*)ovec_[idx] == ptr ) return idx;
	    return -1;
}


template <class T> inline
ObjectSet<T>& ObjectSet<T>::operator +=( T* ptr )
{
    if ( ptr || allow0_ ) ovec_.push_back((void*)ptr);
    return *this;
}


template <class T> inline
ObjectSet<T>& ObjectSet<T>::operator -=( T* ptr )
{
    if ( ptr || allow0_ ) ovec_.erase((void*)ptr);
    return *this;
}


template <class T> inline
void ObjectSet<T>::swap( int idx0, int idx1 )
{
    if ( idx0<0||idx0>=size()||idx1<0||idx1>=size() )
	return;
    void* tmp = ovec_[idx0];
    ovec_[idx0] = ovec_[idx1];
    ovec_[idx1] = tmp;
}


template <class T> inline
T* ObjectSet<T>::replace( int idx, T* newptr )
{
    if (idx<0||idx>=size()) return 0;
    T* ptr = (T*)ovec_[idx];
    ovec_[idx] = (void*)newptr; return ptr;
}


template <class T> inline
void ObjectSet<T>::insertAt( T* newptr, int idx )
{
    ovec_.insert( idx, (void*) newptr );
}


template <class T> inline
void ObjectSet<T>::insertAfter( T* newptr, int idx )
{
    *this += newptr;
    if ( idx < 0 )
	ovec_.moveToStart( (void*)newptr );
    else
	ovec_.moveAfter( (void*)newptr, ovec_[idx] );
}


template <class T> inline
void ObjectSet<T>::copy( const ObjectSet<T>& os )
{
    if ( &os != this )
    {
	erase();
	allow0_ = os.allow0_;
	append( os );
    }
}


template <class T> inline
void ObjectSet<T>::append( const ObjectSet<T>& os )
{
    const int sz = os.size();
    ovec_.setCapacity( size()+sz );
    for ( int idx=0; idx<sz; idx++ )
	*this += const_cast<T*>( os[idx] );
}


template <class T> inline
void ObjectSet<T>::push(T* ptr)
{ *this +=ptr; }


template <class T> inline
T* ObjectSet<T>::pop()
{
    int sz=size();
    if ( !sz ) return 0;
    return remove(sz-1);
}


template <class T> inline
void ObjectSet<T>::erase()
{ e_rase(); }


template <class T> inline
T* ObjectSet<T>::removeFast(int idx)
{
    T* res = (T*)ovec_[idx];
    const int last = size()-1;
    if ( idx!=last )
	ovec_[idx]=ovec_[last];
    ovec_.remove(last);
    return res;
}


template <class T> inline
T* ObjectSet<T>::remove(int idx)
{
    T* res = (T*)ovec_[idx];
    ovec_.remove(idx);
    return res;
}

template <class T> inline
void ObjectSet<T>::remove( int i1, int i2 )
{ ovec_.remove(i1,i2); }


template <class T> inline
void ObjectSet<T>::e_rase()
{ ovec_.erase(); }


#endif
