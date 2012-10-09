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

/*!\brief
is a class that holds a "top N" list with the N highest (or lowest) values
that is added. Each value has an associated value that can be used as an
identifier of where the value come from or something like that.
*/

template <class VT, class AVT>
class TopList
{
public:
			TopList( int maxsize_ )
			    : maxsize( maxsize_ )	{}
    virtual		~TopList() {}

    inline void		reset();
    			/*!< Removes all values */

    inline VT		getValue(int pos) const;
    inline AVT		getAssociatedValue(int pos) const;

    inline virtual int	size() const;
    inline VT		getBottomValue() const;
    inline void		addValue( VT val, AVT aval );
private:

    TypeSet<VT>		values;
    TypeSet<AVT>	avals;
    const int		maxsize;

};



template <class VT, class AVT> inline
void TopList<VT,AVT>::reset()
{
    values.erase();
    avals.erase();
}


template <class VT, class AVT> inline
VT TopList<VT,AVT>::getBottomValue() const
{
    return size() ? values[size()-1] : (VT)0;
}


template <class VT, class AVT> inline
VT TopList<VT,AVT>::getValue(int pos) const { return values[pos]; }


template <class VT, class AVT> inline
AVT TopList<VT,AVT>::getAssociatedValue(int pos) const { return avals[pos]; }


template <class VT, class AVT> inline
int TopList<VT,AVT>::size() const { return avals.size(); }


template <class VT, class AVT> inline
void TopList<VT,AVT>::addValue( VT val, AVT aval )
{
    int pos = 0;
    const int mysize = size();
    while ( pos<mysize && values[pos]>val ) pos++;

    if ( pos==mysize )
    {
	if ( mysize>=maxsize )
	    return;

	values += val;
	avals += aval;
    }
    else
    {
	values.insert( pos, val );
	avals.insert( pos, aval );

	if ( mysize==maxsize )
	{
	    values.remove(mysize);
	    avals.remove(mysize);
	}
    }
}
#endif
