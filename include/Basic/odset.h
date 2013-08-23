#ifndef odset_h
#define odset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2009
 RCS:		$Id$
________________________________________________________________________

-*/

#ifndef gendefs_h
# include "gendefs.h"
#endif

namespace OD
{

/*!
\brief Base class for all sets used in OpendTect. 

Guaranteed are also:

* typedef of indices and sizes: size_type
* typedef of object contained: object_type
* a member 'size() const'
* a member 'removeRange(size_type start,size_type stop)'

*/

mExpClass(Basic) Set
{
public:

    virtual		~Set()					{}

    virtual od_int64	nrItems() const				= 0;
    virtual bool	validIdx(od_int64) const		= 0;
    virtual void	swap(od_int64,od_int64)			= 0;
    virtual void	erase()					= 0;

    inline bool		isEmpty() const		{ return nrItems() <= 0; }
    inline void		setEmpty()		{ erase(); }

};

} // namespace



/*!\brief Removes a range from the set. */

template <class ODSET,class size_type>
inline void removeRange( ODSET& inst, size_type start, size_type stop )
{
    inst.removeRange( start, stop );
}


/*!\brief Adds all names from a set to another set with an add() function
  	(typically a BufferStringSet) */

template <class ODSET,class WITHADD>
inline void addNames( const ODSET& inp, WITHADD& withadd )
{
    const typename ODSET::size_type sz = inp.size();
    for ( typename ODSET::size_type idx=0; idx<sz; idx++ )
	withadd.add( inp[idx]->name() );
}

#define mODSetApplyToAll( itp, os, op ) \
    for ( itp idx=(itp) os.nrItems()-1; idx>=0; idx-- ) \
    { \
        op; \
    }


#endif

