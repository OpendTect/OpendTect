#ifndef sampledprobdenfunc_h
#define sampledprobdenfunc_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2010
 RCS:		$Id: sampledprobdenfunc.h,v 1.1 2010-01-18 16:13:15 cvsbert Exp $
________________________________________________________________________


*/

#include "probdenfunc.h"
#include "samplingdata.h"
#include "arrayndimpl.h"


/*!\brief PDF based on ArrayND implementation.

  This interface should allow read/write generalized from disk.

*/

mClass ArrayNDProbDenFunc
{
public:

    int					size( int dim ) const
		{ return getArrND().info().getSize(dim); }

    virtual const ArrayND<float>&	getData() const
		{ return getArrND(); }
    virtual ArrayND<float>&		getData()
    		{ return const_cast<ArrayND<float>&>(getArrND()); }

    virtual SamplingData<float>		sampling( int dim ) const
		{ return getSampling(dim); }
    virtual SamplingData<float>&	sampling( int dim )
    		{ return const_cast<SamplingData<float>&>(getSampling(dim)); }
				

protected:

    virtual const ArrayND<float>&	getArrND() const	= 0;
    virtual const SamplingData<float>&	getSampling(int) const	= 0;

};


mClass SampledProbDenFunc1D : public ProbDenFunc1D
			    , public ArrayNDProbDenFunc
{
public:

    			SampledProbDenFunc1D(const Array1D<float>&);
    			SampledProbDenFunc1D(const TypeSet<float>&);
    			SampledProbDenFunc1D(const float*,int);
    			SampledProbDenFunc1D(const SampledProbDenFunc1D&);

    virtual float	value(float) const;

    SamplingData<float>	sd_;
    Array1DImpl<float>	bins_;

protected:

    virtual const ArrayND<float>&	getArrND() const	{ return bins_;}
    virtual const SamplingData<float>&	getSampling(int) const	{ return sd_; }

};


mClass SampledProbDenFunc2D : public ProbDenFunc2D
			    , public ArrayNDProbDenFunc
{
public:
    			SampledProbDenFunc2D(const Array2D<float>&);
    			SampledProbDenFunc2D(const SampledProbDenFunc2D&);

    virtual float	value(float,float) const;

    SamplingData<float>	sd0_;
    SamplingData<float>	sd1_;
    Array2DImpl<float>	bins_;

protected:

    virtual const ArrayND<float>&	getArrND() const	{ return bins_;}
    virtual const SamplingData<float>&	getSampling( int d ) const
					{ return d ? sd1_ : sd0_; }

};


mClass SampledProbDenFuncND : public ProbDenFunc
			    , public ArrayNDProbDenFunc
{
public:

    			SampledProbDenFuncND(const ArrayND<float>&);
    			SampledProbDenFuncND(const SampledProbDenFuncND&);

    virtual int		nrDims() const;
    virtual const char*	dimName(int) const;
    virtual float	value(const TypeSet<float>&) const;

    TypeSet< SamplingData<float> >	sds_;
    ArrayNDImpl<float>			bins_;

protected:

    virtual const ArrayND<float>&	getArrND() const	{ return bins_;}
    virtual const SamplingData<float>&	getSampling( int d ) const
					{ return sds_[d]; }

};


#endif
