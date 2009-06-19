/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: mathattrib.cc,v 1.33 2009-06-19 13:02:30 cvshelene Exp $";

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

    FloatParam* recstartpos = new FloatParam( recstartposStr() );
    recstartpos->setDefaultValue( 0 );
    recstartpos->setRequired( false );
    desc->addParam( recstartpos );
    
    desc->addInput( InputSpec("Data",true) );
    desc->addOutputDataType( Seis::UnknowData );

    mAttrEndInitClass
}


void Math::updateDesc( Desc& desc )
{
    ValParam* expr = desc.getValParam( expressionStr() );
    if ( !expr ) return;

    MathExpressionParser mep( expr->getStringValue() );
    PtrMan<MathExpression> formula = mep.parse();
    if ( !formula ) return;

    int nrconsts = 0;
    int nrvariables = 0;
    BufferStringSet varnms;
    for ( int idx=0; idx<formula->nrUniqueVarNames(); idx++ )
    {
	MathExpression::VarType vtyp =
	    MathExpressionParser::varTypeOf( formula->uniqueVarName(idx) );
	switch ( vtyp )
	{
	    case MathExpression::Variable :
		nrvariables++;
		varnms.add( formula->uniqueVarName(idx) );
		break;
	    case MathExpression::Constant :
		nrconsts++;
		break;
	}
    }

    if ( desc.nrInputs() != nrvariables )
    {
	while ( desc.nrInputs() )
	    desc.removeInput(0);

	for ( int idx=0; idx<nrvariables; idx++ )
	    desc.addInput( InputSpec( varnms.get( idx ), true ) );
    }

    desc.setParamEnabled( cstStr(), nrconsts );

    bool isrec = formula->isRecursive();
    desc.setParamEnabled( recstartStr(), isrec );
    desc.setParamEnabled( recstartposStr(), isrec );
}


Math::Math( Desc& dsc )
    : Provider( dsc )
    , desintv_( Interval<float>(0,0) )
    , reqintv_( Interval<int>(0,0) )
    , recstartpos_( 0 )
{
    if ( !isOK() ) return;

    inputdata_.allowNull(true);

    ValParam* expr = dsc.getValParam( expressionStr() );
    if ( !expr ) return;

    MathExpressionParser mep( expr->getStringValue() );
    expression_ = mep.parse();
    errmsg += mep.errMsg();
    if ( !mep.errMsg().isEmpty() ) return;

    mDescGetParamGroup(FloatParam,cstset,dsc,cstStr())
    for ( int idx=0; idx<cstset->size(); idx++ )
    {
	const ValParam& param = (ValParam&)(*cstset)[idx];
	csts_ += param.getfValue();
    }
    
    setUpVarsSets();
    if ( expression_->isRecursive() )
    {
	mGetFloat( recstartval_, recstartStr() );
	mGetFloat( recstartpos_, recstartposStr() );
	desintv_.start = -1000;	//ensure we get the entire trace beginning
    }
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
    //TODO will not work with specials
    int nrinputs = expression_->nrUniqueVarNames() - csts_.size();
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
    //TODO adapt for specials
    PtrMan<MathExpression> mathobj = expression_ ? expression_->clone() : 0;
    if ( !mathobj ) return false;

    const bool isrec = expression_->isRecursive();
    const int nrxvars = varstable_.size();
    const int nrcstvars = cststable_.size();
    const int nrvar = mathobj->nrVariables();
    if ( (nrxvars + nrcstvars) != nrvar ) 
	return false;

    const int recstartidx = mNINT( recstartpos_/refstep );

    //in case first sample is undef prevent result=undef
    //on whole trace for recursive formulas
    bool hasudf = isrec ? true : false;
    
    //exceptional case: recstartpos_>z0
    if ( isrec && recstartidx>z0 )
    {
	for ( int idx=z0; idx<recstartidx; idx++ )
	    setOutputValue( output, 0, idx-z0, z0, recstartval_ );
    }
    
    const int loopstartidx = isrec ? recstartidx : z0;
    const int loopstopidx = z0 + nrsamples;

    //A temp DataHolder is needed for recursive formulas
    const int tmpholdersz = loopstopidx - recstartidx;
    DataHolder* tmpholder = isrec ? new DataHolder(recstartidx, tmpholdersz): 0;
    if ( tmpholder ) tmpholder->add();
    
    for ( int idx=loopstartidx; idx<loopstopidx; idx++ )
    {
	const int sampidx = idx - loopstartidx;
	if ( isrec && sampidx == 0 )
	{
	    setOutputValue( *tmpholder, 0, sampidx, recstartidx, recstartval_ );
	    if ( idx == z0 || recstartidx>z0 )
		setOutputValue( output, 0, idx-z0, z0, recstartval_ );

	    continue;
	}
	
	for ( int xvaridx=0; xvaridx<nrxvars; xvaridx++ )
	{
	    const int variableidx = varstable_[xvaridx].varidx_;
	    const int inpidx = varstable_[xvaridx].inputidx_;
	    const int shift = varstable_[xvaridx].shift_;
	    const DataHolder* inpdata = inpidx == -1 ? tmpholder 
						    : inputdata_[inpidx];
	    const int refdhidx = inpidx == -1 ? recstartidx : 0;
	    const int inpsampidx = inpidx == -1 ? sampidx : idx;
	    int compidx = inpidx == -1 ? 0 : inputidxs_[inpidx];
	    float val = inpidx==-1 && shift+idx-loopstartidx<0
		? recstartval_
		: getInputValue(*inpdata, compidx, inpsampidx+shift, refdhidx);
	    
	    //in case first samp is undef prevent result=undef
	    //on whole trace for recursive formulas
	    if ( inpidx == -1 && mIsUdf( val ) && hasudf )
		val = recstartval_;
	    
	    mathobj->setVariableValue( variableidx, val );
	}
	for ( int cstidx=0; cstidx<nrcstvars; cstidx++ )
	    mathobj->setVariableValue( cststable_[cstidx].fexpvaridx_,
		    		       csts_[cstidx] );

	const float result = mathobj->getValue();
	
	if ( hasudf && !mIsUdf( result ) )
	    hasudf = false;
	
	if ( tmpholder )
	    tmpholder->series(0)->setValue( sampidx, result );
	
	if ( idx >= z0 )
	    setOutputValue( output, 0, sampidx+loopstartidx-z0, z0, result );
    }

    return true;
}


