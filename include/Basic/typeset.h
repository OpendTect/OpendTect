#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert / many others
 Date:		Apr 1995 / Feb 2009
RCS:		$Id:$
________________________________________________________________________

-*/

#include "odset.h"
#include "vectoraccess.h"


/*!\brief Base class for TypeSet, usually not used as such. */

namespace OD
{

template <class T, class IT>
mClass(Basic) ValVec : public Set
{
public:

    typedef IT			size_type;
    typedef size_type		idx_type;
    typedef T			object_type;

    ValVec*			clone() const override		= 0;
    inline virtual		~ValVec();
    inline ValVec&		operator =( const ValVec& oth )
				{ return copy( oth ); }
    inline bool			operator==(const ValVec&) const;
    inline bool			operator!=( const ValVec& oth ) const
				{ return !(*this == oth); }

    inline size_type		size() const;
    inline od_int64		nrItems() const override { return size(); }
    inline virtual bool		setSize(size_type sz,T val=T());
				/*!<\param sz number of new items
				  \param val value assigned to new items */
    inline virtual bool		setCapacity(size_type sz,bool withmargin);
				/*!<Allocates mem only, no size() change */
    inline virtual size_type	getCapacity() const;
    inline void			setAll(T);
    inline void			replace(T,T);

    inline T&			get(idx_type);
    inline const T&		get(idx_type) const;
    inline T&			first();
    inline const T&		first() const;
    inline T&			last();
    inline const T&		last() const;
    inline bool			validIdx(od_int64) const override;
    inline virtual idx_type	indexOf(T,bool forward=true,
					  idx_type start=-1) const;
    inline bool			isPresent(const T&) const;
    inline size_type		count(const T&) const;

    inline ValVec&		add(const T&);
    inline virtual void		insert(idx_type,const T&);
    inline bool			push(const T&);
    inline T			pop();
    inline virtual bool		append(const T*,size_type);
    inline virtual bool		append(const ValVec&);
    inline virtual bool		addIfNew(const T&);
    inline void			swap(IT,IT);
    inline virtual ValVec&	copy(const T*,size_type);
    inline virtual ValVec&	copy(const ValVec&);
    virtual inline void		createUnion(const ValVec&);
				/*!< Adds items not already there */
    virtual inline void		createIntersection(const ValVec&);
				//!< Only keeps common items
    virtual inline void		createDifference(const ValVec&,
					 bool must_preserve_order=false);
				//!< Removes all items present in other set.

    inline void			swapItems( od_int64 i1, od_int64 i2 ) override
				{ swap( (IT)i1, (IT)i2 ); }
    inline virtual void		move(idx_type from,idx_type to);
    inline virtual void		useIndexes(const idx_type*);

    inline void			reverse() override;

    inline void			erase() override;
    inline virtual void		removeSingle(idx_type,
					     bool preserver_order=true);
    inline virtual void		removeRange(idx_type from,idx_type to);

				//! 3rd party access
    inline virtual T*		arr()		{ return gtArr(); }
    inline virtual const T*	arr() const	{ return gtArr(); }
    inline std::vector<T>&	vec();
    inline const std::vector<T>& vec() const;

    inline T&			operator[](idx_type i)       { return get(i); }
    inline const T&		operator[](idx_type i) const { return get(i); }
    inline ValVec&		operator+=(const T& t)       { return add(t); }
    inline ValVec&		operator-=(const T& t);

protected:

    inline			ValVec();
    inline			ValVec(size_type nr,T typ);
    inline			ValVec(const T*,size_type nr);
    inline			ValVec(const ValVec&);

    typedef VectorAccess<T,IT>	impl_type;
    impl_type			vec_;

    inline T*			gtArr() const;

public:

    // Compat with std containers
    typedef T					value_type;
    typedef value_type&				reference;
    typedef const value_type&			const_reference;
    typedef typename impl_type::iterator	iterator;
    typedef typename impl_type::const_iterator	const_iterator;
    typedef size_type				difference_type;

    iterator			begin()		{ return vec_.begin(); }
    const_iterator		begin() const	{ return vec_.cbegin(); }
    const_iterator		cbegin() const	{ return vec_.cbegin(); }
    iterator			end()		{ return vec_.end(); }
    const_iterator		end() const	{ return vec_.cend(); }
    const_iterator		cend() const	{ return vec_.cend(); }
    inline size_type		max_size() const { return maxIdx32(); }
    inline bool			empty() const	{ return isEmpty(); }
    inline void			swap( ValVec& oth ) { vec_.swap(oth.vec_); }

    // Usability
    idx_type	getIdx( iterator it ) const	{ return vec_.getIdx(it); }
    idx_type	getIdx( const_iterator it ) const { return vec_.getIdx(it); }

};

} // namespace OD


