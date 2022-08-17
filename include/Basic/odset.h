#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2009
________________________________________________________________________

-*/

# include "gendefs.h"


namespace OD
{

/*!
\brief Base class for all sets used in OpendTect.

Guaranteed are also:

* typedef of indices and sizes: idx_type and size_type
* typedef of object contained: object_type
* a member 'size() const'
* a member 'removeRange(size_type start,size_type stop)'

*/

mExpClass(Basic) Set
{
public:

    virtual Set*	clone() const				= 0;
    virtual		~Set()					{}

    virtual od_int64	nrItems() const				= 0;
    virtual bool	validIdx(od_int64) const		= 0;
    virtual void	swapItems(od_int64,od_int64)		= 0;
    virtual void	erase()					= 0;
    virtual void	reverse()				= 0;

    inline bool		isEmpty() const		{ return nrItems() <= 0; }
    inline void		setEmpty()		{ erase(); }

    static inline od_int32	maxIdx32()	{ return 2147483647; }
    static inline od_int64	maxIdx64()	{ return 9223372036854775807LL;}
};

} // namespace OD



/*!\brief Removes a range from the set. */

template <class ODSET,class size_type>
inline void removeRange( ODSET& inst, size_type start, size_type stop )
{
    inst.removeRange( start, stop );
}


/*!\brief Adds all names from a set to another set with an add() function
	(typically a BufferStringSet) */

template <class ODSET,class SET>
inline void addNames( const ODSET& inp, SET& setwithadd )
{
    for ( auto obj : inp )
	if ( obj )
	    setwithadd.add( obj->name() );
}


#ifdef __msvc__
# define mTypeName
#else
# define mTypeName typename
#endif

#define mIsContainer( clss, typ, memb ) \
protected: \
 \
    typedef typ		impl_type; \
    impl_type		memb; \
 \
public: \
 \
    typedef mTypeName impl_type::size_type	size_type; \
    typedef mTypeName impl_type::idx_type	idx_type; \
    typedef mTypeName impl_type::object_type	object_type; \
    typedef mTypeName impl_type::value_type	value_type; \
    typedef mTypeName impl_type::iterator	iterator; \
    typedef mTypeName impl_type::const_iterator	const_iterator; \
    typedef mTypeName impl_type::difference_type difference_type; \
    typedef mTypeName impl_type::reference	reference; \
    typedef mTypeName impl_type::const_reference const_reference; \
 \
    iterator		begin()		{ return memb.begin(); } \
    const_iterator	begin() const	{ return memb.cbegin(); } \
    const_iterator	cbegin() const	{ return memb.cbegin(); } \
    iterator		end()		{ return memb.end(); } \
    const_iterator	end() const	{ return memb.cend(); } \
    const_iterator	cend() const	{ return memb.cend(); } \
    inline size_type	max_size() const { return memb.max_size(); } \
    inline bool		empty() const	{ return memb.empty(); } \
    inline void		swap( clss& oth ) { memb.swap(oth.memb); } \
 \
    size_type	getIdx( iterator it ) const	{ return memb.getIdx(it); } \
    size_type	getIdx( const_iterator it ) const { return memb.getIdx(it); }


#define mDefContainerSwapFunction( mod, clss ) \
mGlobal(mod) inline void swap( clss& o1, clss& o2 ) \
{ \
    o1.swap( o2 ); \
}


#define mODSetApplyToAll( itp, os, op ) \
    for ( itp idx=(itp) os.nrItems()-1; idx>=0; idx-- ) \
    { \
        op; \
    }


