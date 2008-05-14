/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: mathattrib.cc,v 1.26 2008-05-14 15:09:26 cvshelene Exp $
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
    desc->addParam( new FloatParam(recstartposStr()) );
    
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

    bool isrec = formula->isRecursive();
    desc.setParamEnabled( recstartStr(), isrec );
    desc.setParamEnabled( recstartposStr(), isrec );
}


Math::Math( Desc& dsc )
    : Provider( dsc )
    , desintv_( Interval<float>(0,0) )
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

    const bool isrec = expression_->isRecursive();
    const int nrxvars = varstable_.size();
    const int nrcstvars = cstsinputtable_.size();
    const int nrvar = mathobj->getNrVariables();
    if ( (nrxvars + nrcstvars) != nrvar ) 
	return false;

    //TODO: handle prev formulas where recstartpos_ did not exist
    const int recstartidx = mNINT( recstartpos_/refstep );

    //in case first samp is undef prevent result=undef
    //on whole trace for recursive formulas
    bool hasudf = isrec ? true : false;
    
    //exceptional case: recstartpos_>z0
    if ( isrec && recstartidx>z0 )
    {
	for ( int idx=z0; idx<recstartidx; idx++ )
	    setOutputValue( output, 0, idx-z0, z0, recstartval_ );
    }
    
    const int loopstartidx = isrec ? recstartidx : z0;
    const int loopstopidx = z0+nrsamples-1;

    //A temp DataHolder is needed for recursive formulas
    const int tmpholdersz = loopstopidx-recstartidx+1;
    DataHolder* tmpholder = isrec ? new DataHolder(recstartidx, tmpholdersz): 0;
    if ( tmpholder ) tmpholder->add();
    
    for ( int idx=loopstartidx; idx<loopstopidx; idx++ )
    {
	const int sampidx = idx - loopstartidx;
	if ( isrec && sampidx == 0 )
	{
	    setOutputValue( *tmpholder, 0, sampidx, recstartidx, recstartval_ );
	    if ( idx == z0 || ( recstartidx>z0 && idx==recstartidx) )
		setOutputValue( output, 0, sampidx+loopstartidx-z0,
				z0, recstartval_ );

	    continue;
	}
	
	for ( int xvaridx=0; xvaridx<nrxvars; xvaridx++ )
	{
	    const int variableidx = varstable_[xvaridx].varidx_;
	    const int inpidx = varstable_[xvaridx].inputidx_;
	    const int shift = varstable_[xvaridx].shift_;
	    const DataHolder* inpdata = inpidx == -1 ? tmpholder 
						    : inputdata_[inpidx];
	    const int refdhidx = inpidx == -1 ? recstartidx : z0;
	    const int inpsampidx = inpidx == -1 ? sampidx 
						: sampidx+loopstartidx-z0;
	    int compidx = inpidx == -1 ? 0 : inputidxs_[inpidx];
	    const float val = inpidx==-1 && shift+idx-loopstartidx<0
		? recstartval_
		: getInputValue(*inpdata, compidx, inpsampidx+shift, refdhidx);
	    mathobj->setVariable( variableidx, val );
	}
	for ( int cstidx=0; cstidx<nrcstvars; cstidx++ )
	    mathobj->setVariable( cstsinputtable_[cstidx], csts_[cstidx] );

	const float result = mathobj->getValue(); 
	if ( tmpholder )
	    tmpholder->series(0)->setValue( sampidx, result );
	
	if ( idx >= z0 )
	    setOutputValue( output, 0, sampidx+loopstartidx-z0, z0, result );
    }

    return true;
}


bool Math::getInputAndShift( int varidx, int& inpidx, int& shift) const
{
    BufferString prefix;
    expression_->getPrefixAndShift( expression_->getVariableStr(varidx),
	   			    prefix, shift );
    inpidx = expression_->getPrefixIdx( prefix, true );
    bool isrec = !strcmp(prefix,"THIS");
    bool inpok = inpidx>=0 || isrec;
    bool shiftok = !mIsUdf(shift) && ( !isrec || shift<=0 );
    return inpok && shiftok;
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


const Interval<float>* Math::desZMargin( int inp, int ) const
{
    return &desintv_;
}
}; // namespace Attrib
