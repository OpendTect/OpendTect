#ifndef selector_h
#define selector_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		18-10-1995
 Contents:	Selectors
 RCS:		$Id: selector.h,v 1.11 2011/09/02 08:51:42 cvskris Exp $
________________________________________________________________________

-*/

#include "ranges.h"


/*!\brief interface for classes that select on basis of a key.

Some Selectors may be extensible: you can ask them to include a key value.

*/

template <class T>
class Selector
{
public:

    virtual		~Selector()				{}
    virtual const char*	selectorType() const			= 0;
    virtual bool	isOK() const				{ return true; }
    bool		isEqual( const Selector<T>& s ) const
			{
			    return selectorType() == s.selectorType()
				&& isEq(s);
			}
    virtual Selector<T>* clone() const				= 0;

    virtual bool	includes(const T&) const		= 0;
    virtual bool	canDoRange() const			{return false;}
    virtual char	includesRange(const T& start,
	    			      const T& stop) const	{ return -1; }
    			/*!<\retval 0 not at all
			    \retval 1 partly
			    \retval 2 completely */

    virtual bool	include(const T&,const char* =0)       { return false; }

private:

    virtual bool	isEq(const Selector<T>&) const		= 0;

};


/*!\brief Selector selecting only a single value. */

template <class T>
class SingleSelector : public Selector<T>
{
public:

			SingleSelector()			{}
			SingleSelector( const T& t ) : val_(t)	{}
    virtual const char*	selectorType() const		{ return "Single"; }
    virtual Selector<T>* clone() const
			{ return new SingleSelector( val_ ); }

    virtual bool	includes( const T& t ) const
			{ return val_ == t; }
    virtual bool	canDoRange() const			{return true;}
    virtual char	includesRange(const T& start,
	    			      const T& stop) const;
    virtual bool	include( const T& t, const char* )
			{ val_ = t; return true; }

    T			val_;

protected:

    virtual bool	isEq( const Selector<T>& ss ) const
			{ return val_ == ((const SingleSelector<T>&)ss).val_; }

};


/*!\brief Selector based on range specification (an Interval). */

template <class T>
class RangeSelector : public Selector<T>
{
public:

			RangeSelector()			{}
			RangeSelector( const T& t1, const T& t2 )
			: range_(t1,t2)			{}
    virtual const char*	selectorType() const		{ return "Range"; }
    virtual Selector<T>* clone() const
			{ return new RangeSelector(range_.start,range_.stop); }

    virtual bool	includes( const T& t ) const
			{ return range_.includes( t, true ); }
    virtual bool	include( const T& t, const char* )
			{ range_.include( t ); return true; }

    Interval<T>		range_;

protected:

    virtual bool	isEq( const Selector<T>& rs ) const
			{ return range_==((const RangeSelector<T>&)rs).range_; }

};


/*!\brief Selector based on array. */

template <class T>
class ArraySelector : public Selector<T>
{
public:

			ArraySelector()
			: vals_(0), sz_(0), valsmine_(true)	  {}
			ArraySelector( const T* v, int s )
			: vals_(v), sz_(s), valsmine_(false)	  {}
			ArraySelector( const ArraySelector& x )
			: vals_(x.vals_), sz_(x.sz_), valsmine_(false) {}
			~ArraySelector()
			{ if ( valsmine_ ) delete [] const_cast<T*>(vals_); }

    virtual const char*	selectorType() const		{ return "Array"; }
    virtual Selector<T>* clone() const
			{ return new ArraySelector( vals_, sz_ ); }

    virtual bool	includes( const T& t ) const
			{
			    for ( int idx=0; idx<sz_; idx++ )
				{ if ( vals_[idx] == t ) return true; }
			    return false;
			}
    void		manageVals( bool yn=true )	{ valsmine_ = yn; }

    const T*		vals_;
    int			sz_;

protected:

    virtual bool	isEq( const Selector<T>& ss ) const
			{
			    const ArraySelector<T>& ass
				= (const ArraySelector<T>&)ss;
			    if ( sz_ != ass.sz_ ) return false;
			    for ( int idx=0; idx<sz_; idx++ )
				{ if ( !ss.includes(vals_[idx]) ) return false;}
			    return true;
			}

    bool		valsmine_;

};


template <class T> inline
char SingleSelector<T>::includesRange( const T& start, const T& stop ) const
{
    const Interval<T> rg( start, stop );
    if ( start==stop==val_ )
	return 2;

    if ( rg.includes( val_, true ) )
	return 1;

    return 0;
}

#endif
