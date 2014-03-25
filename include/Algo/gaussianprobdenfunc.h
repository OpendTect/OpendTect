#ifndef gaussianprobdenfunc_h
#define gaussianprobdenfunc_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2010
 RCS:		$Id$
________________________________________________________________________


*/

#include "algomod.h"
#include "probdenfunc.h"
#include "samplingdata.h"
#include "arrayndimpl.h"
#include "bufstringset.h"


#define mDefGaussianProbDenFuncFns(nm) \
    virtual nm##ProbDenFunc*	clone() const \
				{ return new nm##ProbDenFunc(*this); } \
    static const char*		typeStr()		{ return #nm; } \
    virtual const char*		getTypeStr() const	{ return typeStr(); }


/*!\brief One dimensional Gaussian PDF. */

mExpClass(Algo) Gaussian1DProbDenFunc : public ProbDenFunc1D
{
public:

			Gaussian1DProbDenFunc( float exp=0, float stdev=1 )
			    : exp_(exp), std_(stdev)	{}

    virtual void	copyFrom(const ProbDenFunc&);
			mDefGaussianProbDenFuncFns(Gaussian1D)

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);
    virtual void	dump(od_ostream&,bool binary) const;
    virtual bool	obtain(od_istream&,bool binary);

    float		exp_;
    float		std_;

protected:

    virtual float	gtAvgPos() const		{ return exp_; }
    virtual float	gtVal(float) const;
    virtual void	drwRandPos(float&) const;

};


/*!\brief Two dimensional Gaussian PDF. */

mExpClass(Algo) Gaussian2DProbDenFunc : public ProbDenFunc2D
{
public:

			Gaussian2DProbDenFunc()
			    : exp0_(0),exp1_(0), std0_(1), std1_(1), cc_(0) {}
    virtual void	copyFrom(const ProbDenFunc&);
			mDefGaussianProbDenFuncFns(Gaussian2D)

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);
    virtual void	dump(od_ostream&,bool binary) const;
    virtual bool	obtain(od_istream&,bool binary);
    virtual float	averagePos( int dim ) const
			{ return dim ? exp1_ : exp0_; }

    float		exp0_, exp1_;
    float		std0_, std1_;
    float		cc_;

protected:

    virtual float	gtVal(float,float) const;
    virtual void	drwRandPos(float&,float&) const;

};


/*!\brief Multi-dimensional pure Gaussian PDF. */

mExpClass(Algo) GaussianNDProbDenFunc : public ProbDenFunc
{
public:

			GaussianNDProbDenFunc(int nrdims);
			~GaussianNDProbDenFunc();
    virtual void	copyFrom(const ProbDenFunc&);
			mDefGaussianProbDenFuncFns(GaussianND)

    virtual int		nrDims() const		{ return vars_.size(); }
    virtual const char*	dimName(int) const;
    virtual void	setDimName(int,const char*);
    virtual float	averagePos(int) const;

    void		prepareRandDrawing() const;
    virtual void	drawRandomPos(TypeSet<float>&) const;
    virtual float	value(const TypeSet<float>&) const;
			//!< Not properly implemented because it can't be done

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);
    virtual void	dump(od_ostream&,bool binary) const;
    virtual bool	obtain(od_istream&,bool binary);

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
	bool		operator ==( const Corr& c ) const
			{ return idx0_ == c.idx0_ && idx1_ == c.idx1_; }

	int		idx0_, idx1_;
	float		cc_;

    };

    TypeSet<VarDef>	vars_;
    TypeSet<Corr>	corrs_;

protected:


    ObjectSet<TypeSet<int> > corrs4vars_;

};


#endif