#define mDefTypeSetClass( clss, idxtype ) \
template <class T> \
mClass(Basic) clss : public OD::ValVec<T,idxtype> \
{ \
public: \
 \
    typedef typename OD::ValVec<T,idxtype>::idx_type	idx_type; \
    typedef typename OD::ValVec<T,idxtype>::size_type	size_type; \
 \
		clss() \
		    : OD::ValVec<T,size_type>()		{} \
		clss( size_type nr, T typ ) \
		    : OD::ValVec<T,size_type>( nr, typ )	{} \
    explicit	clss( T typ ) \
		    : OD::ValVec<T,size_type>( 1, typ )	{} \
		clss( const T* t, size_type nr ) \
		    : OD::ValVec<T,size_type>( t, nr )		{} \
		clss( const clss& oth ) \
		    : OD::ValVec<T,size_type>( oth )		{} \
    clss*	clone() const override		{ return new clss(*this); } \
}; \
 \
template <class T> \
mGlobal(Basic) inline void swap( clss<T>& vv1, clss<T>& vv2 ) \
{ \
    vv1.swap( vv2 ); \
}


/*!\brief Sets of (small) copyable elements.

  TypeSet/LargeValVec are meant for simple types or small objects that have
  a copy constructor. The `-=' operator will only remove the first occurrence
  that matches using the `==' operator. The requirement of the presence of that
  operator is actually not that bad: at least you can't forget it.

  Do not make TypeSet<bool> (don't worry, it won't compile). Use the
  BoolTypeSet typedef just after the class definition. See vectoraccess.h for
  details on why.
*/

mDefTypeSetClass( TypeSet, od_int32 )
mDefTypeSetClass( LargeValVec, od_int64 )


/*!\brief Needed because the std lib has a crazy specialization vector<bool>. */

mClass(Basic) BoolTypeSetType
{
public:

			BoolTypeSetType( bool v=false )
			    : val_( v )		{}

    operator		bool() const		{ return bool(val_); }
    bool		operator=( bool v )	{ val_ = v; return v; }

protected:

    char  val_;

};

typedef TypeSet<BoolTypeSetType> BoolTypeSet;
typedef LargeValVec<BoolTypeSetType> BoolLargeValVec;



template <class T, class IT>
inline bool operator ==( const OD::ValVec<T,IT>& a, const OD::ValVec<T,IT>& b )
{
    return a.operator ==( b );
}

template <class T,class IT> inline
bool OD::ValVec<T,IT>::operator ==( const OD::ValVec<T,IT>& oth ) const
{
    const IT sz = size();
    if ( sz != oth.size() )
	return false;

    for ( IT vidx=0; vidx<sz; vidx++ )
	if ( !(get(vidx) == oth.get(vidx)) )
	    return false;

    return true;
}

template <class T, class IT>
inline bool operator !=( const OD::ValVec<T,IT>& a, const OD::ValVec<T,IT>& b)
{ return !(a == b); }


//! append allowing a different type to be merged into set
template <class T, class IT, class J, class S>
inline bool append( OD::ValVec<T,IT>& to, const OD::ValVec<S,J>& from )
{
    const J sz = from.size();
    if ( !to.setCapacity( sz + to.size(), true ) )
	return false;

    for ( J vidx=0; vidx<sz; vidx++ )
	to.add( (T)from.get(vidx) );

    return true;
}


//! copy from different possibly different type into set
//! Note that there is no optimization for equal size, as in member function.
template <class T, class IT,class S>
inline void copy( OD::ValVec<T,IT>& to, const OD::ValVec<S,IT>& from )
{
    if ( (void*)(&to) == (void*)(&from) ) return;
    to.erase();
    append( to, from );
}


//! Get sort indexes. Must have operator > defined for elements
template <class T, class IT> inline
typename OD::ValVec<T,IT>::idx_type* getSortIndexes( OD::ValVec<T,IT>& vv,
						     bool ascending=true )
{
    typedef typename OD::ValVec<T,IT>::idx_type idx_type;
    const IT sz = vv.size();
    if ( sz < 2 )
	{ idx_type* ret = new int [1]; *ret = 0; return ret; }

    mGetIdxArr( idx_type, idxs, sz );

    for ( IT d=sz/2; d>0; d=d/2 )
	for ( IT i=d; i<sz; i++ )
	    for ( IT j=i-d; j>=0 && vv[idxs[j]]>vv[idxs[j+d]]; j-=d )
		std::swap( idxs[j], idxs[j+d] );

    if ( !ascending )
	std::reverse( idxs, idxs+sz );

    return idxs;
}


