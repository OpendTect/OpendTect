#ifndef selector_H
#define selector_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		18-10-1995
 Contents:	Selectors
 RCS:		$Id: selector.h,v 1.1.1.1 1999-09-03 10:11:41 dgb Exp $
________________________________________________________________________

@$*/

#include <ranges.h>


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

    virtual int		includes(const T&) const		= 0;
    virtual int		include(const T&,const char* =0)	{ return NO; }

protected:

    virtual bool	isEq(const Selector<T>&) const		= 0;

};


template <class T>
class SingleSelector : public Selector<T>
{
public:

			SingleSelector()			{}
			SingleSelector( const T& t ) : val(t)	{}
    virtual const char*	selectorType() const		{ return "Single"; }
    virtual Selector<T>* clone() const
			{ return new SingleSelector( val ); }

    virtual int		includes( const T& t ) const
			{ return val == t; }
    virtual int		include( const T& t, const char* )
			{ val = t; return YES; }

    T			val;

protected:

    virtual bool	isEq( const Selector<T>& ss ) const
			{ return val == ((const SingleSelector<T>&)ss).val; }

};


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

    virtual int		includes( const T& t ) const
			{ return range.includes( t ); }
    virtual int		include( const T& t, const char* )
			{ range.include( t ); return YES; }

    Interval<T>		range;

protected:

    virtual bool	isEq( const Selector<T>& rs ) const
			{ return range == ((const RangeSelector<T>&)rs).range; }

};


/*$@
Object Type Selection functions determine whether a file type is a match for
this object. The return values are: \line
0 - The object has no connection whatsoever with this object type \line
1 - This type may be the connection to an object I can handle \line
2 - The type given is exactly a type I can handle \line
The idea is that each object group defines a selector() function.
@$*/

// return: 0 = no, 2=right on, 1=connected to
typedef int (*ObjectTypeSelectionFun)(const char*);

// Will match on exact match or "... File" with "..."
int defaultSelector(const char*,const char*);


/*$-*/
#endif
