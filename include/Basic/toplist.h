#ifndef toplist_h
#define toplist_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K Tingdahl
 Date:		April 2003
 RCS:		$Id: toplist.h,v 1.5 2009-07-22 16:01:14 cvsbert Exp $
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
			TopList( int maxsize_, VT undefval_, bool istop_)
			    : istop( istop_ )
			    , maxsize( maxsize_ )
			    , undefval( undefval_ ) { }
    virtual		~TopList() {}

    inline bool		isTop() const;
    inline void		setTop(bool yn);
    			/*!<\note If the new setting is different from the 
			  	  current one, the object will be reset
			*/
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
    bool		istop;
    const int		maxsize;
    VT			undefval;

};


template <class VT, class AVT> inline
bool TopList<VT,AVT>::isTop() const { return istop; }


template <class VT, class AVT> inline
void TopList<VT,AVT>::setTop( bool yn )
{
    if ( yn==istop ) return;
    istop = yn;
    reset();
}


template <class VT, class AVT> inline
void TopList<VT,AVT>::reset()
{
    values.erase();
    avals.erase();
}


template <class VT, class AVT> inline
VT TopList<VT,AVT>::getBottomValue() const
{
    if ( size() )
	return values[size()-1];
    return undefval;
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
    if ( istop )
	while ( pos<mysize && values[pos]>val ) pos++;
    else
	while ( pos<mysize && values[pos]<val ) pos++;

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
