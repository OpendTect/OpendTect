#ifndef sets_H
#define sets_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		April 1995
 Contents:	Sets of simple objects
 RCS:		$Id: sets.h,v 1.11 2001-06-29 20:17:42 bert Exp $
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
    TypeSet<T>&		operator =( const TypeSet<T>& ts ) { return copy(ts); }
    virtual void	append( const TypeSet<T>& ts ) {

			    const unsigned int sz = ts.size();
			    for ( unsigned int idx=0; idx<sz; idx++ )
				*this += ts[idx];
			}

    virtual void	erase()
				{ typs.erase(); }
    virtual void	remove( int idx )
				{ typs.remove(idx); }
    virtual void	remove( int i1, int i2 )
				{ typs.remove(i1,i2); }
private:

    Vector<T>		typs;

};


template <class T>
inline bool operator ==( const TypeSet<T>& a, const TypeSet<T>& b )
{
    if ( a.size() != b.size() ) return false;

    int sz = a.size();
    for ( int idx=0; idx<sz; idx++ )
	if ( !(a[idx] == b[idx]) ) return false;

    return true;
}

template <class T>
inline bool operator !=( const TypeSet<T>& a, const TypeSet<T>& b )
{ return !(a == b); }


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

    void		allowNull(bool yn=true)	{ allow0 = yn; }
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
    virtual void	insertAfter( T* newptr, int idx )
			{
			    *this += newptr;
			    if ( idx < 0 )
				objs.moveToStart( (void*)newptr );
			    else
				objs.moveAfter( (void*)newptr,
						(void*)(*this)[idx] );
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


template <class T>
inline void deepErase( ObjectSet<T>& os )
{
    for ( int sz=os.size(), idx=0; idx<sz; idx++ )
	delete os[idx];
    os.erase();
}

template <class T>
inline void deepAppend( ObjectSet<T>& to, const ObjectSet<T>& from )
{
    const int sz = from.size();
    for ( int idx=0; idx<sz; idx++ )
	to += from[idx] ? new T( *from[idx] ) : 0;
}

template <class T>
inline void deepCopy( ObjectSet<T>& to, const ObjectSet<T>& from )
{
    if ( &to != &from )
    {
	deepErase(to);
	to.allowNull(from.nullAllowed());
	deepAppend( to, from );
    }
}


#endif
