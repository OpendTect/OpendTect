/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: mathattrib.cc,v 1.6 2005-08-12 11:12:17 cvsnanne Exp $
________________________________________________________________________

-*/

#include "mathattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "datainpspec.h"
#include "mathexpression.h"
#include "undefval.h"


namespace Attrib
{

void Math::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    desc->addParam( new StringParam(expressionStr()) );

    desc->addInput( InputSpec("Data",true) );
    desc->addOutputDataType( Seis::UnknowData );
    desc->init();

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* Math::createInstance( Desc& ds )
{
    Math* res = new Math( ds );
    res->ref();

    if ( !res->isOK() )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


static void getInputTable( const MathExpression* me, TypeSet<int>& inputtable )
{
    const int nrvar = me->getNrVariables();
    for ( int idx=0; idx<nrvar; idx++ )
    {
	char name[8];
	snprintf( name, 8, "x%d", idx );
	for ( int idy=0; idy<nrvar; idy++ )
	{
	    if ( !strcmp(name,me->getVariableStr(idy)) )
	    {
		inputtable += idy;
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

    TypeSet<int> inputtable;
    getInputTable( formula, inputtable );

    while ( desc.nrInputs() )
	desc.removeInput(0);

    for ( int idx=0; idx<formula->getNrVariables(); idx++ )
    {
	desc.addInput( 
		InputSpec(formula->getVariableStr(inputtable[idx]),true) );
    }
}


Math::Math( Desc& desc )
    : Provider( desc )
{
    if ( !isOK() ) return;

    inputdata_.allowNull(true);

    ValParam* expr = desc.getValParam( expressionStr() );
    if ( !expr ) return;

    expression_ = MathExpression::parse( expr->getStringValue() );
}


bool Math::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Math::getInputData( const BinID& relpos, int idi )
{
    const int nrvar = expression_->getNrVariables();
    while ( inputdata_.size() < nrvar )
	inputdata_ += 0;

    for ( int idx=0; idx<nrvar; idx++ )
    {
	const DataHolder* data = inputs[idx]->getData( relpos, idi );
	if ( !data ) return false;
	
	inputdata_.replace( idx, data );
    }

    if ( !inputtable_.size() )
	getInputTable( expression_, inputtable_ );

    return true;
}


bool Math::computeData( const DataHolder& output, const BinID& relpos, 
			int t0, int nrsamples ) const
{
    //if ( !inputdata_.size() ) return false;
    if ( !expression_ ) return false;
    const int nrvar = expression_->getNrVariables();
    if ( inputtable_.size()!=nrvar ) return false;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int cursample = t0 + idx;
	for ( int varidx=0; varidx<nrvar; varidx++ )
	{
	    const float val = inputdata_[varidx]->item(0)->
				value( cursample - inputdata_[varidx]->t0_ );
	    const int variable = inputtable_[varidx];
	    expression_->setVariable( variable, val );
	}

	const float res = expression_->getValue();
	output.item(0)->setValue( idx, mIsUdf(res) ? mUdf(float) : res );
    }

    return true;
}


}; // namespace Attrib
