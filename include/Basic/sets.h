#ifndef sets_H
#define sets_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		April 1995
 Contents:	Sets of simple objects
 RCS:		$Id: sets.h,v 1.29 2004-04-27 15:51:15 bert Exp $
________________________________________________________________________

-*/

#ifndef gendefs_H
#include <gendefs.h>
#endif
#ifndef vectoraccess_h
#include <vectoraccess.h>
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
			TypeSet() {}

			TypeSet( int nr, T typ )
			{
			    for ( int idx=0; idx<nr; idx++ )
				tvec.push_back(typ);
			}
			TypeSet( const TypeSet<T>& t )
				{ append( t ); }
    virtual		~TypeSet()
				{}
    TypeSet<T>&		operator =( const TypeSet<T>& ts )
				{ return copy(ts); }

    virtual int		size() const
				{ return tvec.size(); }
    virtual T&		operator[]( int idx ) const
				{ return (T&)tvec[idx]; }

    virtual int		indexOf( const T& typ ) const
			{
			    const unsigned int sz = size();
			    for ( unsigned int idx=0; idx<sz; idx++ )
				if ( tvec[idx] == typ ) return idx;
			    return -1;
			}

    TypeSet<T>&	operator +=( const T& typ )
				{ tvec.push_back(typ); return *this; }
    TypeSet<T>&	operator -=( const T& typ )
				{ tvec.erase(typ); return *this; }
    virtual TypeSet<T>&	copy( const TypeSet<T>& ts )
			{
			    if ( &ts != this )
			    {
				const unsigned int sz = size();
				if ( sz != ts.size() )
				    { erase(); append(ts); }
				else
				    for ( unsigned int idx=0; idx<sz; idx++ )
					(*this)[idx] = ts[idx];
			    }
			    return *this;
			}
    virtual void	append( const TypeSet<T>& ts )
			{
			    const unsigned int sz = ts.size();
			    for ( unsigned int idx=0; idx<sz; idx++ )
				*this += ts[idx];
			}
    virtual void	fill( const T& t )		{ tvec.fill(t); }

    virtual void	erase()				{ tvec.erase(); }
    virtual void	remove( int idx )		{ tvec.remove(idx); }
    virtual inline void	removeFast( int idx );
    			/*!<Moves the last item to the position of the removed
			  	item, and removes the last item, thus avoiding
				the memmove.
			*/
    virtual void	remove( int i1, int i2 )	{ tvec.remove(i1,i2); }
    virtual void	insert( int idx, const T& typ )	{ tvec.insert(idx,typ);}

			//! 3rd party access
    std::vector<T>&	  vec()		{ return tvec.vec(); }
    const std::vector<T>& vec() const	{ return tvec.vec(); }
    T*			arr()		{ return size() ? &(*this)[0] : 0; }
    const T*		arr() const	{ return size() ? &(*this)[0] : 0; }

protected:

    VectorAccess<T>	tvec;

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

template <class T>
inline void TypeSet<T>::removeFast(int idx)
{
    const int last = size()-1;
    if ( idx!=last )
	tvec[idx]=tvec[last];
    tvec.remove(last);
}

/*!\brief Set of pointers to objects

The ObjectSet does not manage the objects, it is just a collection of
pointers to the the objects.

*/

template <class T>
class ObjectSet
{
public:
    			ObjectSet() : allow0(false)
				{}
    			ObjectSet( const ObjectSet<T>& t )
				{ *this = t; }
    virtual		~ObjectSet()
				{}
    ObjectSet<T>&	operator =( const ObjectSet<T>& os )
				{ allow0 = os.allow0; copy(os); return *this; }

    void		allowNull( bool yn_=true )
				{ allow0 = yn_; }
    bool		nullAllowed() const
				{ return allow0; }

    virtual int		size() const
				{ return ovec.size(); }
    virtual T*		operator[]( int idx ) const
				{ return (T*)(ovec[idx]); }
    virtual T*		operator[]( const T* t ) const
			{
			    int idx = indexOf(t);
			    return idx < 0 ? 0 : (T*)t;
			}
    virtual int		indexOf( const T* ptr ) const
			{

			    for ( int idx=0; idx<size(); idx++ )
				if ( (T*)ovec[idx] == ptr ) return idx;
			    return -1;
			}
    virtual ObjectSet<T>& operator +=( T* ptr )
			{
				if ( ptr || allow0 ) ovec.push_back((void*)ptr);
				return *this;

			}
    virtual ObjectSet<T>& operator -=( T* ptr )
			{
			    if ( ptr || allow0 ) ovec.erase((void*)ptr);
			    return *this;
			}
    virtual T*		replace( T* newptr, int idx )
			{
			    if (idx<0||idx>size()) return 0;
			    T* ptr = (T*)ovec[idx];
			    ovec[idx] = (void*)newptr; return ptr;
			}
    virtual void	insertAt( T* newptr, int idx )
			{
			    ovec.insert( idx, (void*) newptr );
			}
    virtual void	insertAfter( T* newptr, int idx )
			{
			    *this += newptr;
			    if ( idx < 0 )
				ovec.moveToStart( (void*)newptr );
			    else
				ovec.moveAfter( (void*)newptr, ovec[idx] );
			}
    virtual void	copy( const ObjectSet<T>& os )
			{
			    if ( &os != this )
				{ erase(); allow0 = os.allow0; append( os ); }
			}
    virtual void	append( const ObjectSet<T>& os )
			{
			    const int sz = os.size();
			    for ( int idx=0; idx<sz; idx++ )
				*this += os[idx];
			}

    virtual void	erase()				{ e_rase(); }
    virtual void	remove( int idx )		{ ovec.remove(idx); }
    virtual inline void	removeFast( int idx );
    			/*!<Moves the last pointer to the position of the
			  	removed pointer, and removes the last item,
				thus avoiding the memmove.
			*/
    virtual void	remove( int i1, int i2 )	{ ovec.remove(i1,i2); }


    void		e_rase()			{ ovec.erase(); }
			//!< Not virtual. Only use if you know what you're doing

protected:

    VectorAccess<void*>	ovec;
    bool		allow0;

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
	    {
		tmp = os.replace( os[j+d], j );
		os.replace( tmp, j+d );
	    }
}

template <class T>
inline void ObjectSet<T>::removeFast(int idx)
{
    const int last = size()-1;
    if ( idx!=last )
	ovec[idx]=ovec[last];
    ovec.remove(last);
}

#endif
