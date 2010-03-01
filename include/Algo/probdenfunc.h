#ifndef probdenfunc_h
#define probdenfunc_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2010
 RCS:		$Id: probdenfunc.h,v 1.8 2010-03-01 09:28:52 cvsbert Exp $
________________________________________________________________________


*/

#include "namedobj.h"
#include "ranges.h"

template <class T> class TypeSet;
class IOPar;


/* Probability Density Function

   The values may not ne normalized; if you need them to be: multiply with
   'normFac()'. What you are getting can for example be the values from a
   histogram. All we require is that the value() implementation always returns
   positive values.

*/

mClass ProbDenFunc : public NamedObject
{
public:

    virtual		~ProbDenFunc()			{}

    virtual const char*	getTypeStr() const			= 0;
    virtual int		nrDims() const				= 0;
    virtual const char*	dimName(int dim) const			= 0;
    virtual void	setDimName(int dim,const char*)		= 0;
    virtual float	value(const TypeSet<float>&) const	= 0;

    virtual float	normFac() const				{ return 1; }

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&)			{ return true; }
    virtual void	dump(std::ostream&,bool binary) const	{}
    virtual bool	obtain(std::istream&,bool binary)	{ return true; }

    virtual bool	isCompatibleWith(const ProbDenFunc&) const;
    void		getIndexTableFor(const ProbDenFunc& pdf,
	    				 TypeSet<int>& tbl) const;
    			//!< tbl[0] tells what my index is for pdf's index '0'
    
    static const char*	sKeyNrDim();
};


mClass ProbDenFunc1D : public ProbDenFunc
{
public:

    virtual int		nrDims() const		{ return 1; }
    virtual const char*	dimName(int) const	{ return varName(); }
    virtual void	setDimName( int dim, const char* nm )
						{ if ( !dim ) varnm_ = nm; }

    virtual float	value(float) const	= 0;
    virtual const char*	varName() const		{ return varnm_; }

    virtual float	value( const TypeSet<float>& v ) const
						{ return value(v[0]); }

    BufferString	varnm_;

protected:

    			ProbDenFunc1D( const char* vnm )
			    : varnm_(vnm)	{}
    ProbDenFunc1D&	operator =(const ProbDenFunc1D&);

};


mClass ProbDenFunc2D : public ProbDenFunc
{
public:

    virtual int		nrDims() const			{ return 2; }
    virtual const char*	dimName(int) const;
    virtual void	setDimName( int dim, const char* nm )
			{ if ( dim < 2 ) (dim ? dim1nm_ : dim0nm_) = nm; }

    virtual float	value(float,float) const	= 0;
    virtual float	value( const TypeSet<float>& v ) const
			{ return value(v[0],v[1]); }

    BufferString	dim0nm_;
    BufferString	dim1nm_;

protected:

    			ProbDenFunc2D( const char* vnm0, const char* vnm1 )
			    : dim0nm_(vnm0), dim1nm_(vnm1)	{}
    ProbDenFunc2D&	operator =(const ProbDenFunc2D&);

};


#endif
