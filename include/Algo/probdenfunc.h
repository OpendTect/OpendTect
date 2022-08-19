#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "bufstringset.h"
#include "namedobj.h"
#include "od_iosfwd.h"
#include "ranges.h"


/*!
\brief Base class for Probability Density Functions.

  The values may not be normalized; if you need them to be: multiply with
  'normFac()'. What you are getting can for example be the values from a
  histogram. All we require is that the value() implementation always returns
  positive values.
*/

mExpClass(Algo) ProbDenFunc : public NamedObject
{
public:

    virtual		~ProbDenFunc();

    virtual ProbDenFunc* clone() const				= 0;
    virtual void	copyFrom(const ProbDenFunc&)		= 0;
    virtual bool	isEqual(const ProbDenFunc&) const;

    virtual const char*	getTypeStr() const			= 0;
    virtual int		nrDims() const				= 0;
    const char*		dimName(int dim) const;
    const char*		getUOMSymbol(int dim) const;

    void		setDimName(int dim,const char*);
    void		setUOMSymbol(int dim,const char*);

    virtual float	averagePos(int dim) const		= 0;
    virtual float	stddevPos(int dim) const		= 0;
    virtual bool	hasLimits() const			= 0;
    virtual float	value(const TypeSet<float>&) const	= 0;

    virtual bool	canScale() const			{ return false;}
    virtual void	scale(float)				{}
    virtual float	normFac() const				{ return 1; }

			// Used for file store/retrieve:
    virtual void	fillPar(IOPar&) const			= 0;
    virtual bool	usePar(const IOPar&)			= 0;
    virtual void	writeBulk(od_ostream&,bool binary) const {}
    virtual bool	readBulk(od_istream&,bool binary)	{ return true; }

    virtual bool	isCompatibleWith(const ProbDenFunc&) const;
    void		getIndexTableFor(const ProbDenFunc& pdf,
					 TypeSet<int>& tbl) const;
			//!< tbl[0] tells what my index is for pdf's index '0'

    virtual void	prepareRandDrawing() const		{}
    virtual void	drawRandomPos(TypeSet<float>&) const	= 0;
    static const char*	sKeyNrDim();

protected:

			ProbDenFunc();
			ProbDenFunc(const ProbDenFunc&);

    virtual bool	isEq(const ProbDenFunc&) const		= 0;
			//!< already checked for type, name, dim names and units

private:

    BufferStringSet	dimnms_;
    BufferStringSet	uoms_;
};


/*!
\brief Probability Density Function for one dimensional datasets.
*/

mExpClass(Algo) ProbDenFunc1D : public ProbDenFunc
{
public:

    int			nrDims() const override		{ return 1; }

    float		averagePos(int) const override	{ return gtAvgPos(); }
    float		stddevPos(int) const override
						{ return gtStdDevPos(); }
    bool		hasLimits() const override { return hasLims(); }
    inline float	value( float v ) const	{ return gtVal( v ); }
    float		value( const TypeSet<float>& v ) const override
						{ return gtVal( v[0] ); }

    inline void		drawRandomPos( float& v ) const
			{ drwRandPos( v ); }
    void		drawRandomPos( TypeSet<float>& v ) const override
			{ v.setSize(1); drwRandPos( v[0] ); }

protected:

			ProbDenFunc1D(const char* vnm="");

    virtual float	gtAvgPos() const	= 0;
    virtual float	gtStdDevPos() const	= 0;
    virtual bool	hasLims() const		= 0;
    virtual float	gtVal(float) const	= 0;
    virtual void	drwRandPos(float&) const = 0;

};


/*!
\brief Probability Density Function for two dimensional datasets.
*/

mExpClass(Algo) ProbDenFunc2D : public ProbDenFunc
{
public:

    int			nrDims() const override		{ return 2; }
    bool		hasLimits() const override { return hasLims(); }

    float		value( float x1, float x2 ) const
			{ return gtVal( x1, x2 ); }
    float		value( const TypeSet<float>& v ) const override
			{ return gtVal( v[0], v[1] ); }

    inline void		drawRandomPos( float& x1, float& x2 ) const
			{ drwRandPos( x1, x2 ); }
    void		drawRandomPos( TypeSet<float>& v ) const override
			{ v.setSize(2); drwRandPos( v[0], v[1] ); }

protected:

			ProbDenFunc2D(const char* vnm0="Dim 0",
				      const char* vnm1="Dim 1");

    virtual float	gtVal(float,float) const	= 0;
    virtual bool	hasLims() const			= 0;
    virtual void	drwRandPos(float&,float&) const = 0;
};
