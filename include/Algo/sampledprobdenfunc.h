#ifndef sampledprobdenfunc_h
#define sampledprobdenfunc_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2010
 RCS:		$Id: sampledprobdenfunc.h,v 1.8 2010-03-04 15:28:41 cvsbert Exp $
________________________________________________________________________


*/

#include "probdenfunc.h"
#include "samplingdata.h"
#include "arrayndimpl.h"

class IOPar;

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

};


#define mDefSampledProbDenFuncClone(clss) \
    clss* clone() const	{ return new clss(*this); }


mClass SampledProbDenFunc1D : public ProbDenFunc1D
			    , public ArrayNDProbDenFunc
{
public:

    			SampledProbDenFunc1D(const Array1D<float>&);
    			SampledProbDenFunc1D(const TypeSet<float>&);
    			SampledProbDenFunc1D(const float*,int);
    			SampledProbDenFunc1D(const SampledProbDenFunc1D&);
    SampledProbDenFunc1D& operator =(const SampledProbDenFunc1D&);
    			mDefSampledProbDenFuncClone(SampledProbDenFunc1D)
    virtual void	copyFrom(const ProbDenFunc&);

    virtual float	value(float) const;

    static const char*	typeStr()			{ return "Sampled1D"; }
    virtual const char*	getTypeStr() const		{ return typeStr(); }

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);
    virtual void	dump(std::ostream&,bool binary) const;
    virtual bool	obtain(std::istream&,bool binary);
    virtual ArrayND<float>* getArrClone() const	
    			{ return new Array1DImpl<float>(bins_); }

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
    SampledProbDenFunc2D& operator =(const SampledProbDenFunc2D&);
    			mDefSampledProbDenFuncClone(SampledProbDenFunc2D)
    virtual void	copyFrom(const ProbDenFunc&);

    virtual float	value(float,float) const;

    static const char*	typeStr()			{ return "Sampled2D"; }
    virtual const char*	getTypeStr() const		{ return typeStr(); }

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

};


/*!\brief Multi-dimensional PDF based on binned data.

  If the 'dimnms_' are not filled, 'Dim0', 'Dim1' ... etc. will be returned.

 */


mClass SampledProbDenFuncND : public ProbDenFunc
			    , public ArrayNDProbDenFunc
{
public:

    			SampledProbDenFuncND(const ArrayND<float>&);
    			SampledProbDenFuncND(const SampledProbDenFuncND&);
			SampledProbDenFuncND();
    SampledProbDenFuncND& operator =(const SampledProbDenFuncND&);
    			mDefSampledProbDenFuncClone(SampledProbDenFuncND)
    virtual void	copyFrom(const ProbDenFunc&);

    virtual int		nrDims() const	{ return bins_.info().getNDim(); }
    virtual const char*	dimName(int) const;
    virtual void	setDimName( int dim, const char* nm )
					{ *dimnms_[dim] = nm; }
    virtual float	value(const TypeSet<float>&) const;
    virtual ArrayND<float>* getArrClone() const	
    			{ return new ArrayNDImpl<float>(bins_); }

    static const char*	typeStr()			{ return "SampledND"; }
    virtual const char*	getTypeStr() const		{ return typeStr(); }

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

};


#endif
