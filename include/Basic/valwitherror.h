#ifndef valwitherror_h
#define valwitherror_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id$
________________________________________________________________________

ValWithError is a value with a known error (variance), and that knows
how the error propagates in the four basic operations +,-,*,/  
ValWithError can be instanciated with float & double. It's probable 
that it will work on complex data, although not tested.

@$*/

#include <math.h>
#include "gendefs.h"

template <class A>
class ValWithError
{
public:
    					ValWithError(A val__ = 0, A var__ = 0)
					    : val_(val__), variance_(var__) {}

    					operator A () { return val_; }

    template<class B> ValWithError<A>& 	operator += (const ValWithError<B>& b)
					{ *this = *this + b; return *this; }
    template<class B> ValWithError<A>& 	operator -= (const ValWithError<B>& b)
					{ *this = *this - b; return *this; }
    template<class B> ValWithError<A>& 	operator *= (const ValWithError<B>& b)
					{ *this = *this * b; return *this; }
    template<class B> ValWithError<A>& 	operator /= (const ValWithError<B>& b)
					{ *this = *this / b; return *this; }

    template<class B> void    		operator += (B b) { val_ += b; }
    template<class B> void     		operator -= (B b) { val_ -= b; }
    template<class B> void     		operator *= (B b) 
					{ val_ *= b; variance_ *= fabs(b);}
    template<class B> void     		operator /= (B b) 
					{ val_ /= b; variance_ /= fabs(b);} 

    A					stDev() const 
    					{ return Math::Sqrt(variance_);}
    A					var() const { return variance_; }
    A	        			val() const { return val_; }

    void				setVal(A val__) { val_ = val__; }
    void				setVar(A var__) { variance_ = var__; }

protected:	     		
    A		val_;
    A		variance_;
};

template <class A, class B> 
inline ValWithError<A> operator + (const ValWithError<A>& a, 
				   const ValWithError<B>& b) 
{
  return ValWithError<A> (a.val() + b.val(), a.var() + b.var());
}

template <class A, class B> 
inline ValWithError<A> operator + (const ValWithError<A>& a, B b)
{
  return ValWithError<A> (a.val() + b, a.var());
}

template <class A, class B> 
inline ValWithError<A> operator - (const ValWithError<A>& a, 
				   const ValWithError<B>& b) 
{
  return ValWithError<A> (a.val() - b.val(), a.var() + b.var());
}

template <class A, class B> 
inline ValWithError<A> operator - (const ValWithError<A>& a, B b)
{
  return ValWithError<A> (a.val() - b, a.var());
}

#define mSQ(val) ((val)*(val))
template <class A, class B> 
inline ValWithError<A> operator * (const ValWithError<A>& a, 
				   const ValWithError<B>& b) 
{
    A product = a.val() * b.val();
    return ValWithError<A> ( product, 
		mSQ(product) * (a.var()/mSQ(a.val() + b.var()/mSQ(b.val())))); 
}

template <class A, class B> 
inline ValWithError<A> operator * (const ValWithError<A>& a, B b)
{
  return ValWithError<A> (a.val() * b, a.var() * fabs(b));
}


template <class A, class B> 
inline ValWithError<A> operator / (const ValWithError<A>& a, 
				   const ValWithError<B>& b) 
{
    A div = a.val() / b.val();
    return ValWithError<A> ( div, 
		mSQ(div) * (a.var()/mSQ(a.val() + b.var()/mSQ(b.val())))); 
}

template <class A, class B> 
inline ValWithError<A> operator / (const ValWithError<A>& a, B b)
{
  return ValWithError<A> (a.val() / b, a.var() / fabs(b));
}


template <class A>
inline ValWithError<A> operator + (const ValWithError<A>& x) 
{ return x; }

template <class A> 
inline ValWithError<A> operator - (const ValWithError<A>& x)
{
  return ValWithError<A> (-x.val(), x.var());
}

template <class A> inline bool
operator == (const ValWithError<A>& x, const ValWithError<A>& y) 
{
  return mIsEqual(x.val(),y.val(),mDefEps) && mIsEqual(x.var(),y.var(),mDefEps);
}

template <class A> inline bool
operator == (A x, const ValWithError<A>& y)
{
  return mIsEqual(x,y.val(),mDefEps);
}

template <class A> inline bool
operator == (const ValWithError<A>& x, A y)
{
  return mIsEqual(x.val(),y,mDefEps);
}

template <class A> inline bool
operator != (const ValWithError<A>& x, const ValWithError<A>& y) 
{
  return !(x == y);
} 

template <class A> inline bool
operator != (A x, const ValWithError<A>& y)
{
  return !(x == y);
}

template <class A> inline bool
operator != (const ValWithError<A>& x, A y)
{
  return !(x == y);
}

#endif
