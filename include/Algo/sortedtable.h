#ifndef sortedtable_h
#define sortedtable_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		19-4-2000
 Contents:	Array sorting
 RCS:		$Id$
________________________________________________________________________

-*/

#include "sets.h"
#include "sortedlist.h"


/*!
  \ingroup Algo
  \brief A SortedTable keeps track of ids and their corresponding values. Each
  id can only be present once.
*/

template <class IDT, class T>
class SortedTable
{
public:
    			SortedTable();

    int 		size() const { return vals_.size(); }
    void		set( IDT id, T val );
    			/*<! If id is set twice, it the old value will
			     be replaced by the new one */
    bool		get( IDT id, T& val ) const;
    			/*!< If id is not found, val is unchanged and
			     false is returned. If id is found, val is set
			     and true is returned.
			*/

    const IDT&		id(int idx) const { return ids_[idx]; }
    const T&		val(int idx) const { return vals_[idx]; }

    bool		remove(IDT id);
    void		erase()  { vals_.erase(); ids_.erase(); }

private:
    TypeSet<T>		vals_;
    SortedList<IDT>	ids_;
};


template <class IDT, class T> inline
SortedTable<IDT,T>::SortedTable()
    : ids_( false )
{}


template <class IDT, class T> inline
void SortedTable<IDT,T>::set( IDT theid, T theval )
{
    int newpos = ids_.indexOf( theid );

    if ( newpos==-1 )
    {
	ids_ += theid;

	newpos = ids_.indexOf( theid );
	vals_.insert( newpos, theval );
    }

    vals_[newpos] = theval;
}


template <class IDT, class T> inline
bool SortedTable<IDT,T>::get( IDT theid, T& v ) const
{
    int pos = ids_.indexOf( theid );

    if ( pos==-1 )
	return false;

    v = vals_[pos];
    return true;
}


template <class IDT, class T> inline
bool  SortedTable<IDT,T>::remove(IDT theid)
{
    int pos = ids_.indexOf( theid );

    if ( pos==-1 ) return false;

    vals_.removeSingle( pos );
    ids_.remove( pos );

    return true;
}


/*!
  \ingroup Algo
  \brief A SortedPointers keeps track of ids and their corresponding pointer.
  Each id can only be present once.
*/

template <class T>
class SortedPointers
{
public:
    			SortedPointers( );
			~SortedPointers();

    int 		size() const { return vals.size(); }
    void		set( int id, T* val );
    			/*<! If id is set twice, it the old value will
			     be replaced by the new one 
			*/
    const T*		get( int id ) const;
    T*			get( int id );

    const T*		getByPos( int pos ) const { return vals[pos]; }
    T*			getByPos( int pos ) { return vals[pos]; }


    int			id( int pos ) const { return ids[pos]; }

    bool		remove( int id );
    bool		removePos( int pos );
    void		erase();

private:
    ObjectSet<T>	vals;

    SortedList<int>	ids;
};


template <class T> inline
SortedPointers<T>::SortedPointers()
    : ids( false )
{}


template <class T> inline
SortedPointers<T>::~SortedPointers()
{ } 


template <class T> inline
void	SortedPointers<T>::set( int id_, T* val )
{
    int newpos = ids.indexOf( id_ );

    if ( newpos==-1 )
    {
	ids += id_;

	newpos = ids.indexOf( id_ );
	vals.insertAt( val, newpos );
    }

    vals.replace( newpos, val );
}


template <class T> inline
const T* SortedPointers<T>::get( int id_ ) const
{
    int pos = ids.indexOf( id_ );

    if ( pos==-1 )
	return 0;

    return vals[pos];
}


template <class T> inline
T* SortedPointers<T>::get( int id_ )
{
    int pos = ids.indexOf( id_ );

    if ( pos==-1 )
	return 0;

    return vals[pos];
}


template <class T> inline
bool  SortedPointers<T>::remove(int id_)
{
    int pos = ids.indexOf( id_ );

    if ( pos==-1 ) return false;

    vals.removeSingle( pos );
    ids.remove( pos );

    return true;
}

template <class T> inline
void  SortedPointers<T>::erase()
{
    vals.erase();

    ids.erase();
}

#endif
