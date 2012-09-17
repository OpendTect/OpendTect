#ifndef convolveattrib_h
#define convolveattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: convolveattrib.h,v 1.18 2011/01/06 15:25:01 cvsbert Exp $
________________________________________________________________________

    
-*/

#include "attribprovider.h"

class Wavelet;

/*!\brief Convolution Attribute

Convolve [kernel=LowPass|Laplacian|Prewitt] [shape=Sphere] [size=3]

Convolve convolves a signal with the on the command-line specified signal.

Kernel:         Uses Shape      Uses Size       Desc

LowPass         Yes             Yes             A basic averaging kernel.
Laplacian       Yes             Yes             A laplacian kernel (signal-avg).
Prewitt         No              No              A 3x3x3 gradient filter with
                                                three subkernels: 1 (inl),
						2 (crl) and 3 (time).

Inputs:
0       Signal to be convolved.

Outputs:
0       Sum of the convolution with all kernels / N
1       Subkernel 1
.
.
.
N       Subkernel N

*/

namespace Attrib
{

mClass Convolve : public Provider
{
public:
    static void			initClass();
				Convolve(Desc&);

    static const char*		attribName()		{ return "Convolve"; }

    static const char*		kernelStr()		{ return "kernel"; }
    static const char*		shapeStr()		{ return "shape"; }
    static const char*		sizeStr()		{ return "size"; }
    static const char*		waveletStr()		{ return "waveletid"; }
    static const char*		kernelTypeStr(int);
    static const char*  	shapeTypeStr(int);

    static const float  	prewitt[];
    static const float  	prewitt2D[];

    virtual bool		isSingleTrace() const;

protected:
    				~Convolve();

    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			allowParallelComputation() const;
    bool			getInputOutput(int input,
	    				       TypeSet<int>& res) const;
    bool			getInputData(const BinID&,int idx);
    bool			computeDataKernel(const DataHolder&,
						  int t0, int nrsamples ) const;
    bool			computeDataWavelet(const DataHolder&,
					    	  int t0, int nrsamples ) const;
    bool			computeData(const DataHolder&,const BinID& rel,
					    int t0,int nrsamples,
					    int threadid) const;

    const BinID*		reqStepout(int input,int output) const;
    const Interval<int>*	reqZSampMargin(int input,int output) const;
    const Interval<int>*	desZSampMargin(int input,int output) const;

    int				kerneltype_;
    int				shape_;
    int				size_;
    BinID			stepout_;
    Wavelet*			wavelet_;
    Interval<int>		dessampgate_;

    int				dataidx_;

    ObjectSet<const DataHolder>	inputdata_;

    mClass Kernel
    {
    public:
	const float*            getKernel() const;
	int                     nrSubKernels() const;
	const BinID&            getStepout() const;
	const Interval<int>&    getSG() const;
	int                     getSubKernelSize() const;
	float                   getSum() const { return sum_; }

				Kernel(int kernelfunc,int shape,int size, bool);
				~Kernel();

    protected:
	float*			kernel_;
	int			nrsubkernels_;
	BinID			stepout_;
	Interval<int>		sg_;
	float			sum_;
    };

    Kernel*			kernel_;
};

}; // namespace Attrib


#endif

