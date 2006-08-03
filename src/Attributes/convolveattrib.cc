/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: convolveattrib.cc,v 1.14 2006-08-03 08:04:34 cvshelene Exp $";

#include "convolveattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "ptrman.h"

#define mShapeCube	0
#define mShapeSphere    1

#define mKernelFunctionLowPass		0
#define mKernelFunctionLaplacian	1
#define mKernelFunctionPrewitt		2

namespace Attrib
{

mAttrDefCreateInstance(Convolve)
    
void Convolve::initClass()
{
    mAttrStartInitClassWithUpdate

    EnumParam* kernel = new EnumParam(kernelStr());
    //Note: Ordering must be the same as numbering!
    kernel->addEnum( kernelTypeStr(mKernelFunctionLowPass) );
    kernel->addEnum( kernelTypeStr(mKernelFunctionLaplacian) );
    kernel->addEnum( kernelTypeStr(mKernelFunctionPrewitt) );
    kernel->setDefaultValue( mKernelFunctionLowPass );
    desc->addParam(kernel);

    EnumParam* shape = new EnumParam( shapeStr() );
    //Note: Ordering must be the same as numbering!
    shape->addEnum( shapeTypeStr(mShapeCube) );
    shape->addEnum( shapeTypeStr(mShapeSphere) );
    shape->setDefaultValue( mShapeSphere );
    desc->addParam(shape);

    IntParam* sizepar = new IntParam( sizeStr() );
    sizepar->setLimits( Interval<int>(0,30) );
    sizepar->setDefaultValue( 3 );
    desc->addParam( sizepar );

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


void Convolve::updateDesc( Desc& desc )
{
    const ValParam* kernel = desc.getValParam( kernelStr() );
    if ( !strcmp( kernel->getStringValue(0), 
		  kernelTypeStr(mKernelFunctionPrewitt) ) )
    {
	desc.setParamEnabled(sizeStr(),false);
	desc.setParamEnabled(shapeStr(),false);
	desc.setNrOutputs( Seis::UnknowData, 4 );
    }
    else
    {
	desc.setParamEnabled(sizeStr(),true);
	desc.setParamEnabled(shapeStr(),true);
    }
}


const char* Convolve::kernelTypeStr(int type)
{
    if ( type==mKernelFunctionLowPass ) return "LowPass";
    if ( type==mKernelFunctionLaplacian ) return "Laplacian";
    return "Prewitt";
}


const char* Convolve::shapeTypeStr(int type)
{
    if ( type==mShapeCube ) return "Cube";
    return "Sphere";
}


const float* Convolve::Kernel::getKernel(  ) const 
{ return kernel; }


const BinID& Convolve::Kernel::getStepout( ) const
{ return stepout; }


int Convolve::Kernel::nrSubKernels(  ) const
{ return nrsubkernels; }


int Convolve::Kernel::getSubKernelSize() const
{
    int inl = 1 + stepout.inl * 2;
    int crl = 1 + stepout.crl * 2;
    int sgw = 1 + sg.width();
    return inl * crl * sgw;
}


const Interval<int>& Convolve::Kernel::getSG( ) const
{ return sg; }


Convolve::Kernel::Kernel( int kernelfunc, int shapetype, int size)
    : kernel( 0 )
    , sum( 0 )
    , nrsubkernels( 1 )
{
    if ( kernelfunc == -1 && shapetype == -1 && size == 0 )
	return;

    if ( kernelfunc==mKernelFunctionLowPass || 
	    kernelfunc==mKernelFunctionLaplacian )
    {
	nrsubkernels = 1;
	const int hsz = size/2;

	stepout = BinID(hsz,hsz);
	sg = Interval<int>(-hsz,hsz);

	kernel = new float[getSubKernelSize()];
	const int value = kernelfunc==mKernelFunctionLowPass ? 1 : -1;

	const int limit2 = hsz*hsz;

	int pos = 0;

	for ( int inl=-stepout.inl; inl<=stepout.inl; inl++ )
	{
	    for ( int crl=-stepout.crl; crl<=stepout.crl; crl++ )
	    {
		for ( int tidx=sg.start; tidx<=sg.stop; tidx++ )
		{
		    float nv =
			( shapetype==mShapeSphere && 
			  limit2<inl*inl+crl*crl+tidx*tidx )
			? 0 : value;
		    kernel[pos++] = nv;
		    sum += nv;
		}
	    }
	}

	if ( kernelfunc==mKernelFunctionLaplacian )
	{
	    kernel[getSubKernelSize()/2] -= sum;
	    sum = 0;
	}
    }
    else if ( kernelfunc==mKernelFunctionPrewitt )
    {
	nrsubkernels = 3;
	stepout = BinID(1,1);
	sg=Interval<int>(-1,1);
	int sz = getSubKernelSize()*nrSubKernels();
	kernel = new float[sz];
	memcpy( kernel, Convolve::prewitt, sz*sizeof(float) );
    }

    int subkernelsize = getSubKernelSize();

    for ( int idy=0; idy<nrSubKernels(); idy++ )
    {
	float subkernelsum = 0;
	for ( int idx=0; idx<subkernelsize; idx++ )
	    subkernelsum += kernel[idy*subkernelsize+idx];

	if ( !mIsZero(subkernelsum,mDefEps) )
	{
	    for ( int idx=0; idx<subkernelsize; idx++ )
	    {
		kernel[idy*subkernelsize+idx] /= subkernelsum;
	    }
	}

	sum += subkernelsum;
    }
}
    


Convolve::Kernel::~Kernel()
{ delete [] kernel; }


Convolve::Convolve( Desc& ds )
    : Provider(ds)
    , shape (-1)
    , size(0)
    , stepout(0,0)
    , kernel(0)
{
    if ( !isOK() ) return;

    inputdata.allowNull( true );

    mGetEnum( kerneltype, kernelStr() );
    if ( kerneltype != mKernelFunctionPrewitt )
    {
	mGetEnum( shape, shapeStr() );
	mGetInt( size, sizeStr() );
    }

    kernel = new Kernel( kerneltype, shape , size );
    stepout = kernel->getStepout();
}


Convolve::~Convolve()
{
    delete kernel;
}


bool Convolve::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Convolve::getInputData( const BinID& relpos, int idx )
{
    stepout = kernel->getStepout();
    int sz = (1+stepout.inl*2) * (1+stepout.crl*2);
    
    while ( inputdata.size()< sz )
	inputdata += 0;

    const BinID bidstep = inputs[0]->getStepoutStep();
    BinID truepos;
    int index = 0;
    for (int inl=-stepout.inl; inl<=stepout.inl; inl++ )
    {
	for (int crl=-stepout.crl; crl<=stepout.crl; crl++ )
	{
	    truepos.inl = inl * bidstep.inl;
	    truepos.crl = crl * bidstep.crl;
	    const DataHolder* data = inputs[0]->getData( relpos+truepos, idx );
	    if ( !data )
		return false;
	    inputdata.replace( index++, data );
	}
    }
    dataidx_ = getDataIndex( 0 );

    return true;
}


const BinID* Convolve::reqStepout( int inp, int out ) const
{ return &stepout; }


const Interval<float>* Convolve::reqZMargin( int inp, int ) const
{
    Interval<float> tg( kernel->getSG().start *refstep, 
	    		kernel->getSG().stop * refstep );
    const_cast<Convolve*>(this)->interval = tg;
    return &interval;
}


bool Convolve::computeData( const DataHolder& output, const BinID& relpos,
	                    int z0, int nrsamples ) const
{
    const int nrofkernels = kernel->nrSubKernels();
    const int subkernelsz = kernel->getSubKernelSize();
    const float* kernelvals = kernel->getKernel();

    const int nrtraces = (1+stepout.inl*2) * (1+stepout.crl*2);

    ArrPtrMan<bool> docalculate = new bool[nrofkernels];
    const bool customcalc = !outputinterest[0];
    for ( int idx=0; idx<nrofkernels; idx++ )
	docalculate[idx] = customcalc ? outputinterest[idx+1] : true;

    const Interval<int> sg = kernel->getSG();
    const int sgwidth = 1 + sg.width();

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int cursample = z0 + idx;
	ArrPtrMan<float> res = new float[nrofkernels];
	for ( int idy=0; idy<nrofkernels; idy++ )
	    res[idy] = 0;

	for ( int idy=0; idy<nrtraces; idy++)
	{
	    if ( !inputdata[idy] )
		continue;
	    
	    const int valoffset = idy * sgwidth;

	    ArrPtrMan<float> vals = new float[sgwidth];
	    for ( int valindex=0; valindex<sgwidth; valindex++ )
		vals[valindex] = inputdata[idy]->series(dataidx_)->
		   value( cursample-inputdata[idy]->z0_ + (sg.start+valindex) );

	    for ( int kidx=0; kidx<nrofkernels; kidx++ )
	    {
		if ( customcalc && !docalculate[kidx] ) continue;

		int kerneloff = kidx*subkernelsz+valoffset;

		for ( int valindex=0; valindex<sgwidth; valindex++ )
		    res[kidx] += vals[valindex]*kernelvals[kerneloff+valindex];
	    }

	}

	const int outidx = z0 - output.z0_ + idx;
	if ( outputinterest[0] )
	{
	    float ressum = 0;
	    for ( int idy=0; idy<nrofkernels; idy++)
		ressum += res[idy];

	    output.series(0)->setValue( outidx, ressum/nrofkernels );
	}

	if ( nrofkernels>1 )
	{
	    for ( int idy=0; idy<nrofkernels; idy++ )
		if ( outputinterest[idy+1] ) 
		    output.series(idy+1)->setValue( outidx, res[idy] );
	}
    }

    return true;
}

} // namespace Attrib
