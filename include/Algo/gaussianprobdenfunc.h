#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2010
________________________________________________________________________


*/

#include "algomod.h"
#include "probdenfunc.h"
#include "samplingdata.h"
#include "arrayndimpl.h"

template <class T> class Array2DMatrix;
namespace Stats { class NormalRandGen; }


inline float cMaxGaussianCC()		{ return 0.99999f; }
inline const char* sGaussianCCRangeErrMsg()
{ return "Correlation coefficients should be in range <-1,1>.\n"
	 "Maximum correlation is 0.99999."; }

#define mDefGaussianProbDenFuncFns(nm) \
				~nm##ProbDenFunc(); \
    nm##ProbDenFunc&		operator =(const nm##ProbDenFunc&); \
    virtual nm##ProbDenFunc*	clone() const \
				{ return new nm##ProbDenFunc(*this); } \
    virtual void		copyFrom(const ProbDenFunc&); \
    static const char*		typeStr()		{ return #nm; } \
    virtual const char*		getTypeStr() const	{ return typeStr(); } \
    virtual void		fillPar(IOPar&) const; \
    virtual bool		usePar(const IOPar&); \
    virtual bool		isEq(const ProbDenFunc&) const;



/*!\brief One dimensional Gaussian PDF. */

mExpClass(Algo) Gaussian1DProbDenFunc : public ProbDenFunc1D
{
public:

			Gaussian1DProbDenFunc( float exp=0, float stdev=1 )
			    : rgen_(0), exp_(exp), std_(stdev)	{}
			Gaussian1DProbDenFunc(const Gaussian1DProbDenFunc& oth)
			    : rgen_(0)	{ *this = oth; }

			mDefGaussianProbDenFuncFns(Gaussian1D)

    float		exp_;
    float		std_;

protected:

    mutable Stats::NormalRandGen* rgen_ = nullptr;

    virtual float	gtAvgPos() const		{ return exp_; }
    virtual float	gtVal(float) const;
    virtual void	drwRandPos(float&) const;

};


/*!\brief Two dimensional Gaussian PDF. */

mExpClass(Algo) Gaussian2DProbDenFunc : public ProbDenFunc2D
{
public:

			Gaussian2DProbDenFunc()
			    : rgen0_(0), rgen1_(0)
			    , exp0_(0), exp1_(0)
			    , std0_(1), std1_(1), cc_(0) {}
			Gaussian2DProbDenFunc(const Gaussian2DProbDenFunc& oth)
			    : rgen0_(0), rgen1_(0)	{ *this = oth; }

			mDefGaussianProbDenFuncFns(Gaussian2D)

    virtual float	averagePos( int dim ) const
			{ return dim ? exp1_ : exp0_; }

    float		exp0_, exp1_;
    float		std0_, std1_;
    float		cc_;

protected:

    mutable Stats::NormalRandGen* rgen0_ = nullptr;
    mutable Stats::NormalRandGen* rgen1_ = nullptr;

    virtual float	gtVal(float,float) const;
    virtual void	drwRandPos(float&,float&) const;

};


/*!\brief Multi-dimensional pure Gaussian PDF. */

mExpClass(Algo) GaussianNDProbDenFunc : public ProbDenFunc
{
public:

			GaussianNDProbDenFunc(int nrdims=3);
			GaussianNDProbDenFunc(const GaussianNDProbDenFunc& oth)
						{ *this = oth; }
			mDefGaussianProbDenFuncFns(GaussianND)

    virtual int		nrDims() const		{ return vars_.size(); }
    virtual const char*	dimName(int) const;
    virtual void	setDimName(int,const char*);
    virtual float	averagePos(int) const;

    void		prepareRandDrawing() const;
    virtual void	drawRandomPos(TypeSet<float>&) const;
    virtual float	value(const TypeSet<float>&) const;
			//!< Not properly implemented because it can't be done

    mExpClass(Algo) VarDef
    {
    public:

			VarDef( const char* nm, float e=0, float s=1 )
			    : name_(nm), exp_(e), std_(s)	{}
	bool		operator ==( const VarDef& vd ) const
			{ return name_ == vd.name_; }

	BufferString	name_;
	float		exp_;
	float		std_;
    };

    mExpClass(Algo) Corr
    {
    public:
			Corr( int i0=0, int i1=0, float cc=1 )
			    : idx0_(i0), idx1_(i1), cc_(cc)	{}
	bool		operator ==( const Corr& oth ) const
			{ return (idx0_ == oth.idx0_ && idx1_ == oth.idx1_)
			      || (idx0_ == oth.idx1_ && idx1_ == oth.idx0_); }

	int		idx0_, idx1_;
	float		cc_;

    };

    TypeSet<VarDef>	vars_;
    TypeSet<Corr>	corrs_;

    const char*		firstUncorrelated() const;

protected:


    mutable ObjectSet<Stats::NormalRandGen> rgens_;
    Array2DMatrix<float>*	cholesky_ = nullptr;

};


