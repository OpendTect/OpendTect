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
#include "bufstringset.h"


/*!
\brief PDF based on ArrayND implementation.

  This interface should allow read/write generalized from disk.
*/

mExpClass(Algo) ArrayNDProbDenFunc
{
public:
    virtual		~ArrayNDProbDenFunc();
    ArrayNDProbDenFunc&	operator =(const ArrayNDProbDenFunc&);

    int			size( int dim ) const
			{ return getArrND().info().getSize(dim); }
    od_uint64		totalSize() const
			{ return getArrND().info().getTotalSz(); }
    const ArrayND<float>& getData() const		{ return getArrND(); }

    bool		setSize(const TypeSet<int>&);
    bool		setSize(const int*,int sz);
    ArrayND<float>&	getData()
			{ return const_cast<ArrayND<float>&>(getArrND()); }
    ArrayND<float>*	getArrClone() const;

    const SamplingData<float>& sampling( int dim ) const
			{ return getSampling(dim); }
    SamplingData<float>& sampling(int dim);

    Interval<float>	getRange(int dim) const;
    void		setRange(int dim,const StepInterval<float>&);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		writeBulkData(od_ostream&,bool) const;
    bool		readBulkData(od_istream&,bool);

    float		getAveragePos(int dim) const;
    static float	findAveragePos(const float*,int,float grandtotal);

protected:
			ArrayNDProbDenFunc(int nrdims);
			ArrayNDProbDenFunc( const ArrayNDProbDenFunc& oth )
							{ *this = oth; }

    virtual const ArrayND<float>& getArrND() const	= 0;
    virtual float	getNormFac() const;
    virtual void	doScale(float);
    virtual bool	gtIsEq(const ProbDenFunc&) const;

    mutable float*	cumbins_ = nullptr;

    void		prepRndDrw() const;
    void		fillCumBins() const;
    od_uint64		getRandBin() const;
    od_uint64		getBinPos(float) const;

    int			nrDims_() const { return getData().info().getNDim(); }

private:

    const SamplingData<float>& getSampling(int) const;

    TypeSet<SamplingData<float> > sds_;
    mutable TypeSet<float> avgpos_;

};


#define mDefArrayNDProbDenFuncFns(nm) \
    virtual nm##ProbDenFunc*	clone() const \
				{ return new nm##ProbDenFunc(*this); } \
    static const char*		typeStr()		{ return #nm; } \
    virtual const char*		getTypeStr() const	{ return typeStr(); } \
    virtual float		normFac() const		{ return getNormFac();}\
    virtual bool		canScale() const	{ return true; } \
    virtual void		scale( float f )	{ doScale(f); } \
    virtual void		prepareRandDrawing() const { prepRndDrw(); } \
    virtual bool		isEq( const ProbDenFunc& oth ) const \
							{ return gtIsEq(oth); }


/*!
\brief One dimensional PDF based on binned data.
*/

mExpClass(Algo) Sampled1DProbDenFunc : public ProbDenFunc1D
				     , public ArrayNDProbDenFunc
{
public:

			Sampled1DProbDenFunc(const char* vnm="");
			Sampled1DProbDenFunc(const Array1D<float>&);
			Sampled1DProbDenFunc(const TypeSet<float>&);
			Sampled1DProbDenFunc(const float*,int);
			Sampled1DProbDenFunc(const Sampled1DProbDenFunc&);
    Sampled1DProbDenFunc& operator =(const Sampled1DProbDenFunc&);
    void		copyFrom(const ProbDenFunc&) override;
			mDefArrayNDProbDenFuncFns(Sampled1D)

    const SamplingData<float>& sampling() const;
    SamplingData<float>& sampling();

    float		get( int idx ) const	{ return bins_.get( idx ); }
    void		set( int idx, float val )
			{ bins_.set( idx, val ); }

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;
    void		writeBulk(od_ostream&,bool binary) const override;
    bool		readBulk(od_istream&,bool binary) override;

private:

    const ArrayND<float>&	getArrND() const override { return bins_; }

    float		gtAvgPos() const override;
    float		gtStdDevPos() const override	{ return mUdf(float); }
    float		gtVal(float) const override;
    void		drwRandPos(float&) const override;

    Array1DImpl<float>	bins_;

};


/*!
\brief Two dimensional PDF based on binned data.
*/

mExpClass(Algo) Sampled2DProbDenFunc : public ProbDenFunc2D
				     , public ArrayNDProbDenFunc
{
public:

			Sampled2DProbDenFunc(const char* vnm0= "",
					     const char* vnm1= "");
			Sampled2DProbDenFunc(const Array2D<float>&);
			Sampled2DProbDenFunc(const Sampled2DProbDenFunc&);
    Sampled2DProbDenFunc& operator =(const Sampled2DProbDenFunc&);
    void		copyFrom(const ProbDenFunc&) override;
			mDefArrayNDProbDenFuncFns(Sampled2D)

    float		get( int idx, int idy ) const
			{ return bins_.get( idx, idy ); }
    void		set(int idx,int idy, float val )
			{ bins_.set( idx, idy, val ); }

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;
    void		writeBulk(od_ostream&,bool binary) const override;
    bool		readBulk(od_istream&,bool binary) override;
    float		averagePos( int dim ) const override
			{ return getAveragePos( dim ); }
    float		stddevPos( int dim ) const override
			{ return mUdf(float); }

private:


    const ArrayND<float>&	getArrND() const override { return bins_; }

    float		gtVal(float,float) const override;
    void		drwRandPos(float&,float&) const override;

    Array2DImpl<float>	bins_;

};


/*!
\brief Multi-dimensional PDF based on binned data.
*/

mExpClass(Algo) SampledNDProbDenFunc : public ProbDenFunc
				     , public ArrayNDProbDenFunc
{
public:

			SampledNDProbDenFunc(int nrdims);
			SampledNDProbDenFunc(const ArrayND<float>&);
			SampledNDProbDenFunc(const SampledNDProbDenFunc&);
    SampledNDProbDenFunc& operator =(const SampledNDProbDenFunc&);
    void		copyFrom(const ProbDenFunc&) override;
			mDefArrayNDProbDenFuncFns(SampledND)

    int			nrDims() const override
			{ return ArrayNDProbDenFunc::nrDims_(); }

    void		setND( int* idxs, float val )
			{ bins_.setND( idxs, val ); }

    float		averagePos( int dim ) const override
			{ return getAveragePos( dim ); }
    float		stddevPos(int) const override	{ return mUdf(float); }
    float		value(const TypeSet<float>&) const override;
    void		drawRandomPos(TypeSet<float>&) const override;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;
    void		writeBulk(od_ostream&,bool binary) const override;
    bool		readBulk(od_istream&,bool binary) override;


private:

    const ArrayND<float>& getArrND() const override	{ return bins_; }

    ArrayNDImpl<float>	bins_;

public:

			SampledNDProbDenFunc(); //TODO: remove ?

};


