#ifndef sets_H
#define sets_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		April 1995
 Contents:	Sets of simple objects
 RCS:		$Id: sets.h,v 1.17 2002-03-29 17:28:24 bert Exp $
________________________________________________________________________

-*/

#ifndef gendefs_H
#include <gendefs.h>
#endif
#ifndef Vector_H
#include <Vector.h>
#endif

/*!\brief Set of objects

The TypeSet is meant for simple types or small objects that have a copy
constructor. The `-=' function will only remove the first occurrence that
matches using the `==' operator. The requirement of the presence of that
operator is actually not that bad: at least you can't forget it.

Do not make TypeSet<bool>. Use the BoolTypeSet typedef. See Vector for details.

*/

template <class T>
class TypeSet
{
public:
			TypeSet() {}

			TypeSet( int nr, T typ )
			{
			    for ( int idx=0; idx<nr; idx++ )
				typs.push_back(typ);
			}
			TypeSet( const TypeSet<T>& t )
				{ append( t ); }
    virtual		~TypeSet()
				{}
    TypeSet<T>&		operator =( const TypeSet<T>& ts ) { return copy(ts); }

    virtual int		size() const
				{ return typs.size(); }
    virtual T&		operator[]( int idx ) const
				{ return (T&)typs[idx]; }
    virtual T*		get( const T& t ) const
				{ return indexOf(t) < 0 ? 0 : (T*)&t; }
    virtual int		indexOf( const T& typ ) const {

			    const unsigned int sz = size();
			    for ( unsigned int idx=0; idx<sz; idx++ )
				if ( typs[idx] == typ ) return idx;
			    return -1;
			}

    TypeSet<T>&	operator +=( const T& typ )
				{ typs.push_back(typ); return *this; }
    TypeSet<T>&	operator -=( const T& typ )
				{ typs.erase(typ); return *this; }
    virtual TypeSet<T>&	copy( const TypeSet<T>& ts ) {

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
    virtual void	append( const TypeSet<T>& ts ) {

			    const unsigned int sz = ts.size();
			    for ( unsigned int idx=0; idx<sz; idx++ )
				*this += ts[idx];
			}

    virtual void	erase()				{ typs.erase(); }
    virtual void	remove( int idx )		{ typs.remove(idx); }
    virtual void	remove( int i1, int i2 )	{ typs.remove(i1,i2); }
    virtual void	insert( int idx, const T& typ )	{ typs.insert(idx,typ);}

    Vector<T>&		vec()				{ return typs.vec(); }
    const Vector<T>&	vec() const			{ return typs.vec(); }
    T*			arr()		{ return size() ? &(*this)[0] : 0; }
    const T*		arr() const	{ return size() ? &(*this)[0] : 0; }

private:

    Vector<T>		typs;

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


/*!\brief Set of pointers to objects

The ObjectSet does not manage the objects, it is just a collection of
pointers to the the objects.

*/

template <class T>
class ObjectSet
{
public:
    			ObjectSet() : allow0(false) {}
    			ObjectSet( const ObjectSet<T>& t )
						{ *this = t; }
    virtual		~ObjectSet()		{}
    ObjectSet<T>&	operator =( const ObjectSet<T>& os )
			{ allow0 = os.allow0; copy(os); return *this; }

    void		allowNull(bool yn_=true){ allow0 = yn_; }
    bool		nullAllowed() const	{ return allow0; }

    virtual int		size() const
				{ return objs.size(); }
    virtual T*		operator[]( int idx ) const
				{ return (T*)(objs[idx]); }
    virtual T*		operator[]( const T* t ) const {

			    int idx = indexOf(t);
			    return idx < 0 ? 0 : (T*)t;
			}
    virtual int		indexOf( const T* ptr ) const {

			    for ( int idx=0; idx<size(); idx++ )
				if ( (T*)objs[idx] == ptr ) return idx;
			    return -1;
			}
    virtual ObjectSet<T>& operator +=( T* ptr ) {

				if ( ptr || allow0 ) objs.push_back((void*)ptr);
				return *this;

			}
    virtual ObjectSet<T>& operator -=( T* ptr ) {

			    if ( ptr || allow0 ) objs.erase((void*)ptr);
			    return *this;
			}
    virtual T*		replace( T* newptr, int idx ) {
 
			    if (idx<0||idx>size()) return 0;
			    T* ptr = (T*)objs[idx];
			    objs[idx] = (void*)newptr; return ptr;
			}

    virtual void	insertAt( T* newptr, int idx )
			{
			    objs.insert( idx, (void*) newptr );
			}

    virtual void	insertAfter( T* newptr, int idx )
			{
			    *this += newptr;
			    if ( idx < 0 )
				objs.moveToStart( (void*)newptr );
			    else
				objs.moveAfter( (void*)newptr, objs[idx] );
			}
    virtual void	copy( const ObjectSet<T>& os ) {

			    if ( &os != this )
				{ erase(); allow0 = os.allow0; append( os ); }
			}
    virtual void	append( const ObjectSet<T>& os ) {

			    int sz = os.size();
			    for ( int idx=0; idx<sz; idx++ )
				*this += os[idx];
			}

    virtual void	erase()			{ objs.erase(); }
    virtual void	remove( int idx )	{ objs.remove(idx); }
    virtual void	remove( int i1, int i2 ) { objs.remove(i1,i2); }

private:

    Vector<void*>	objs;
    bool		allow0;

};


//! empty the ObjectSet deleting all objects pointed to.
template <class T>
inline void deepErase( ObjectSet<T>& os )
{
    for ( int sz=os.size(), idx=0; idx<sz; idx++ )
	delete os[idx];
    os.erase();
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


#endif
