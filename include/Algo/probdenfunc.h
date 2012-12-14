#ifndef probdenfunc_h
#define probdenfunc_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2010
 RCS:		$Id$
________________________________________________________________________


*/

#include "algomod.h"
#include "namedobj.h"
#include "ranges.h"

template <class T> class TypeSet;
class IOPar;


/*!
  \ingroup Algo
  \brief Base class for Probability Density Functions.
  
  The values may not be normalized; if you need them to be: multiply with
  'normFac()'. What you are getting can for example be the values from a
  histogram. All we require is that the value() implementation always returns
  positive values.
*/

mClass(Algo) ProbDenFunc : public NamedObject
{
public:

    virtual ProbDenFunc* clone() const				= 0;
    virtual		~ProbDenFunc()				{}
    virtual void	copyFrom(const ProbDenFunc&)		= 0;

    virtual const char*	getTypeStr() const			= 0;
    virtual int		nrDims() const				= 0;
    virtual const char*	dimName(int dim) const			= 0;
    virtual void	setDimName(int dim,const char*)		= 0;
    virtual float	averagePos(int dim) const		= 0;
    virtual float	value(const TypeSet<float>&) const	= 0;

    virtual bool	canScale() const			{ return false;}
    virtual void	scale(float)				{}
    virtual float	normFac() const				{ return 1; }

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&)			{ return true; }
    virtual void	dump(std::ostream&,bool binary) const	{}
    virtual bool	obtain(std::istream&,bool binary)	{ return true; }

    virtual bool	isCompatibleWith(const ProbDenFunc&) const;
    void		getIndexTableFor(const ProbDenFunc& pdf,
	    				 TypeSet<int>& tbl) const;
    			//!< tbl[0] tells what my index is for pdf's index '0'
    
    virtual void	prepareRandDrawing() const		{}
    virtual void	drawRandomPos(TypeSet<float>&) const	= 0;
    static const char*	sKeyNrDim();

protected:

    			ProbDenFunc()				{}
    			ProbDenFunc(const ProbDenFunc&);

};


/*!
  \ingroup Algo
  \brief Probability Density Function for one dimensional datasets.
*/
mClass(Algo) ProbDenFunc1D : public ProbDenFunc
{
public:

    virtual void	copyFrom( const ProbDenFunc& pdf )
			{ varnm_ = pdf.dimName(0); setName(pdf.name()); }

    virtual int		nrDims() const		{ return 1; }
    virtual const char*	dimName(int) const	{ return varName(); }
    virtual void	setDimName( int dim, const char* nm )
						{ if ( !dim ) varnm_ = nm; }

    virtual const char*	varName() const		{ return varnm_; }

    virtual float	averagePos(int) const	{ return gtAvgPos(); }
    inline float	value( float v ) const	{ return gtVal( v ); }
    virtual float	value( const TypeSet<float>& v ) const
						{ return gtVal( v[0] ); }

    inline void		drawRandomPos( float& v ) const
			{ drwRandPos( v ); }
    virtual void	drawRandomPos( TypeSet<float>& v ) const
			{ v.setSize(1); drwRandPos( v[0] ); }

    BufferString	varnm_;

protected:

    			ProbDenFunc1D( const char* vnm )
			    : varnm_(vnm)	{}
    			ProbDenFunc1D( const ProbDenFunc1D& pdf )
			    : ProbDenFunc(pdf)
			    , varnm_(pdf.varnm_)		{}
    ProbDenFunc1D&	operator =(const ProbDenFunc1D&);

    virtual float	gtAvgPos() const	= 0;
    virtual float	gtVal(float) const	= 0;
    virtual void	drwRandPos(float&) const = 0;

};


/*!
  \ingroup Algo
  \brief Probability Density Function for two dimensional datasets.
*/
mClass(Algo) ProbDenFunc2D : public ProbDenFunc
{
public:

    virtual void	copyFrom( const ProbDenFunc& pdf )
			{ dim0nm_ = pdf.dimName(0); dim1nm_ = pdf.dimName(1);
			  setName(pdf.name()); }

    virtual int		nrDims() const			{ return 2; }
    virtual const char*	dimName(int) const;
    virtual void	setDimName( int dim, const char* nm )
			{ if ( dim < 2 ) (dim ? dim1nm_ : dim0nm_) = nm; }

    float		value( float x1, float x2 ) const
			{ return gtVal( x1, x2 ); }
    virtual float	value( const TypeSet<float>& v ) const
			{ return gtVal( v[0], v[1] ); }

    inline void		drawRandomPos( float& x1, float& x2 ) const
			{ drwRandPos( x1, x2 ); }
    virtual void	drawRandomPos( TypeSet<float>& v ) const
			{ v.setSize(2); drwRandPos( v[0], v[1] ); }

    BufferString	dim0nm_;
    BufferString	dim1nm_;

protected:

    			ProbDenFunc2D( const char* vnm0, const char* vnm1 )
			    : dim0nm_(vnm0), dim1nm_(vnm1)	{}
    			ProbDenFunc2D( const ProbDenFunc2D& pdf )
			    : ProbDenFunc(pdf)
			    , dim0nm_(pdf.dim0nm_)
			    , dim1nm_(pdf.dim1nm_)		{}
    ProbDenFunc2D&	operator =(const ProbDenFunc2D&);

    virtual float	gtVal(float,float) const	= 0;
    virtual void	drwRandPos(float&,float&) const = 0;

};


#endif

