#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
    nm##ProbDenFunc*		clone() const override \
				{ return new nm##ProbDenFunc(*this); } \
    void			copyFrom(const ProbDenFunc&) override; \
    static const char*		typeStr()		{ return #nm; } \
    const char*			getTypeStr() const override { return typeStr();} \
    void			fillPar(IOPar&) const override; \
    bool			usePar(const IOPar&) override; \
    bool			isEq(const ProbDenFunc&) const override;



/*!\brief One dimensional Gaussian PDF. */

mExpClass(Algo) Gaussian1DProbDenFunc : public ProbDenFunc1D
{
public:

			Gaussian1DProbDenFunc( const char* dimnm =nullptr )
			    : ProbDenFunc1D(dimnm)	{}
			Gaussian1DProbDenFunc(const Gaussian1DProbDenFunc& oth)
						{ *this = oth; }

			mDefGaussianProbDenFuncFns(Gaussian1D)

    Gaussian1DProbDenFunc& set( float exp,float std )
			{ exp_ = exp; std_ = std; return *this; }

private:

    float		gtAvgPos() const override	{ return exp_; }
    float		gtStdDevPos() const override	{ return std_; }
    bool		hasLims() const override	{ return false; }
    float		gtVal(float) const override;
    void		drwRandPos(float&) const override;

    float		exp_ = 0.f;
    float		std_ = 1.f;

    mutable Stats::NormalRandGen* rgen_ = nullptr;
};


/*!\brief Two dimensional Gaussian PDF. */

mExpClass(Algo) Gaussian2DProbDenFunc : public ProbDenFunc2D
{
public:

			Gaussian2DProbDenFunc( const char* dim0nm =nullptr,
					       const char* dim1nm =nullptr )
			    : ProbDenFunc2D(dim0nm,dim1nm)	{}
			Gaussian2DProbDenFunc(const Gaussian2DProbDenFunc& oth)
				{ *this = oth; }

			mDefGaussianProbDenFuncFns(Gaussian2D)

    Gaussian2DProbDenFunc& set(int idim,float exp,float std);
    Gaussian2DProbDenFunc& setCorrelation( float cc )
			{ cc_ = cc; return *this; }

    float		averagePos( int dim ) const override
			{ return dim ? exp1_ : exp0_; }
    float		stddevPos( int dim ) const override
			{ return dim ? std1_ : std0_; }
    float		getCorrelation() const	{ return cc_; }

private:

    bool		hasLims() const override	{ return false; }
    float		gtVal(float,float) const override;
    void		drwRandPos(float&,float&) const override;

    float		exp0_ = 0.f, exp1_ = 0.f;
    float		std0_ = 1.f, std1_ = 1.f;
    float		cc_ = 0.f;

    mutable Stats::NormalRandGen* rgen0_ = nullptr;
    mutable Stats::NormalRandGen* rgen1_ = nullptr;

};


/*!\brief Multi-dimensional pure Gaussian PDF. */

mExpClass(Algo) GaussianNDProbDenFunc : public ProbDenFunc
{
public:

			GaussianNDProbDenFunc(int nrdims=3);
			GaussianNDProbDenFunc(const GaussianNDProbDenFunc& oth)
						{ *this = oth; }
			mDefGaussianProbDenFuncFns(GaussianND)

    int			nrDims() const override;
    bool		hasLimits() const override	{ return false; }
    float		averagePos(int) const override;
    float		stddevPos(int) const override;

    void		prepareRandDrawing() const override;
    void		drawRandomPos(TypeSet<float>&) const override;
    float		value(const TypeSet<float>&) const override;
			//!< Not properly implemented because it can't be done

    mExpClass(Algo) VarDef
    {
    public:

			VarDef( float e=0.f, float s=1.f )
			    : exp_(e), std_(s)	{}
	bool		operator ==( const VarDef& oth ) const
			{ return exp_ == oth.exp_ && std_ == oth.std_; }

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

private:

    mutable ObjectSet<Stats::NormalRandGen> rgens_;
    Array2DMatrix<float>*	cholesky_ = nullptr;

};
