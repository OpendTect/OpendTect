#ifndef sets_H
#define sets_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		April 1995
 Contents:	Sets of simple objects
 RCS:		$Id: sets.h,v 1.1.1.2 1999-09-16 09:19:15 arend Exp $
________________________________________________________________________

The TypeSet is meant for simple types or small objects that have a copy
constructor. The `-=' function will only remove the first occurrence that
matches with the `==' operator.
The ObjectSet does not manage the objects, it is just a collection of
pointers to the the objects.

@$*/

#ifndef gendefs_H
#include <gendefs.h>
#endif
#ifndef Vector_H
#include <Vector.h>
#endif


template <class T>
class TypeSet
{
public:
			TypeSet()
			{}
			TypeSet( int nr, T typ=0 ) {

				for ( int idx=0; idx<nr; idx++ )
				    typs.push_back(typ);
			}
    virtual		~TypeSet()	{}

    virtual int		size() const
				{ return typs.size(); }
    virtual T&		operator[]( int idx ) const
				{ return (T&)typs[idx]; }
    virtual T*		get( const T& t ) const
				{ return indexOf(t) < 0 ? 0 : (T*)&t; }
    virtual int		indexOf( const T& typ ) const {

				for ( int idx=0; idx<size(); idx++ )
				    if ( typs[idx] == typ ) return idx;
				return -1;
			}

    TypeSet<T>&	operator +=( const T& typ )
				{ typs.push_back(typ); return *this; }
    TypeSet<T>&	operator -=( const T& typ )
				{ typs.erase(typ); return *this; }
    TypeSet<T>&		operator =( const TypeSet<T>& ts ) {

				if ( &ts != this ) { erase(); append(ts); }
				return *this;

			}
    virtual void	append( const TypeSet<T>& ts ) {

				int sz = ts.size();
				for ( int idx=0; idx<sz; idx++ )
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
class ObjectSet
{
public:
    			ObjectSet() : allow0(NO) {}
    virtual		~ObjectSet()		 {}
    void		allowNull()		 { allow0 = YES; }
    ObjectSet<T>&	operator =( const ObjectSet<T>& os )
						 { copy(os); return *this; }
    virtual void	erase()			 { objs.erase(); }

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
				    if ( objs[idx] == ptr ) return idx;
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
    virtual void	copy( const ObjectSet<T>& os ) {

				if ( &os != this )
				    { erase(); append( os ); }

			}
    virtual void	append( const ObjectSet<T>& os ) {

				int sz = os.size();
				for ( int idx=0; idx<sz; idx++ )
				    *this += os[idx];

			}

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
inline void deepCopy( ObjectSet<T>& to, const ObjectSet<T>& from )
{
    if ( &to != &from )
	{ deepErase(to); deepAppend( to, from ); }
}

template <class T>
inline void deepAppend( ObjectSet<T>& to, const ObjectSet<T>& from )
{
    int sz = from.size();
    for ( int idx=0; idx<sz; idx++ )
	to += new T( *from[idx] );
}


/*$-*/
#endif
