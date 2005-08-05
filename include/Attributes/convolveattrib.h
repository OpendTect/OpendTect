#ifndef convolveattrib_h
#define convolveattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: convolveattrib.h,v 1.4 2005-08-05 10:51:52 cvshelene Exp $
________________________________________________________________________

    
-*/

#include "attribprovider.h"

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

class Convolve : public Provider
{
public:
    static void		initClass();
			Convolve( Desc& );

    static const char*	attribName()	{ return "Convolve"; }
    static const char*	kernelStr()	{ return "kernel"; }
    static const char*	shapeStr()	{ return "shape"; }
    static const char*	sizeStr()	{ return "size"; }
    static const char*	kernelTypeStr(int);
    static const char*  shapeTypeStr(int);

    static const float  prewitt[];

protected:
			~Convolve();
    static Provider*	createInstance( Desc& );
    static void		updateDesc( Desc& );

    static Provider*	internalCreate( Desc&, ObjectSet<Provider>& existing );

    bool		getInputOutput( int input, TypeSet<int>& res ) const;
    bool		getInputData( const BinID&, int idx );
    bool		computeData( const DataHolder&, const BinID& relpos,
	    			     int t0, int nrsamples ) const;

    const BinID*		reqStepout( int input, int output ) const;
    const Interval<float>*	reqZMargin(int input, int output) const;

    int 		kerneltype;
    int 		shape;
    int 		size;
    BinID		stepout;
    Interval<float>	interval;

    ObjectSet<const DataHolder>	inputdata;

    class               Kernel
    {
    public:
	const float*            getKernel( ) const;
	int                     nrSubKernels() const;
	const BinID&            getStepout() const;
	const Interval<int>&    getSG() const;
	int                     getSubKernelSize() const;
	float                   getSum() const { return sum; }

				Kernel( int kernelfunc, int shape, int size );
				~Kernel();

    protected:
	float*          kernel;
	int             nrsubkernels;
	BinID           stepout;
	Interval<int>   sg;

	float           sum;

    };

    Kernel              kernel;
};

}; // namespace Attrib


#endif

