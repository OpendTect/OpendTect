/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          February 2006
 RCS:           $Id: fingerprintattrib.cc,v 1.4 2006-05-19 14:34:02 cvshelene Exp $
________________________________________________________________________

-*/

#include "fingerprintattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "genericnumer.h"

#include <math.h>


namespace Attrib
{

void FingerPrint::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    BinIDParam* refpos = new BinIDParam( refposStr() );
    refpos->setRequired( false );
    desc->addParam( refpos );
    
    FloatParam* refposz = new FloatParam( refposzStr() );
    refposz->setRequired( false );
    desc->addParam( refposz );

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
    statstype->setRequired( false );
    desc->addParam( statstype );
    
    StringParam* valpickset = new StringParam( valpicksetStr() );
    valpickset->setRequired( false );
    desc->addParam( valpickset );
    
    IntParam* valreftype = new IntParam( valreftypeStr() );
    valreftype->setDefaultValue(2);
    desc->addParam( valreftype );
    
    StringParam* rgpickset = new StringParam( rgpicksetStr() );
    rgpickset->setRequired( false );
    desc->addParam( rgpickset );
    
    IntParam* rgreftype = new IntParam( rgreftypeStr() );
    rgreftype->setDefaultValue(2);
    desc->addParam( rgreftype );
    
    desc->addInput( InputSpec("Input data",true) );
    desc->addOutputDataType( Seis::UnknowData );

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* FingerPrint::createInstance( Desc& dsc )
{
    FingerPrint* res = new FingerPrint( dsc );
    res->ref();

    if ( !res->isOK() )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


void FingerPrint::updateDesc( Desc& desc )
{
    mDescGetParamGroup(FloatParam,valueset,desc,valStr())

    while ( desc.nrInputs() )
	desc.removeInput(0);

    for ( int idx=0; idx<valueset->size(); idx++ )
    {
	BufferString bfs = "Input Data"; bfs += idx+1;
	desc.addInput( InputSpec(bfs, true) );
    }

    int type = desc.getValParam(valreftypeStr())->getIntValue();
    desc.setParamEnabled( refposStr(), type == 1 );
    desc.setParamEnabled( refposzStr(), type == 1 );
    desc.setParamEnabled( valpicksetStr(), type == 2 );
    desc.setParamEnabled( statstypeStr(), type == 2 );
    
    int rgtype = desc.getValParam(rgreftypeStr())->getIntValue();
    desc.setParamEnabled( rgpicksetStr(), rgtype == 1 );
}


FingerPrint::FingerPrint( Desc& dsc )
    : Provider( dsc )
    , vectorsize_( 0 )
{
    if ( !isOK() ) return;

    inputdata_.allowNull(true);

    mDescGetParamGroup(FloatParam,valueset,desc,valStr())
    for ( int idx=0; idx<valueset->size(); idx++ )
    {
	const ValParam& param = (ValParam&)(*valueset)[idx];
	refvector_ += param.getfValue(0);
    }

    mDescGetParamGroup(FloatGateParam,rangeset,desc,rangeStr())
    for ( int idx=0; idx<rangeset->size(); idx++ )
    {
	const FloatGateParam& param = (FloatGateParam&)(*rangeset)[idx];
	ranges_ += param.getValue();
    }

    mDescGetParamGroup(IntParam,weightset,desc,weightStr())
    for ( int idx=0; idx<weightset->size(); idx++ )
    {
	const ValParam& param = (ValParam&)(*weightset)[idx];
	weights_ += param.getIntValue(0);
	vectorsize_++;
	int nrtimes = weights_[idx]-1;
	while ( nrtimes )
	{
	    refvector_+= refvector_[idx];
	    ranges_ += ranges_[idx];
	    nrtimes--;
	    vectorsize_++;
	}
    }
}


bool FingerPrint::getInputData( const BinID& relpos, int zintv )
{
    while ( inputdata_.size() < vectorsize_ )
	inputdata_ += 0;

    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	const DataHolder* data = inputs[idx]->getData( relpos, zintv );
	if ( !data ) return false;
	inputdata_.replace( idx, data );
	if ( dataidx_.size()< vectorsize_ ) 
	    dataidx_ += getDataIndex( idx );
    }

    int dataindex = inputs.size();
    for ( int idx=0; idx<weights_.size(); idx++ )
    {
	int nrtimes = weights_[idx]-1;
	while ( nrtimes )
	{
	    inputdata_.replace( dataindex, inputdata_[idx] );
	    if ( dataidx_.size()< vectorsize_ )
		dataidx_ += dataidx_[idx];
	    nrtimes--;
	    dataindex++;
	}
    }
    
    return true;
}


TypeSet<float> FingerPrint::scaleVector( TypeSet<float> rawvalues ) const
{
    TypeSet<float> scaledvalues;
    if ( rawvalues.size() != ranges_.size() ) return scaledvalues;

    for ( int idx=0; idx<rawvalues.size(); idx++ )
    {
	float diff = ranges_[idx].stop - ranges_[idx].start;
	float denom = mIsZero( diff , 0.001 ) ? 0.001 : diff;
	scaledvalues += ( rawvalues[idx] - ranges_[idx].start ) / denom;
    }
    return scaledvalues;
}

bool FingerPrint::computeData( const DataHolder& output, const BinID& relpos, 
			      int z0, int nrsamples ) const
{
    if ( !inputdata_.size() || !outputinterest[0] ) return false;

    static TypeSet<float> scaledref = scaleVector( refvector_ );
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int cursample = idx + z0;
	TypeSet<float> localvals;
	for ( int inpidx=0; inpidx<inputdata_.size(); inpidx++ )
	    localvals += inputdata_[inpidx]->series(dataidx_[inpidx])
			 ->value( cursample-inputdata_[inpidx]->z0_ );

	TypeSet<float> scaledlocal = scaleVector( localvals );

	float val = similarity( scaledref, scaledlocal, scaledref.size() );
	output.series(0)->setValue( cursample-output.z0_, val );
    }

    return true;
}

}; //namespace
