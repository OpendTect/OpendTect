/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "fingerprintattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "genericnumer.h"

#include <math.h>


namespace Attrib
{

static void scaleVector( const TypeSet<float>& rawvalues,
			 const TypeSet< Interval<float> >& ranges,
			 TypeSet<float>& scaledvalues )
{
    if ( rawvalues.size() != ranges.size() ) return;

    for ( int idx=0; idx<rawvalues.size(); idx++ )
    {
	float diff = ranges[idx].stop_ - ranges[idx].start_;
	float denom = mIsZero( diff , 0.001 ) ? 0.001f : diff;
	scaledvalues += ( rawvalues[idx] - ranges[idx].start_ ) / denom;
    }
}


mAttrDefCreateInstance(FingerPrint)

void FingerPrint::initClass()
{
    mAttrStartInitClassWithUpdate

    BinIDParam* refpos = new BinIDParam( refposStr() );
    desc->addParam( refpos );

    FloatParam* refposz = new FloatParam( refposzStr() );
    desc->addParam( refposz );

    StringParam* reflineset = new StringParam( reflinesetStr() );
    desc->addParam( reflineset );

    StringParam* ref2dline = new StringParam( ref2dlineStr() );
    desc->addParam( ref2dline );

    FloatParam value( valStr() );
    ParamGroup<FloatParam>* valueset =
				new ParamGroup<FloatParam>(0,valStr(),value);
    desc->addParam( valueset );

    FloatGateParam range( rangeStr() );
    ParamGroup<FloatGateParam>* rangeset =
			    new ParamGroup<FloatGateParam>(0,rangeStr(),range);
    desc->addParam( rangeset );

    IntParam weight( weightStr() );
    ParamGroup<IntParam>* weightset =
				new ParamGroup<IntParam>(0,weightStr(),weight);
    desc->addParam( weightset );

    IntParam* statstype = new IntParam( statstypeStr() );
    statstype->setDefaultValue( 0 );
    desc->addParam( statstype );

    StringParam* valpickset = new StringParam( valpicksetStr() );
    desc->addParam( valpickset );

    IntParam* valreftype = new IntParam( valreftypeStr() );
    valreftype->setDefaultValue(2);
    desc->addParam( valreftype );

    StringParam* rgpickset = new StringParam( rgpicksetStr() );
    desc->addParam( rgpickset );

    IntParam* rgreftype = new IntParam( rgreftypeStr() );
    rgreftype->setDefaultValue(2);
    desc->addParam( rgreftype );

    desc->addInput( InputSpec("Input data",true) );
    desc->addOutputDataType( Seis::UnknowData );

    desc->setLocality( Desc::SingleTrace );
    desc->setUsesTrcPos( true );
    mAttrEndInitClass
}


void FingerPrint::updateDesc( Desc& desc )
{
    mDescGetParamGroup(FloatParam,valueset,desc,valStr())

    if ( desc.nrInputs() != valueset->size() )
    {
	while ( desc.nrInputs() )
	    desc.removeInput(0);

	for ( int idx=0; idx<valueset->size(); idx++ )
	{
	    BufferString bfs = "Input Data"; bfs += idx+1;
	    desc.addInput( InputSpec(bfs, true) );
	}
    }
    const int type = desc.getValParam(valreftypeStr())->getIntValue();
    desc.setParamEnabled( refposStr(), type == 1 );
    desc.setParamEnabled( refposzStr(), type == 1 );
    desc.setParamEnabled( valpicksetStr(), type == 2 );
    desc.setParamEnabled( statstypeStr(), type == 2 );

    const bool is2d = desc.descSet() ? desc.descSet()->is2D() : false;
    desc.setParamEnabled( reflinesetStr(), type == 1 && is2d );
    desc.setParamEnabled( ref2dlineStr(), type == 1 && is2d );

    int rgtype = desc.getValParam(rgreftypeStr())->getIntValue();
    desc.setParamEnabled( rgpicksetStr(), rgtype == 1 );
}


FingerPrint::FingerPrint( Desc& dsc )
    : Provider( dsc )
{
    if ( !isOK() ) return;

    inputdata_.allowNull(true);

    mDescGetParamGroup(FloatParam,valueset,getDesc(),valStr())
    for ( int idx=0; idx<valueset->size(); idx++ )
    {
	const ValParam& param = (ValParam&)(*valueset)[idx];
	refvector_ += param.getFValue(0);
    }

    mDescGetParamGroup(FloatGateParam,rangeset,getDesc(),rangeStr())
    for ( int idx=0; idx<rangeset->size(); idx++ )
    {
	const FloatGateParam& param = (FloatGateParam&)(*rangeset)[idx];
	ranges_ += param.getValue();
    }

    mDescGetParamGroup(IntParam,weightset,getDesc(),weightStr())
    for ( int idx=0; idx<weightset->size(); idx++ )
    {
	const ValParam& param = (ValParam&)(*weightset)[idx];
	weights_ += param.getIntValue(0);
	int nrtimes = weights_[idx]-1;
	while ( nrtimes )
	{
	    refvector_+= refvector_[idx];
	    ranges_ += ranges_[idx];
	    nrtimes--;
	}
    }

    scaleVector( refvector_, ranges_, scaledref_ );
}


FingerPrint::~FingerPrint()
{}


bool FingerPrint::getInputData( const BinID& relpos, int zintv )
{
    while ( inputdata_.size() < inputs_.size() )
	inputdata_ += 0;

    while ( dataidx_.size() < inputs_.size() )
	dataidx_ += -1;

    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	const DataHolder* data = inputs_[idx]->getData( relpos, zintv );
	if ( !data ) return false;

	inputdata_.replace( idx, data );
	dataidx_[idx] = getDataIndex( idx );
    }

    return true;
}


bool FingerPrint::usesTracePosition() const
{
    const int type = getDesc().getValParam(valreftypeStr())->getIntValue();
    return type == 1;
}


bool FingerPrint::computeData( const DataHolder& output, const BinID& relpos,
			       int z0, int nrsamples, int threadid ) const
{
    if ( inputdata_.isEmpty() || !outputinterest_[0] ||
	 weights_.size()!=inputdata_.size() )
	return false;

    TypeSet<float> scaledlocal;
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	TypeSet<float> localvals;
	for ( int inpidx=0; inpidx<inputdata_.size(); inpidx++ )
	{
	    const float inputval = getInputValue( *inputdata_[inpidx],
						  dataidx_[inpidx], idx, z0 );
	    for ( int weight=0; weight<weights_[inpidx]; weight++ )
		localvals += inputval;
	}

	scaledlocal.erase();
	scaleVector( localvals, ranges_, scaledlocal );

	const int scaledrefsz = scaledref_.size();
	if ( scaledlocal.size() != scaledrefsz )
	    return false;	//Should never hapen, check to prevent crash

	const float val = similarity( scaledref_, scaledlocal, scaledrefsz );
	setOutputValue( output, 0, idx, z0, val );
    }

    return true;
}

} // namespace Attrib
