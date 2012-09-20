#ifndef toplist_h
#define toplist_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K Tingdahl
 Date:		April 2003
 RCS:		$Id$
________________________________________________________________________

-*/

#include "typeset.h"

#include <utility>

/*!\brief
is a class that holds a "top N" list with the N highest (or lowest) values
that is added. Each value has an associated value that can be used as an
identifier of where the value come from or something like that.
*/

template <class VT, class AVT>
class TopList
{
public:
			TopList( int maxsize )
			    : maxsize_( maxsize )		{}

    virtual		~TopList()				{}

    inline void		reset();
    			/*!< Removes all values */

    inline VT		getValue(int rank) const;
    inline AVT		getAssociatedValue(int rank) const;

    inline bool		getHighestValue( VT& ) const;
    inline bool		getLowestValue( VT& ) const;

    inline int		size() const;
    inline int		isEmpty() const { return !size(); }

    inline void		addValue( VT val, AVT aval );
private:

    TypeSet<std::pair<VT,AVT> >	values_;
    const int			maxsize_;
};


template <class VT, class AVT> inline
void TopList<VT,AVT>::reset()
{
    values_.erase();
}


template <class VT, class AVT> inline
VT TopList<VT,AVT>::getValue(int pos) const
{ return values_[pos].first; }


template <class VT, class AVT> inline
AVT TopList<VT,AVT>::getAssociatedValue(int pos) const
{ return values_[pos].second; }


template <class VT, class AVT> inline
int TopList<VT,AVT>::size() const
{ return values_.size(); }



template <class VT, class AVT> inline
bool TopList<VT,AVT>::getHighestValue( VT& res ) const
{
    if ( size() )
    {
	res = values_[0].first;
	return true;
    }

    return false;
}


template <class VT, class AVT> inline
bool TopList<VT,AVT>::getLowestValue( VT& res ) const
{
    const int sz = size();
    if ( sz )
    {
	res = values_[sz-1].first;
	return true;
    }

    return false;
}



template <class VT, class AVT> inline
void TopList<VT,AVT>::addValue( VT val, AVT aval )
{
    int pos = 0;
    const int mysize = size();

    while ( pos<mysize && values_[pos].first>val ) pos++;

    if ( pos==mysize )
    {
	if ( mysize>=maxsize_ )
	    return;

	values_ += std::pair<VT,AVT>( val, aval );
    }
    else
    {
	values_.insert( pos, std::pair<VT,AVT>(val,aval) );

	if ( mysize==maxsize_ )
	    values_.remove(mysize);
    }
}
#endif
