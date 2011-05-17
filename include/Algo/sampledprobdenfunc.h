#ifndef sampledprobdenfunc_h
#define sampledprobdenfunc_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2010
 RCS:		$Id: sampledprobdenfunc.h,v 1.15 2011-05-17 08:51:59 cvsbert Exp $
________________________________________________________________________


*/

#include "probdenfunc.h"
#include "samplingdata.h"
#include "arrayndimpl.h"
#include "bufstringset.h"

class IOPar;

/*!\brief PDF based on ArrayND implementation.

  This interface should allow read/write generalized from disk.

*/

mClass ArrayNDProbDenFunc
{
public:

    virtual				~ArrayNDProbDenFunc()	{}

    int					size( int dim ) const
		{ return getArrND().info().getSize(dim); }
    od_uint64				totalSize() const
		{ return getArrND().info().getTotalSz(); }

    virtual const ArrayND<float>&	getData() const
		{ return getArrND(); }
    virtual ArrayND<float>&		getData()
    		{ return const_cast<ArrayND<float>&>(getArrND()); }
    virtual ArrayND<float>*		getArrClone() const	= 0;

    virtual SamplingData<float>		sampling( int dim ) const
		{ return getSampling(dim); }
    virtual SamplingData<float>&	sampling( int dim )
    		{ return const_cast<SamplingData<float>&>(getSampling(dim)); }

    void				fillPar(IOPar&) const;
    bool				usePar(const IOPar&);
    void				dump(std::ostream&,bool binary) const;
    bool				obtain(std::istream&,bool binary);

protected:

    virtual const ArrayND<float>&	getArrND() const	= 0;
    virtual const SamplingData<float>&	getSampling(int) const	= 0;
    virtual float			getNormFac() const;
    virtual void			doScale(float);

};


#define mDefSampledProbDenFuncClone(clss) \
    clss* clone() const	{ return new clss(*this); }

#define mDefArrayNDProbDenFuncFns(nm) \
    nm##ProbDenFunc*	clone() const	{ return new nm##ProbDenFunc(*this); } \
    static const char*	typeStr()	{ return #nm; } \
    virtual const char*	getTypeStr() const	{ return typeStr(); } \
    float		normFac() const		{ return getNormFac(); } \
    bool		canScale() const	{ return true; } \
    void		scale( float f )	{ doScale(f); }


mClass Sampled1DProbDenFunc : public ProbDenFunc1D
			    , public ArrayNDProbDenFunc
{
public:

    			Sampled1DProbDenFunc(const Array1D<float>&);
    			Sampled1DProbDenFunc(const TypeSet<float>&);
    			Sampled1DProbDenFunc(const float*,int);
    			Sampled1DProbDenFunc(const Sampled1DProbDenFunc&);
    Sampled1DProbDenFunc& operator =(const Sampled1DProbDenFunc&);
    virtual void	copyFrom(const ProbDenFunc&);
    			mDefArrayNDProbDenFuncFns(Sampled1D)

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);
    virtual void	dump(std::ostream&,bool binary) const;
    virtual bool	obtain(std::istream&,bool binary);
    virtual ArrayND<float>* getArrClone() const	
    			{ return new Array1DImpl<float>(bins_); }

    SamplingData<float>	sd_;
    Array1DImpl<float>	bins_;

    virtual void	prepareRandDrawing() const;

protected:

    virtual const ArrayND<float>&	getArrND() const	{ return bins_;}
    virtual const SamplingData<float>&	getSampling(int) const	{ return sd_; }

    virtual float	gtVal(float) const;
    virtual void	drwRandPos(float&) const;

    float*		cumbins_;
    void		fillCumBins() const;

};


mClass Sampled2DProbDenFunc : public ProbDenFunc2D
			    , public ArrayNDProbDenFunc
{
public:

    			Sampled2DProbDenFunc(const Array2D<float>&);
    			Sampled2DProbDenFunc(const Sampled2DProbDenFunc&);
    Sampled2DProbDenFunc& operator =(const Sampled2DProbDenFunc&);
    virtual void	copyFrom(const ProbDenFunc&);
    			mDefArrayNDProbDenFuncFns(Sampled2D)

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);
    virtual void	dump(std::ostream&,bool binary) const;
    virtual bool	obtain(std::istream&,bool binary);
    virtual ArrayND<float>* getArrClone() const	
    			{ return new Array2DImpl<float>(bins_); }

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


/*!\brief Multi-dimensional PDF based on binned data.

  If the 'dimnms_' are not filled, 'Dim0', 'Dim1' ... etc. will be returned.

 */


mClass SampledNDProbDenFunc : public ProbDenFunc
			    , public ArrayNDProbDenFunc
{
public:

    			SampledNDProbDenFunc(const ArrayND<float>&);
    			SampledNDProbDenFunc(const SampledNDProbDenFunc&);
    SampledNDProbDenFunc& operator =(const SampledNDProbDenFunc&);
    virtual void	copyFrom(const ProbDenFunc&);
    			mDefArrayNDProbDenFuncFns(SampledND)

    virtual int		nrDims() const	{ return bins_.info().getNDim(); }
    virtual const char*	dimName(int) const;
    virtual void	setDimName( int dim, const char* nm )
					{ *dimnms_[dim] = nm; }
    virtual float	value(const TypeSet<float>&) const;
    virtual void	drawRandomPos(TypeSet<float>&) const;
    virtual ArrayND<float>* getArrClone() const	
    			{ return new ArrayNDImpl<float>(bins_); }

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);
    virtual void	dump(std::ostream&,bool binary) const;
    virtual bool	obtain(std::istream&,bool binary);

    TypeSet< SamplingData<float> >	sds_;
    ArrayNDImpl<float>			bins_;
    BufferStringSet			dimnms_;

protected:

    virtual const ArrayND<float>&	getArrND() const	{ return bins_;}
    virtual const SamplingData<float>&	getSampling( int d ) const
					{ return sds_[d]; }

public:

					SampledNDProbDenFunc();

};


#endif