//! Sort OD::ValVec. Must have operator > defined for elements
template <class T, class IT>
inline void sort( OD::ValVec<T,IT>& vv )
{
    T tmp; const IT sz = vv.size();
    for ( IT d=sz/2; d>0; d=d/2 )
	for ( IT i=d; i<sz; i++ )
	    for ( IT j=i-d; j>=0 && vv[j]>vv[j+d]; j-=d )
		{ tmp = vv[j]; vv[j] = vv[j+d]; vv[j+d] = tmp; }
}


// Member function implementations
template <class T, class IT> inline
OD::ValVec<T,IT>::ValVec()
{}

template <class T, class IT> inline
OD::ValVec<T,IT>::ValVec( IT nr, T typ )
{ setSize( nr, typ ); }

template <class T, class IT> inline
OD::ValVec<T,IT>::ValVec( const T* tarr, IT nr )
{ append( tarr, nr ); }

template <class T, class IT> inline
OD::ValVec<T,IT>::ValVec( const OD::ValVec<T,IT>& t )
    : OD::Set( t )
{ append( t ); }

template <class T, class IT> inline
OD::ValVec<T,IT>::~ValVec() {}

template <class T, class IT> inline
IT OD::ValVec<T,IT>::size() const
{ return vec_.size(); }

template <class T, class IT> inline
bool OD::ValVec<T,IT>::setSize( IT sz, T val )
{ return vec_.setSize( sz, val ); }

template <class T, class IT> inline
IT OD::ValVec<T,IT>::getCapacity() const
{ return vec_.getCapacity(); }

template <class T, class IT> inline
bool OD::ValVec<T,IT>::setCapacity( IT sz, bool withmargin )
{ return vec_.setCapacity( sz, withmargin ); }

template <class T, class IT> inline
void OD::ValVec<T,IT>::setAll( T val )
{ vec_.fillWith( val ); }

template <class T, class IT> inline
    void OD::ValVec<T,IT>::replace( T val, T newval )
{ vec_.replace( val, newval ); }


template <class T, class IT> inline
bool OD::ValVec<T,IT>::validIdx( od_int64 vidx ) const
{ return vec_.validIdx( sCast(IT,vidx) ); }

template <class T, class IT> inline
T& OD::ValVec<T,IT>::get( IT vidx )
{
#ifdef __debug__
    if ( !validIdx(vidx) )
	DBG::forceCrash(true);
#endif
    return vec_[vidx];
}

template <class T, class IT> inline
const T& OD::ValVec<T,IT>::get( IT vidx ) const
{
#ifdef __debug__
    if ( !validIdx(vidx) )
	DBG::forceCrash(true);
#endif
    return vec_[vidx];
}

template <class T, class IT> inline
T& OD::ValVec<T,IT>::first()
{ return vec_.first(); }

template <class T, class IT> inline
const T& OD::ValVec<T,IT>::first() const
{ return vec_.first(); }

template <class T, class IT> inline
T& OD::ValVec<T,IT>::last()
{ return vec_.last(); }

template <class T, class IT> inline
const T& OD::ValVec<T,IT>::last() const
{ return vec_.last(); }

template <class T, class IT> inline
T OD::ValVec<T,IT>::pop()
{ return vec_.pop_back(); }

template <class T, class IT> inline
IT OD::ValVec<T,IT>::indexOf( T typ, bool forward, IT start ) const
{ return vec_.indexOf( typ, forward, start ); }

template <class T, class IT> inline
bool OD::ValVec<T,IT>::isPresent( const T& t ) const
{ return vec_.isPresent(t); }

template <class T, class IT> inline
IT OD::ValVec<T,IT>::count( const T& typ ) const
{ return vec_.count( typ ); }

template <class T, class IT> inline
OD::ValVec<T,IT>& OD::ValVec<T,IT>::add( const T& typ )
{ vec_.push_back( typ ); return *this; }

template <class T, class IT> inline
bool OD::ValVec<T,IT>::push( const T& typ )
{ return vec_.push_back( typ ); }

template <class T, class IT> inline
OD::ValVec<T,IT>& OD::ValVec<T,IT>::operator -=( const T& typ )
{ vec_.erase( typ ); return *this; }

template <class T, class IT> inline
OD::ValVec<T,IT>& OD::ValVec<T,IT>::copy( const OD::ValVec<T,IT>& oth )
{ return this == &oth ? *this : copy( oth.arr(), oth.size() ); }

template <class T, class IT> inline
void OD::ValVec<T,IT>::erase()
{ vec_.erase(); }

template <class T, class IT> inline
void OD::ValVec<T,IT>::removeRange( IT i1, IT i2 )
{ vec_.remove( i1, i2 ); }

template <class T, class IT> inline
void OD::ValVec<T,IT>::insert( IT vidx, const T& typ )
{ vec_.insert( vidx, typ );}

template <class T, class IT> inline
std::vector<T>& OD::ValVec<T,IT>::vec()
{ return vec_.vec(); }

