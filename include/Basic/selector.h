#ifndef selector_H
#define selector_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		18-10-1995
 Contents:	Selectors
 RCS:		$Id: selector.h,v 1.3 2001-04-17 11:40:14 bert Exp $
________________________________________________________________________

-*/

#include <ranges.h>
#include <sets.h>


/*!\brief interface for classes that select on basis of a key.

Some Selectors may be extensible: you can ask them to include a key value.

*/

template <class T>
class Selector
{
public:

    virtual		~Selector()				{}
    virtual const char*	selectorType() const			= 0;
    bool		isEqual( const Selector<T>& s ) const
			{
			    return selectorType() == s.selectorType()
				&& isEq(s);
			}
    virtual Selector<T>* clone() const				= 0;

    virtual bool	includes(const T&) const		= 0;
    virtual bool	include(const T&,const char* =0)       { return false; }

protected:

    virtual bool	isEq(const Selector<T>&) const		= 0;

};


/*!\brief Selector selecting only a single value. */

template <class T>
class SingleSelector : public Selector<T>
{
public:

			SingleSelector()			{}
			SingleSelector( const T& t ) : val(t)	{}
    virtual const char*	selectorType() const		{ return "Single"; }
    virtual Selector<T>* clone() const
			{ return new SingleSelector( val ); }

    virtual bool	includes( const T& t ) const
			{ return val == t; }
    virtual bool	include( const T& t, const char* )
			{ val = t; return true; }

    T			val;

protected:

    virtual bool	isEq( const Selector<T>& ss ) const
			{ return val == ((const SingleSelector<T>&)ss).val; }

};


/*!\brief Selector based on range specification (an Interval). */

template <class T>
class RangeSelector : public Selector<T>
{
public:

			RangeSelector()			{}
			RangeSelector( const T& t1, const T& t2 )
			: range(t1,t2)			{}
    virtual const char*	selectorType() const		{ return "Range"; }
    virtual Selector<T>* clone() const
			{ return new RangeSelector( range.start, range.stop ); }

    virtual bool	includes( const T& t ) const
			{ return range.includes( t ); }
    virtual bool	include( const T& t, const char* )
			{ range.include( t ); return true; }

    Interval<T>		range;

protected:

    virtual bool	isEq( const Selector<T>& rs ) const
			{ return range == ((const RangeSelector<T>&)rs).range; }

};


/*!\brief Selector based on array. */

template <class T>
class ArraySelector : public Selector<T>
{
public:

			ArraySelector() : vals(0), sz(0)	  {}
			ArraySelector( const T* v, int s )
			: vals(v), sz(s), valsmine(false)	  {}
			ArraySelector( const ArraySelector& x )
			: vals(x.vals), sz(x.sz), valsmine(false) {}
			~ArraySelector()
			{ if ( valsmine ) delete [] const_cast<T*>(vals); }

    virtual const char*	selectorType() const		{ return "Array"; }
    virtual Selector<T>* clone() const
			{ return new ArraySelector( vals, sz ); }

    virtual bool	includes( const T& t ) const
			{
			    for ( int idx=0; idx<sz; idx++ )
				{ if ( vals[idx] == t ) return true; }
			    return false;
			}
    void		manageVals( bool yn=true )	{ valsmine = yn; }

    const T*		vals;
    int			sz;

protected:

    virtual bool	isEq( const Selector<T>& ss ) const
			{
			    const ArraySelector<T>& ass
				= (const ArraySelector<T>&)ss;
			    if ( sz != ass.sz ) return false;
			    for ( int idx=0; idx<sz; idx++ )
				{ if ( !ss.includes(vals[idx]) ) return false; }
			    return true;
			}

    bool		valsmine;

};


typedef int (*ObjectTypeSelectionFun)(const char*);

int defaultSelector(const char*,const char*);
/*!<
Object Type Selection functions determine whether a file type is a match for
this object. The return values are: \line
0 - The object has no connection whatsoever with this object type \line
1 - This type may be the connection to an object I can handle \line
2 - The type given is exactly a type I can handle \line
The idea is that each object group defines a selector() function.

defaultSelector returns 0 or 2, depending on exact match.

*/


#endif
