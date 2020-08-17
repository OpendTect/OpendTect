#pragma once

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


/*!
\brief PDF based on ArrayND implementation.

  This interface should allow read/write generalized from disk.
*/

mExpClass(Algo) ArrayNDProbDenFunc
{
public:
    virtual		~ArrayNDProbDenFunc()	{ delete [] cumbins_; }
    ArrayNDProbDenFunc&	operator =(const ArrayNDProbDenFunc&);

    int			size( int dim ) const
			{ return getArrND().info().getSize(dim); }
    od_uint64		totalSize() const
			{ return getArrND().info().getTotalSz(); }

    virtual const ArrayND<float>& getData() const
			{ return getArrND(); }
    virtual ArrayND<float>& getData()
			{ return const_cast<ArrayND<float>&>(getArrND()); }
    virtual ArrayND<float>* getArrClone() const	= 0;

    virtual SamplingData<float>	sampling( int dim ) const
			{ return getSampling(dim); }
    virtual SamplingData<float>& sampling( int dim )
			{ return const_cast<SamplingData<float>&>(
							getSampling(dim)); }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		writeBulkData(od_ostream&,bool) const;
    bool		readBulkData(od_istream&,bool);

    float		getAveragePos(int dim) const;
    static float	findAveragePos(const float*,int,float grandtotal);

protected:
			ArrayNDProbDenFunc()
			: cumbins_(nullptr)		{}
			ArrayNDProbDenFunc( const ArrayNDProbDenFunc& oth)
			: cumbins_(nullptr)		{ *this = oth; }

    virtual const ArrayND<float>& getArrND() const	= 0;
    virtual const SamplingData<float>& getSampling(int) const	= 0;
    virtual float	getNormFac() const;
    virtual void	doScale(float);
    virtual bool	gtIsEq(const ProbDenFunc&) const;

    mutable float*	cumbins_;
    mutable TypeSet<float> avgpos_;

    void		prepRndDrw() const;
    void		fillCumBins() const;
    od_uint64		getRandBin() const;
    od_uint64		getBinPos(float) const;

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

			Sampled1DProbDenFunc();
			Sampled1DProbDenFunc(const Array1D<float>&);
			Sampled1DProbDenFunc(const TypeSet<float>&);
			Sampled1DProbDenFunc(const float*,int);
			Sampled1DProbDenFunc(const Sampled1DProbDenFunc&);
    Sampled1DProbDenFunc& operator =(const Sampled1DProbDenFunc&);
    virtual void	copyFrom(const ProbDenFunc&);
			mDefArrayNDProbDenFuncFns(Sampled1D)

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);
    virtual void	writeBulk(od_ostream&,bool binary) const;
    virtual bool	readBulk(od_istream&,bool binary);
    virtual ArrayND<float>* getArrClone() const
			{ return new Array1DImpl<float>(bins_); }

    SamplingData<float>	sd_;
    Array1DImpl<float>	bins_;

protected:

    virtual const ArrayND<float>&	getArrND() const	{ return bins_;}
    virtual const SamplingData<float>&	getSampling(int) const	{ return sd_; }

    virtual float	gtAvgPos() const;
    virtual float	gtVal(float) const;
    virtual void	drwRandPos(float&) const;

};


/*!
\brief Two dimensional PDF based on binned data.
*/

mExpClass(Algo) Sampled2DProbDenFunc : public ProbDenFunc2D
				     , public ArrayNDProbDenFunc
{
public:

			Sampled2DProbDenFunc();
			Sampled2DProbDenFunc(const Array2D<float>&);
			Sampled2DProbDenFunc(const Sampled2DProbDenFunc&);
    Sampled2DProbDenFunc& operator =(const Sampled2DProbDenFunc&);
    virtual void	copyFrom(const ProbDenFunc&);
			mDefArrayNDProbDenFuncFns(Sampled2D)

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);
    virtual void	writeBulk(od_ostream&,bool binary) const;
    virtual bool	readBulk(od_istream&,bool binary);
    virtual ArrayND<float>* getArrClone() const
			{ return new Array2DImpl<float>(bins_); }
    virtual float	averagePos( int dim ) const
			{ return getAveragePos( dim ); }

    SamplingData<float>	sd0_;
    SamplingData<float>	sd1_;
    Array2DImpl<float>	bins_;

protected:

    virtual const ArrayND<float>&	getArrND() const	{ return bins_;}
    virtual const SamplingData<float>&	getSampling( int d ) const
					{ return d ? sd1_ : sd0_; }

    virtual float	gtVal(float,float) const;
    virtual void	drwRandPos(float&,float&) const;

};


/*!
\brief Multi-dimensional PDF based on binned data.

  If the 'dimnms_' are not filled, 'Dim0', 'Dim1' ... etc. will be returned.
*/

mExpClass(Algo) SampledNDProbDenFunc : public ProbDenFunc
				     , public ArrayNDProbDenFunc
{
public:

			SampledNDProbDenFunc(int nrdims);
			SampledNDProbDenFunc(const ArrayND<float>&);
			SampledNDProbDenFunc(const SampledNDProbDenFunc&);
    SampledNDProbDenFunc& operator =(const SampledNDProbDenFunc&);
    virtual void	copyFrom(const ProbDenFunc&);
			mDefArrayNDProbDenFuncFns(SampledND)

    virtual int		nrDims() const	{ return bins_.info().getNDim(); }
    virtual const char*	dimName(int) const;
    virtual void	setDimName( int dim, const char* nm )
					{ *dimnms_[dim] = nm; }
    virtual float	averagePos( int dim ) const
			{ return getAveragePos( dim ); }
    virtual float	value(const TypeSet<float>&) const;
    virtual void	drawRandomPos(TypeSet<float>&) const;
    virtual ArrayND<float>* getArrClone() const
			{ return new ArrayNDImpl<float>(bins_); }

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);
    virtual void	writeBulk(od_ostream&,bool binary) const;
    virtual bool	readBulk(od_istream&,bool binary);

    TypeSet< SamplingData<float> > sds_;
    ArrayNDImpl<float>	bins_;
    BufferStringSet	dimnms_;

protected:

    virtual const ArrayND<float>& getArrND() const
			{ return bins_;}
    virtual const SamplingData<float>& getSampling( int d ) const
			{ return sds_[d]; }

public:

			SampledNDProbDenFunc();

};


