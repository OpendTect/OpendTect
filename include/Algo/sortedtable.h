#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "sets.h"
#include "sortedlist.h"


/*!
\brief A SortedTable keeps track of ids and their corresponding values. Each id can only be present once.
*/

template <class IDT, class T>
mClass(Algo) SortedTable
{
public:

    typedef int		size_type;
    typedef T		object_type;

    			SortedTable();

    size_type 		size() const { return vals_.size(); }
    void		set( IDT id, T val );
    			/*<! If id is set twice, it the old value will
			     be replaced by the new one */
    bool		get( IDT id, T& val ) const;
    			/*!< If id is not found, val is unchanged and
			     false is returned. If id is found, val is set
			     and true is returned.
			*/

    const IDT&		id( size_type idx ) const	{ return ids_[idx]; }
    const T&		val( size_type idx ) const	{ return vals_[idx]; }

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
\brief A SortedPointers keeps track of ids and their corresponding pointer.
Each id can only be present once.
*/

template <class T>
mClass(Algo) SortedPointers
{
public:

    typedef int		size_type;
    typedef int		id_type;

    			SortedPointers() : ids_(false)	{}
			~SortedPointers()		{}

    size_type 		size() const			{ return vals_.size(); }
    void		set(id_type,T*);
			    /*<! If id is set twice, it the old value will
				 be replaced by the new one */
    const T*		get(id_type) const;
    T*			get(id_type);

    const T*		getByPos( size_type pos ) const { return vals_[pos]; }
    T*			getByPos( size_type pos ) { return vals_[pos]; }


    id_type		id( size_type pos ) const { return ids_[pos]; }

    bool		remove(id_type id);
    void		removePos(size_type);
    void		erase();

private:

    ObjectSet<T>	vals_;
    SortedList<size_type> ids_;

};



template <class T> inline
void SortedPointers<T>::set( id_type iid, T* val )
{
    const size_type newpos = ids_.indexOf( iid );

    if ( newpos==-1 )
    {
	ids_ += iid;

	newpos = ids_.indexOf( iid );
	vals_.insertAt( val, newpos );
    }

    vals_.replace( newpos, val );
}


template <class T> inline
const T* SortedPointers<T>::get( id_type iid ) const
{
    const size_type pos = ids_.indexOf( iid );
    return pos < 0 ? 0 : vals_[pos];
}


template <class T> inline
T* SortedPointers<T>::get( id_type iid )
{
    const size_type pos = ids_.indexOf( iid );
    return pos < 0 ? 0 : vals_[pos];
}


template <class T> inline
bool SortedPointers<T>::remove( id_type iid )
{
    const size_type pos = ids_.indexOf( iid );
    if ( pos==-1 ) return false;

    vals_.removeSingle( pos );
    ids_.removeSingle( pos );

    return true;
}


template <class T> inline
void SortedPointers<T>::removePos( size_type pos )
{
    vals_.removeSingle( pos );
    ids_.removeSingle( pos );
}


template <class T> inline
void SortedPointers<T>::erase()
{
    vals_.erase();
    ids_.erase();
}
