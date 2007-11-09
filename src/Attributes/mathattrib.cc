/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: mathattrib.cc,v 1.22 2007-11-09 16:53:52 cvshelene Exp $
________________________________________________________________________

-*/

#include "mathattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "mathexpression.h"

namespace Attrib
{

mAttrDefCreateInstance(Math)
    
void Math::initClass()
{
    mAttrStartInitClassWithUpdate

    desc->addParam( new StringParam(expressionStr()) );

    FloatParam cst( cstStr() );
    ParamGroup<FloatParam>* cstset = 
			new ParamGroup<FloatParam>( 0, cstStr(), cst );
    desc->addParam( cstset );

    desc->addInput( InputSpec("Data",true) );
    desc->addOutputDataType( Seis::UnknowData );

    mAttrEndInitClass
}


void Math::getInputTable( const MathExpression* me, TypeSet<int>& inptab, 
			  bool iscst )
{
    const int nrvar = me->getNrVariables();
    for ( int idx=0; idx<nrvar; idx++ )
    {
	char name[8];
	snprintf( name, 8,iscst ? "c%d" : "x%d", idx );
	for ( int idy=0; idy<nrvar; idy++ )
	{
	    if ( !strcmp(name,me->getVariableStr(idy)) )
	    {
		inptab += idy;
		break;
	    }
	}
    }
}


void Math::updateDesc( Desc& desc )
{
    ValParam* expr = desc.getValParam( expressionStr() );
    if ( !expr ) return;

    PtrMan<MathExpression> formula = 
			MathExpression::parse( expr->getStringValue() );
    if ( !formula ) return;

    TypeSet<int> inptab;
    getInputTable( formula, inptab, false );

    if ( desc.nrInputs() != inptab.size() )
    {
	while ( desc.nrInputs() )
	    desc.removeInput(0);

	for ( int idx=0; idx<inptab.size(); idx++ )
	    desc.addInput(InputSpec(formula->getVariableStr(inptab[idx]),true));
    }

    TypeSet<int> csttab;
    getInputTable( formula, csttab, true );
    desc.setParamEnabled( cstStr(), csttab.size() );
}


Math::Math( Desc& dsc )
    : Provider( dsc )
{
    if ( !isOK() ) return;

    inputdata_.allowNull(true);

    ValParam* expr = dsc.getValParam( expressionStr() );
    if ( !expr ) return;

    expression_ = MathExpression::parse( expr->getStringValue() );

    mDescGetParamGroup(FloatParam,cstset,dsc,cstStr())
    for ( int idx=0; idx<cstset->size(); idx++ )
    {
	const ValParam& param = (ValParam&)(*cstset)[idx];
	csts_ += param.getfValue();
    }
    
    getInputTable( expression_, cstsinputtable_, true );
}


bool Math::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Math::getInputData( const BinID& relpos, int zintv )
{
    if ( varsinputtable_.isEmpty() )
	getInputTable( expression_, varsinputtable_, false );

    while ( inputdata_.size() < varsinputtable_.size() )
    {
	inputdata_ += 0;
	inputidxs_ += -1;
    }

    for ( int varidx=0; varidx<varsinputtable_.size(); varidx++ )
    {
	const DataHolder* data = inputs[varidx]->getData( relpos, zintv );
	if ( !data ) return false;
	
	inputdata_.replace( varidx, data );
	inputidxs_[varidx] = getDataIndex( varidx );
    }

    return true;
}


bool Math::computeData( const DataHolder& output, const BinID& relpos, 
			int z0, int nrsamples, int threadid ) const
{
    PtrMan<MathExpression> mathobj = expression_ ? expression_->clone() : 0;
    if ( !mathobj ) return false;

    const int nrxvars = varsinputtable_.size();
    const int nrcstvars = cstsinputtable_.size();
    const int nrvar = mathobj->getNrVariables();
    if ( (nrxvars + nrcstvars) != nrvar ) 
	return false;
    
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int cursample = z0 + idx;
	for ( int varidx=0; varidx<nrxvars; varidx++ )
	{
	    const float val = getInputValue( *inputdata_[varidx], 
		    			     inputidxs_[varidx], idx, z0 );
	    const int variable = varsinputtable_[varidx];
	    mathobj->setVariable( variable, val );
	}
	for ( int cstidx=0; cstidx<nrcstvars; cstidx++ )
	    mathobj->setVariable( cstsinputtable_[cstidx], csts_[cstidx] );

	setOutputValue( output, 0, idx, z0, mathobj->getValue() );
    }

    return true;
}

}; // namespace Attrib
