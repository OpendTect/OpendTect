/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "convolveattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "ptrman.h"
#include "ioman.h"
#include "ioobj.h"
#include "wavelet.h"
#include "genericnumer.h"


#define mShapeCube	0
#define mShapeSphere    1

#define mKernelFunctionLowPass		0
#define mKernelFunctionLaplacian	1
#define mKernelFunctionPrewitt		2
#define mKernelFunctionWavelet		3

namespace Attrib
{

mAttrDefCreateInstance(Convolve)
    
void Convolve::initClass()
{
    mAttrStartInitClassWithUpdate

    EnumParam* kernel = new EnumParam( kernelStr() );
    //Note: Ordering must be the same as numbering!
    kernel->addEnum( kernelTypeStr(mKernelFunctionLowPass) );
    kernel->addEnum( kernelTypeStr(mKernelFunctionLaplacian) );
    kernel->addEnum( kernelTypeStr(mKernelFunctionPrewitt) );
    kernel->addEnum( kernelTypeStr(mKernelFunctionWavelet) );
    kernel->setDefaultValue( mKernelFunctionLowPass );
    desc->addParam( kernel );

    EnumParam* shape = new EnumParam( shapeStr() );
    //Note: Ordering must be the same as numbering!
    shape->addEnum( shapeTypeStr(mShapeCube) );
    shape->addEnum( shapeTypeStr(mShapeSphere) );
    shape->setDefaultValue( mShapeSphere );
    desc->addParam( shape );

    IntParam* sizepar = new IntParam( sizeStr() );
    sizepar->setLimits( StepInterval<int>(3,30,2) );
    sizepar->setDefaultValue( 3 );
    desc->addParam( sizepar );

    StringParam* waveletid = new StringParam( waveletStr() );
    desc->addParam( waveletid );

    desc->addOutputDataType( Seis::UnknowData );
    desc->addInput( InputSpec("Signal to be convolved",true) );

    mAttrEndInitClass
}


const float Convolve::prewitt[] = 
{
	// Kernel 0 (Inline)
	-1,-1,-1,	/* Inline -1, Crossline = -1, Time = -dt -- +dt */
	-1,-1,-1,	/* Inline -1, Crossline =  0, Time = -dt -- +dt */
	-1,-1,-1,	/* Inline -1, Crossline = +1, Time = -dt -- +dt */

	0, 0, 0,	/* Inline  0, Crossline = -1, Time = -dt -- +dt */
	0, 0, 0,	/* Inline  0, Crossline =  0, Time = -dt -- +dt */
	0, 0, 0, 	/* Inline  0, Crossline = +1, Time = -dt -- +dt */

	1, 1, 1, 	/* Inline +1, Crossline = -1, Time = -dt -- +dt */
	1, 1, 1, 	/* Inline +1, Crossline =  0, Time = -dt -- +dt */
	1, 1, 1, 	/* Inline +1, Crossline = +1, Time = -dt -- +dt */

	// Kernel 1 (Crossline)
 
	-1,-1,-1,	/* Inline -1, Crossline = -1, Time = -dt -- +dt */
	0, 0, 0,	/* Inline -1, Crossline =  0, Time = -dt -- +dt */
	1, 1, 1,	/* Inline -1, Crossline = +1, Time = -dt -- +dt */

	-1,-1,-1,	/* Inline  0, Crossline = -1, Time = -dt -- +dt */
	0, 0, 0,	/* Inline  0, Crossline =  0, Time = -dt -- +dt */
	1, 1, 1, 	/* Inline  0, Crossline = +1, Time = -dt -- +dt */

	-1,-1,-1, 	/* Inline +1, Crossline = -1, Time = -dt -- +dt */
	0, 0, 0, 	/* Inline +1, Crossline =  0, Time = -dt -- +dt */
	1, 1, 1, 	/* Inline +1, Crossline = +1, Time = -dt -- +dt */ 

	// Kernel 2 (Time)
	-1, 0, 1,	/* Inline -1, Crossline = -1, Time = -dt -- +dt */
	-1, 0, 1,	/* Inline -1, Crossline =  0, Time = -dt -- +dt */
	-1, 0, 1,	/* Inline -1, Crossline = +1, Time = -dt -- +dt */

	-1, 0, 1,	/* Inline  0, Crossline = -1, Time = -dt -- +dt */
	-1, 0, 1,	/* Inline  0, Crossline =  0, Time = -dt -- +dt */
	-1, 0, 1, 	/* Inline  0, Crossline = +1, Time = -dt -- +dt */

	-1, 0, 1, 	/* Inline +1, Crossline = -1, Time = -dt -- +dt */
	-1, 0, 1, 	/* Inline +1, Crossline =  0, Time = -dt -- +dt */
	-1, 0, 1 	/* Inline +1, Crossline = +1, Time = -dt -- +dt */ 
};


const float Convolve::prewitt2D[] = 
{
	// Kernel 0 (Crossline = Trace number)
 
	-1,-1,-1, 	/* Trace nr = -1, Time = -dt -- +dt */
	0, 0, 0, 	/* Trace nr =  0, Time = -dt -- +dt */
	1, 1, 1, 	/* Trace nr = +1, Time = -dt -- +dt */ 

	// Kernel 1 (Time)

	-1, 0, 1, 	/* Trace nr = -1, Time = -dt -- +dt */
	-1, 0, 1, 	/* Trace nr =  0, Time = -dt -- +dt */
	-1, 0, 1 	/* Trace nr = +1, Time = -dt -- +dt */ 
};


void Convolve::updateDesc( Desc& desc )
{
    const ValParam* kernel = desc.getValParam( kernelStr() );
    bool isprewitt = !strcmp( kernel->getStringValue(0),
	    		      kernelTypeStr(mKernelFunctionPrewitt) );
    bool iswavelet = !strcmp( kernel->getStringValue(0),
	    		      kernelTypeStr(mKernelFunctionWavelet) );
    bool needsz = !(isprewitt || iswavelet);
    desc.setParamEnabled(sizeStr(),needsz);
    desc.setParamEnabled(shapeStr(),needsz);
    desc.setParamEnabled(waveletStr(),iswavelet);
    
    if ( isprewitt ) 
	desc.setNrOutputs( Seis::UnknowData, desc.is2D() ? 3 : 4 );
}


const char* Convolve::kernelTypeStr(int type)
{
    if ( type==mKernelFunctionLowPass ) return "LowPass";
    if ( type==mKernelFunctionLaplacian ) return "Laplacian";
    if ( type==mKernelFunctionPrewitt ) return "Prewitt";
    return "Wavelet";
}


const char* Convolve::shapeTypeStr(int type)
{
    if ( type==mShapeCube ) return "Cube";
    return "Sphere";
}


const float* Convolve::Kernel::getKernel(  ) const 
{ return kernel_; }


const BinID& Convolve::Kernel::getStepout( ) const
{ return stepout_; }


int Convolve::Kernel::nrSubKernels(  ) const
{ return nrsubkernels_; }


int Convolve::Kernel::getSubKernelSize() const
{
    int inl = 1 + stepout_.inl * 2;
    int crl = 1 + stepout_.crl * 2;
    int sgw = 1 + sg_.width();
    return inl * crl * sgw;
}


const Interval<int>& Convolve::Kernel::getSG( ) const
{ return sg_; }


Convolve::Kernel::Kernel( int kernelfunc, int shapetype, int size, bool is2d )
    : kernel_( 0 )
    , sum_( 0 )
    , nrsubkernels_( 1 )
{
    if ( kernelfunc == -1 && shapetype == -1 && size == 0 )
	return;

    if ( kernelfunc==mKernelFunctionLowPass || 
	    kernelfunc==mKernelFunctionLaplacian )
    {
	nrsubkernels_ = 1;
	const int hsz = size/2;

	stepout_ = is2d ? BinID(0,hsz) : BinID(hsz,hsz);
	sg_ = Interval<int>(-hsz,hsz);

	kernel_ = new float[getSubKernelSize()];
	const int value = kernelfunc==mKernelFunctionLowPass ? 1 : -1;

	const int limit2 = hsz*hsz;

	int pos = 0;

	for ( int inl=-stepout_.inl; inl<=stepout_.inl; inl++ )
	{
	    for ( int crl=-stepout_.crl; crl<=stepout_.crl; crl++ )
	    {
		for ( int tidx=sg_.start; tidx<=sg_.stop; tidx++ )
		{
		    const float nv =
			( shapetype==mShapeSphere && 
			  limit2<inl*inl+crl*crl+tidx*tidx )
			? 0.f : value;
		    kernel_[pos++] = nv;
		    sum_ += nv;
		}
	    }
	}

	if ( kernelfunc==mKernelFunctionLaplacian )
	{
	    kernel_[getSubKernelSize()/2] -= sum_;
	    sum_ = 0;
	}
    }
    else if ( kernelfunc==mKernelFunctionPrewitt )
    {
	nrsubkernels_ = is2d ? 2 : 3;
	stepout_ = is2d ? BinID(0,1) : BinID(1,1);
	sg_=Interval<int>(-1,1);
	int sz = getSubKernelSize()*nrSubKernels();
	kernel_ = new float[sz];
	if ( is2d )
	    memcpy( kernel_, Convolve::prewitt2D, sz*sizeof(float) );
	else
	    memcpy( kernel_, Convolve::prewitt, sz*sizeof(float) );
    }

    int subkernelsize = getSubKernelSize();

    for ( int idy=0; idy<nrSubKernels(); idy++ )
    {
	float subkernelsum = 0;
	for ( int idx=0; idx<subkernelsize; idx++ )
	    subkernelsum += kernel_[idy*subkernelsize+idx];

	if ( !mIsZero(subkernelsum,mDefEps) )
	{
	    for ( int idx=0; idx<subkernelsize; idx++ )
	    {
		kernel_[idy*subkernelsize+idx] /= subkernelsum;
	    }
	}

	sum_ += subkernelsum;
    }
}
    


Convolve::Kernel::~Kernel()
{ delete [] kernel_; }


Convolve::Convolve( Desc& ds )
    : Provider(ds)
    , shape_ (-1)
    , size_(0)
    , stepout_(0,0)
    , kernel_(0)
    , wavelet_(0)
{
    if ( !isOK() ) return;

    inputdata_.allowNull( true );

    mGetEnum( kerneltype_, kernelStr() );
    if ( kerneltype_ == mKernelFunctionLowPass || 
	    kerneltype_ == mKernelFunctionLaplacian )
    {
	mGetEnum( shape_, shapeStr() );
	mGetInt( size_, sizeStr() );
	if ( size_%2 == 0 )
	    size_++;
    }
    else if ( kerneltype_ == mKernelFunctionWavelet )
    {
	BufferString wavidstr;
	mGetString( wavidstr, waveletStr() );
	IOObj* ioobj = IOM().get( MultiID(wavidstr) );
	wavelet_ = Wavelet::get( ioobj );
	int wvletmididx = wavelet_->centerSample();
	dessampgate_ = Interval<int>( -wvletmididx, wvletmididx );
	return;
    }

    kernel_ = new Kernel( kerneltype_, shape_ , size_, is2D() );
    stepout_ = kernel_->getStepout();
}


Convolve::~Convolve()
{
    if ( kernel_ ) delete kernel_;
    if ( wavelet_ ) delete wavelet_;
}


bool Convolve::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Convolve::getInputData( const BinID& relpos, int idx )
{
    int sz = (1+stepout_.inl*2) * (1+stepout_.crl*2);
    
    while ( inputdata_.size()< sz )
	inputdata_ += 0;

    const BinID bidstep = inputs_[0]->getStepoutStep();
    BinID truepos;
    int index = 0;
    for (int inl=-stepout_.inl; inl<=stepout_.inl; inl++ )
    {
	for (int crl=-stepout_.crl; crl<=stepout_.crl; crl++ )
	{
	    truepos.inl = inl * bidstep.inl;
	    truepos.crl = crl * bidstep.crl;
	    const DataHolder* data = inputs_[0]->getData( relpos+truepos, idx );
	    if ( !data )
		return false;
	    inputdata_.replace( index++, data );
	}
    }
    dataidx_ = getDataIndex( 0 );

    return true;
}


const BinID* Convolve::reqStepout( int inp, int out ) const
{ return &stepout_; }


const Interval<int>* Convolve::reqZSampMargin( int inp, int ) const
{
    if ( !kernel_ ) return 0;
    return &kernel_->getSG();
}


const Interval<int>* Convolve::desZSampMargin( int inp, int ) const
{
    if ( kernel_ ) return 0;
    return &dessampgate_;
}


bool Convolve::computeDataKernel( const DataHolder& output, int z0,
				  int nrsamples ) const
{
    const int nrofkernels = kernel_->nrSubKernels();
    const int subkernelsz = kernel_->getSubKernelSize();
    const float* kernelvals = kernel_->getKernel();

    const int nrtraces = (1+stepout_.inl*2) * (1+stepout_.crl*2);

    ArrPtrMan<bool> docalculate = new bool[nrofkernels];
    const bool customcalc = !isOutputEnabled(0);
    for ( int idx=0; idx<nrofkernels; idx++ )
	docalculate[idx] = customcalc ? isOutputEnabled(idx+1) : true;

    const Interval<int> sg_ = kernel_->getSG();
    const int sgwidth = 1 + sg_.width();

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	ArrPtrMan<float> res = new float[nrofkernels];
	for ( int idy=0; idy<nrofkernels; idy++ )
	    res[idy] = 0;

	for ( int idy=0; idy<nrtraces; idy++)
	{
	    if ( !inputdata_[idy] )
		continue;
	    
	    const int valoffset = idy * sgwidth;

	    ArrPtrMan<float> vals = new float[sgwidth];
	    for ( int valindex=0; valindex<sgwidth; valindex++ )
		vals[valindex] = getInputValue( *inputdata_[idy], dataidx_,
					        idx+sg_.start+valindex, z0 );

	    for ( int kidx=0; kidx<nrofkernels; kidx++ )
	    {
		if ( customcalc && !docalculate[kidx] ) continue;

		int kerneloff = kidx*subkernelsz+valoffset;

		for ( int valindex=0; valindex<sgwidth; valindex++ )
		    res[kidx] += vals[valindex]*kernelvals[kerneloff+valindex];
	    }

	}

	if ( isOutputEnabled(0) )
	{
	    float ressum = 0;
	    for ( int idy=0; idy<nrofkernels; idy++)
		ressum += res[idy];

	    setOutputValue( output, 0, idx, z0, ressum/nrofkernels );
	}

	if ( nrofkernels>1 )
	{
	    for ( int idy=0; idy<nrofkernels; idy++ )
		if ( isOutputEnabled(idy+1) )
		   setOutputValue( output, idy+1, idx, z0, res[idy] ); 
	}
    }

    return true;
}


bool Convolve::computeDataWavelet( const DataHolder& output, int z0,
				   int nrsamples ) const
{
    if ( !inputdata_[0] || !wavelet_ ) return false;

    int waveletsz = wavelet_->size();
    int wavfirstidx = -wavelet_->centerSample();
    float* wavarr = wavelet_->samples();
    int inpshift = z0-inputdata_[0]->z0_;
    float* outp = output.series(0)->arr();
    GenericConvolve( waveletsz, wavfirstidx, wavarr, inputdata_[0]->nrsamples_,
		     -inpshift, *inputdata_[0]->series(dataidx_),
		     nrsamples, 0, outp );
    
    return true;
}


bool Convolve::computeData( const DataHolder& output, const BinID& relpos,
	                    int z0, int nrsamples, int threadid ) const
{
    return kerneltype_==mKernelFunctionWavelet
		? computeDataWavelet( output, z0, nrsamples )
		: computeDataKernel( output, z0, nrsamples );
}


bool Convolve::allowParallelComputation() const
{
    return kerneltype_ != mKernelFunctionWavelet;
}


bool Convolve::isSingleTrace() const
{
    return kerneltype_ != mKernelFunctionPrewitt;
}

} // namespace Attrib
