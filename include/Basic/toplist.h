#ifndef toplist_h
#define toplist_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K Tingdahl
 Date:		April 2003
 RCS:		$Id: toplist.h,v 1.2 2003-11-07 12:21:51 bert Exp $
________________________________________________________________________

-*/

#include <sets.h>

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
			    , undefval( undefval_ )
			{ }
    virtual		~TopList() {}

    VT			getValue(int pos) const { return values[pos]; }
    AVT			getAssociatedValue(int pos) const { return avals[pos]; }

    virtual int		size() const { return values.size(); }
    VT			getBottomValue() const
			{
			    if ( size() )
				return values[size()-1];
			    return undefval;
			}

    void		addValue( VT val, AVT aval )
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
private:

    TypeSet<VT>		values;
    TypeSet<AVT>	avals;
    const bool		istop;
    const int		maxsize;
    VT			undefval;

};


#endif