void Math::setUpVarsSets()
{
    for ( int idx=0; idx<expression_->nrVariables(); idx++ )
    {
	int nrcsts = 0;
	int nrspecs = 0;
	BufferString fvarexp = expression_->fullVariableExpression( idx );
	MathExpression::VarType vtyp = MathExpressionParser::varTypeOf(fvarexp);
	switch ( vtyp )
	{
	    case MathExpression::Variable :
	    {
		int shift=0;
		const BufferString varnm =
			    MathExpressionParser::varNameOf( fvarexp, &shift );
		int inputidx = expression_->indexOfUnVarName( varnm.buf() )
		    		- nrcsts - nrspecs;
		varstable_ += VAR( idx, inputidx, shift );
		break;
	    }
	    case MathExpression::Constant :
	    {
		int insertatidx=0;
		int constidx = expression_->getConstIdx( idx );
		while ( insertatidx<cststable_.size()
			&& constidx>cststable_[insertatidx].cstidx_ )
		    insertatidx++;
		cststable_.insert( insertatidx, CSTS( idx, constidx ) );
		break;
	    }
	    case MathExpression::Recursive :
	    {
		int shift=0;
		const BufferString varnm =
			    MathExpressionParser::varNameOf( fvarexp, &shift );
		varstable_ += VAR( idx, -1, shift );
	    }
	    default :	break;
		//TODO specials
	}
    }
}


const Interval<int>* Math::reqZSampMargin( int inp, int ) const
{
    //Trick: as call to this function is not multithreaded
    //we use a single address for reqintv_ which will be reset for every input
    const_cast< Interval<int>* >(&reqintv_)->set(0,0);
    bool found = false;
    for ( int idx=0; idx<varstable_.size(); idx++ )
	if ( varstable_[idx].inputidx_ == inp )
	{
	    found = true;
	    const_cast< Interval<int>* >
		(&reqintv_)->include( varstable_[idx].sampgate_ );
	}
    
    return found ? &reqintv_ : 0;
}


const Interval<float>* Math::desZMargin( int inp, int ) const
{
    return &desintv_;
}
}; // namespace Attrib