template <class T, class IT> inline
const std::vector<T>& OD::ValVec<T,IT>::vec() const
{ return vec_.vec(); }

template <class T, class IT> inline
T* OD::ValVec<T,IT>::gtArr() const
{
    return isEmpty() ? nullptr : const_cast<T*>( &first() );
}


template <class T, class IT> inline
void OD::ValVec<T,IT>::swap( IT idx1, IT idx2 )
{
    if ( !validIdx(idx1) || !validIdx(idx2) )
    {
#ifdef __debug__
	DBG::forceCrash(true);
#endif
	return;
    }
    vec_.swapElems( idx1, idx2 );
}


template <class T, class IT> inline
void OD::ValVec<T,IT>::useIndexes( const idx_type* idxs )
{
    const size_type sz = size();
    if ( idxs && sz > 1 )
    {
	ValVec* tmp = clone();
	for ( size_type idx=0; idx<sz; idx++ )
	    get(idx) = tmp->get( idxs[idx] );
	delete tmp;
    }
}


template <class T, class IT> inline
void OD::ValVec<T,IT>::move( IT idxfrom, IT idxto )
{
    if ( !validIdx(idxfrom) || !validIdx(idxto) )
	return;

    T tmp = vec_[idxfrom];
    insert( idxto, tmp );
    vec_.remove( idxfrom < idxto ? idxfrom : idxfrom+1 );
}


template <class T, class IT> inline
void OD::ValVec<T,IT>::reverse()
{
    const IT sz = size();
    const IT hsz = sz/2;
    for ( IT vidx=0; vidx<hsz; vidx++ )
	swap( vidx, sz-1-vidx );
}


template <class T, class IT> inline
OD::ValVec<T,IT>& OD::ValVec<T,IT>::copy( const T* tarr, IT sz )
{
    if ( size() != sz )
	{ erase(); append(tarr,sz); }
    else
    {
	for ( IT vidx=0; vidx<sz; vidx++ )
	    get(vidx) = tarr[vidx];
    }
    return *this;
}


template <class T, class IT> inline
bool OD::ValVec<T,IT>::append( const OD::ValVec<T,IT>& oth )
{
    if ( this != &oth )
	return append( oth.arr(), oth.size() );

    const ValVec* cln = clone();
    const bool ret = append( *cln );
    delete cln;
    return ret;
}


template <class T, class IT> inline
bool OD::ValVec<T,IT>::append( const T* tarr, IT sz )
{
    if ( !sz ) return true;

    if ( !setCapacity( sz+size(), true ) )
	return false;

    for ( IT vidx=0; vidx<sz; vidx++ )
	add( tarr[vidx] );

    return true;
}


template <class T, class IT>
inline void OD::ValVec<T,IT>::createUnion( const OD::ValVec<T,IT>& oth )
{
    const IT sz = oth.size();
    const T* ptr = oth.arr();
    for ( IT vidx=0; vidx<sz; vidx++, ptr++ )
	addIfNew( *ptr );
}


template <class T, class IT>
inline void OD::ValVec<T,IT>::createIntersection( const OD::ValVec<T,IT>& oth )
{
    for ( IT vidx=0; vidx<size(); vidx++ )
    {
	if ( oth.isPresent((*this)[vidx]) )
	    continue;
	removeSingle( vidx--, false );
    }
}


template <class T, class IT>
inline void OD::ValVec<T,IT>::createDifference( const OD::ValVec<T,IT>& oth,
    bool kporder )
{
    const IT sz = oth.size();
    for ( IT vidx=0; vidx<sz; vidx++ )
    {
	const T typ = oth[vidx];
	for ( IT idy=0; idy<size(); idy++ )
	{
	    if ( vec_[idy] == typ )
		removeSingle( idy--, kporder );
	}
    }
}


template <class T, class IT> inline
bool OD::ValVec<T,IT>::addIfNew( const T& typ )
{
    if ( !isPresent(typ) )
	{ *this += typ; return true; }
    return false;
}


template <class T, class IT> inline
void OD::ValVec<T,IT>::removeSingle( IT vidx, bool kporder )
{
    if ( kporder )
	vec_.remove( vidx );
    else
    {
	const IT lastidx = size()-1;
	if ( vidx != lastidx )
	    vec_[vidx] = vec_[lastidx];
	vec_.remove( lastidx );
    }
}



				//--- useful for iterating over any OD::Set
template <class T,class IT>
mGlobal(Basic) inline T& getRef( OD::ValVec<T,IT>& vv, IT i )
{ return vv.get( i ); }
template <class T,class IT>
mGlobal(Basic) inline const T& getRef( const OD::ValVec<T,IT>& vv, IT i )
{ return vv.get( i ); }
