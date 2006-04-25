/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          February 2006
 RCS:           $Id: fingerprintattrib.cc,v 1.3 2006-04-25 14:47:46 cvshelene Exp $
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

    IntParam* statstype = new IntParam( statstypeStr() );
    statstype->setRequired( false );
    desc->addParam( statstype );
    
    StringParam* pickset = new StringParam( picksetStr() );
    pickset->setRequired( false );
    desc->addParam( pickset );
    
    IntParam* reftype = new IntParam( reftypeStr() );
    reftype->setDefaultValue(0);
    desc->addParam( reftype );
    
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

    int type = desc.getValParam(reftypeStr())->getIntValue();
    desc.setParamEnabled( refposStr(), type == 1 );
    desc.setParamEnabled( refposzStr(), type == 1 );
    desc.setParamEnabled( picksetStr(), type == 2 );
    desc.setParamEnabled( statstypeStr(), type == 2 );
}


FingerPrint::FingerPrint( Desc& dsc )
    : Provider( dsc )
{
    if ( !isOK() ) return;

    inputdata_.allowNull(true);

    mDescGetParamGroup(FloatParam,valueset,desc,valStr())
    for ( int idx=0; idx<valueset->size(); idx++ )
    {
	const ValParam& param = (ValParam&)(*valueset)[idx];
	vector_ += param.getfValue(0);
    }
}


bool FingerPrint::getInputData( const BinID& relpos, int zintv )
{
    while ( inputdata_.size() < inputs.size() )
	inputdata_ += 0;

    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	const DataHolder* data = inputs[idx]->getData( relpos, zintv );
	if ( !data ) return false;
	inputdata_.replace( idx, data );
	dataidx_ += getDataIndex( idx );
    }
    
    return true;
}


bool FingerPrint::computeData( const DataHolder& output, const BinID& relpos, 
			      int z0, int nrsamples ) const
{
    if ( !inputdata_.size() || !outputinterest[0] ) return false;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int cursample = idx + z0;
	TypeSet<float> localvals;
	for ( int inpidx=0; inpidx<inputdata_.size(); inpidx++ )
	    localvals += inputdata_[inpidx]->series(dataidx_[inpidx])
			 ->value( cursample-inputdata_[inpidx]->z0_ );

	float val = similarity( vector_, localvals, vector_.size() );
	output.series(0)->setValue( cursample-output.z0_, val );
    }

    return true;
}

}; //namespace
