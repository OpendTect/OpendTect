#ifndef probdenfunc_h
#define probdenfunc_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2010
 RCS:		$Id: probdenfunc.h,v 1.1 2010-01-18 16:13:15 cvsbert Exp $
________________________________________________________________________


*/

#include "bufstring.h"
#include "ranges.h"
template <class T> class TypeSet;


mClass ProbDenFunc
{
public:

    virtual		~ProbDenFunc()			{}

    virtual int		nrDims() const			= 0;
    virtual const char*	dimName(int dim) const		= 0;
    virtual float	value(const TypeSet<float>&) const = 0;

    virtual float	normFac() const			{ return 1; }
			//!< factor to get 'true' normalized probabilities
};


mClass ProbDenFunc1D : public ProbDenFunc
{
public:

    virtual int		nrDims() const		{ return 1; }
    virtual const char*	dimName(int) const	{ return varName(); }

    virtual float	value(float) const	= 0;
    virtual const char*	varName() const		{ return varnm_; }

    virtual float	value( const TypeSet<float>& v ) const
						{ return value(v[0]); }

    BufferString	varnm_;

};


mClass ProbDenFunc2D : public ProbDenFunc
{
public:

    virtual int		nrDims() const			{ return 2; }
    virtual const char*	dimName( int idim ) const
			{ return (idim ? dim2nm_ : dim1nm_).buf(); }

    virtual float	value(float,float) const	= 0;
    virtual float	value( const TypeSet<float>& v ) const
			{ return value(v[0],v[1]); }

    BufferString	dim1nm_;
    BufferString	dim2nm_;

};


#endif
