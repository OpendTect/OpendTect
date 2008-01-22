/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: mathattrib.cc,v 1.23 2008-01-22 16:24:39 cvshelene Exp $
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

    desc->addParam( new FloatParam(recstartStr()) );
    
    desc->addInput( InputSpec("Data",true) );
    desc->addOutputDataType( Seis::UnknowData );

    mAttrEndInitClass
}


void Math::getInputTable( const MathExpression* me, TypeSet<int>& inptab, 
			  bool iscst )
{
    const int nrvar = me->getNrVariables();
    TypeSet<int> tmpset;
    for ( int idx=0; idx<nrvar; idx++ )
    {
	char name[8];
	tmpset += idx;
	snprintf( name, 8, "c%d", idx );
	for ( int idy=0; idy<nrvar; idy++ )
	{
	    if ( !strcmp(name,me->getVariableStr(idy)) )
	    {
		inptab += idy;
		break;
	    }
	}
    }
    if ( !iscst )
    {
	tmpset.createDifference( inptab );
	inptab = tmpset;
    }
}


void Math::updateDesc( Desc& desc )
{
    ValParam* expr = desc.getValParam( expressionStr() );
    if ( !expr ) return;

    PtrMan<MathExpression> formula = 
			MathExpression::parse( expr->getStringValue() );
    if ( !formula ) return;

    TypeSet<int> csttab;
    getInputTable( formula, csttab, true );
    int nrcstvars = csttab.size();
    int nrdiffvars = formula->getNrDiffVariables();
    if ( desc.nrInputs() != ( nrdiffvars-nrcstvars ) )
    {
	while ( desc.nrInputs() )
	    desc.removeInput(0);

	for ( int idx=0; idx<nrdiffvars-nrcstvars; idx++ )
	    desc.addInput(InputSpec(formula->getVarPrefixStr(idx),true));
    }

    desc.setParamEnabled( cstStr(), nrcstvars );
    desc.setParamEnabled( recstartStr(), formula->isRecursive() );
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
    fillInVarsSet();
    if ( expression_->isRecursive() )
	mGetFloat( recstart_, recstartStr() );
}


bool Math::allowParallelComputation() const
{
    return !expression_->isRecursive();
}


bool Math::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Math::getInputData( const BinID& relpos, int zintv )
{
    int nrinputs = expression_->getNrDiffVariables() - csts_.size();
    while ( inputdata_.size() < nrinputs )
    {
	inputdata_ += 0;
	inputidxs_ += -1;
    }

    for ( int varidx=0; varidx<nrinputs; varidx++ )
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

    const int nrxvars = varstable_.size();
    const int nrcstvars = cstsinputtable_.size();
    const int nrvar = mathobj->getNrVariables();
    if ( (nrxvars + nrcstvars) != nrvar ) 
	return false;
    
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	for ( int xvaridx=0; xvaridx<nrxvars; xvaridx++ )
	{
	    const int variableidx = varstable_[xvaridx].varidx_;
	    const int inpidx = varstable_[xvaridx].inputidx_;
	    const int shift = varstable_[xvaridx].shift_;
	    const DataHolder* inpdata = inpidx == -1 ? &output 
						    : inputdata_[inpidx];
	    int compidx = inpidx == -1 ? 0 : inputidxs_[inpidx];
	    const float val = inpidx==-1 && shift+idx<0 
			    ? recstart_ 
			    : getInputValue( *inpdata, compidx, idx+shift, z0);
	    mathobj->setVariable( variableidx, val );
	}
	for ( int cstidx=0; cstidx<nrcstvars; cstidx++ )
	    mathobj->setVariable( cstsinputtable_[cstidx], csts_[cstidx] );

	setOutputValue( output, 0, idx, z0, mathobj->getValue() );
    }

    return true;
}


bool Math::getInputAndShift( int varidx, int& inpidx, int& shift) const
{
    BufferString prefix;
    expression_->getPrefixAndShift( expression_->getVariableStr(varidx),
	   			    prefix, shift );
    inpidx = expression_->getPrefixIdx( prefix );
    return (inpidx>=0 || !strcmp(prefix,"THIS")) && !mIsUdf(shift) && shift<=0;
}


void Math::fillInVarsSet()
{
    TypeSet<int> varstable;
    int inpidx, shift;
    getInputTable( expression_, varstable, false );
    for ( int idx=0; idx<varstable.size(); idx++ )
    {
	const int variableidx = varstable[idx];
	bool canusevar = getInputAndShift( variableidx, inpidx, shift );
	if ( canusevar )
	    varstable_ += VAR( variableidx, inpidx, shift );
    }
}


const Interval<int>* Math::reqZSampMargin( int inp, int ) const
{
    for ( int idx=0; idx<varstable_.size(); idx++ )
	if ( varstable_[idx].inputidx_ == inp )
	    return &varstable_[idx].sampgate_;
    
    return 0;
}


}; // namespace Attrib
