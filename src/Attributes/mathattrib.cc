/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/

#include "mathattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "mathformula.h"
#include "mathspecvars.h"
#include "separstr.h"
#include "survinfo.h"

const Math::SpecVarSet& Attrib::Mathematics::getSpecVars()
{
    mDefineStaticLocalObject( Math::SpecVarSet, svs, );

    if ( svs.isEmpty() )
    {
	svs.add( "DZ", "Z Step" );
	svs.add( "Inl", "Inline number" );
	svs.add( "Crl", "Crossline number" );
	svs.add( "ZIndex", "Z Index" );
	svs.add( "XCoord", "X Coordinate" );
	svs.add( "YCoord", "Y Coordinate" );
	svs.add( "Z", "Z" );
    }

    return svs;
}

namespace Attrib
{
mAttrDefCreateInstance(Mathematics)
}; // namespace Attrib


void Attrib::Mathematics::initClass()
{
    mAttrStartInitClassWithUpdate

    desc->addParam( new StringParam(expressionStr()) );

    DoubleParam cst( cstStr() );
    ParamGroup<DoubleParam>* cstset =
			new ParamGroup<DoubleParam>( 0, cstStr(), cst );
    desc->addParam( cstset );

    desc->addParam( new StringParam(recstartvalsStr(), 0, false) );

    desc->addInput( InputSpec("Data",true) );
    desc->addOutputDataType( Seis::UnknowData );

    desc->setLocality( Desc::SingleTrace );
    mAttrEndInitClass
}


void Attrib::Mathematics::updateDesc( Desc& desc )
{
    ValParam* expr = desc.getValParam( expressionStr() );
    if ( !expr ) return;

    Math::Formula formula( true,Attrib::Mathematics::getSpecVars() );
    formula.setText( expr->getStringValue() );
    if ( formula.isBad() ) return;

    bool foundcst = false;
    BufferStringSet inpnms;
    for ( int idx=0; idx<formula.nrInputs(); idx++ )
    {
	if ( formula.isConst(idx) )
	    foundcst = true;
	else if ( !formula.isSpec(idx) )
	    inpnms.addIfNew( formula.variableName(idx) );
    }

    if ( desc.nrInputs() != inpnms.size() )
    {
	while ( desc.nrInputs() )
	    desc.removeInput(0);

	for ( int idx=0; idx<inpnms.size(); idx++ )
	    desc.addInput( InputSpec( inpnms.get( idx ), true ) );
    }

    desc.setParamEnabled( cstStr(), foundcst );
    desc.setParamEnabled( recstartvalsStr(), formula.isRecursive() );
}


Attrib::Mathematics::Mathematics( Desc& dsc )
    : Provider( dsc )
    , desintv_( Interval<float>(0,0) )
    , reqintv_( Interval<int>(0,0) )
    , formula_(0)
{
    if ( !isOK() ) return;

    inputdata_.allowNull(true);

    //Called expression for backward compatibility, it is now a Math::Formula
    ValParam* form = dsc.getValParam( expressionStr() );
    if ( !form ) return;

    formula_ = new Math::Formula( true, Attrib::Mathematics::getSpecVars() );
    formula_->setText( form->getStringValue() );
    if ( formula_->isBad() )
    { errmsg_ = mToUiStringTodo(formula_->errMsg()); return; }

    int curinpidx = 0;
    TypeSet<int> inputsalreadyused;
    mDescGetParamGroup(DoubleParam,cstset,dsc,cstStr())
    for ( int idx=0; idx<cstset->size(); idx++ )
    {
	const ValParam& param = (ValParam&)(*cstset)[idx];
	for ( int iinp=curinpidx; iinp<formula_->nrInputs(); iinp++ )
	{
	    if ( inputsalreadyused.isPresent(iinp) ) continue;
	    if ( formula_->isConst(iinp) )
	    {
		formula_->setInputDef( iinp, toString(param.getDValue()) );
		inputsalreadyused += iinp;
		curinpidx++;
		break;
	    }
	}
    }

    if ( formula_->isRecursive() )
    {
	SeparString recstartstr;
	mGetString( recstartstr, recstartvalsStr() );
	const int nrvals = recstartstr.size();
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    double val = toDouble( recstartstr[idx] );
	    if ( mIsUdf(val) )
		break;
	    formula_->recStartVals()[idx] = val;
	}

	desintv_.start = -1000;	//ensure we get the entire trace beginning
    }
}


bool Attrib::Mathematics::allowParallelComputation() const
{
    return !formula_->isRecursive();
}


bool Attrib::Mathematics::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Attrib::Mathematics::getInputData( const BinID& relpos, int zintv )
{
    int nrinputs = formula_->nrExternalInputs();
    while ( inputdata_.size() < nrinputs )
    {
	inputdata_ += 0;
	inputidxs_ += -1;
    }

    for ( int varidx=0; varidx<nrinputs; varidx++ )
    {
	const DataHolder* data = inputs_[varidx]->getData( relpos, zintv );
	if ( !data ) return false;

	inputdata_.replace( varidx, data );
	inputidxs_[varidx] = getDataIndex( varidx );
    }

    return true;
}


bool Attrib::Mathematics::computeData( const DataHolder& output,
				       const BinID& relpos, int z0,
				       int nrsamples, int threadid ) const
{
    PtrMan< ::Math::Formula > mathobj
		= formula_ ? new ::Math::Formula(*formula_) : nullptr;
    if ( !mathobj ) return false;

    mathobj->startNewSeries();

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	TypeSet<double> inpvals;
	int nrconstsandspecsfound = 0;
	for ( int inpidx=0; inpidx<formula_->nrInputs(); inpidx++ )
	{
	    if ( formula_->isConst(inpidx) )
	    {
		inpvals += formula_->getConstVal( inpidx );
		nrconstsandspecsfound ++;
		continue;
	    }

	    const int specidx = formula_->specIdx( inpidx );
	    switch ( formula_->specIdx( inpidx ) )
	    {
		case 0 :   inpvals += mCast( double, refstep_ ); break;
		case 1 :   inpvals += mCast( double, currentbid_.inl() ); break;
		case 2 :   inpvals += mCast( double, currentbid_.crl() ); break;
		case 3 :   inpvals += mCast( double, (z0+idx) ); break;
		case 4 :   inpvals += getCurrentCoord().x; break;
		case 5 :   inpvals += getCurrentCoord().y; break;
		case 6 :   inpvals += mCast( double,(z0+idx)*refstep_ ); break;
	    }
	    if ( specidx >=0 )
	    {
		nrconstsandspecsfound ++;
		continue;
	    }

	    const TypeSet<int>& reqshifts = formula_->getShifts( inpidx );
	    for ( int ishft=0; ishft<reqshifts.size(); ishft++ )
	    {
		const int inpdataidx = inpidx-nrconstsandspecsfound;
		const Attrib::DataHolder* inpdh = inputdata_[inpdataidx];
		inpvals += inpdh ? mCast( double, getInputValue( *inpdh,
						  inputidxs_[inpdataidx],
						  idx+reqshifts[ishft],z0 ) )
				 : mUdf(double);
	    }
	}

	double result = mathobj->getValue( inpvals.arr() );
	if ( !Math::IsNormalNumber(result) )
	    result = mUdf(double);

	setOutputValue( output, 0, idx, z0, float(result) );
    }

    return true;
}


const Interval<int>* Attrib::Mathematics::reqZSampMargin( int inp, int ) const
{
    //Trick: as call to this function is not multithreaded
    //we use a single address for reqintv_ which will be reset for every input
    const_cast< Interval<int>* >(&reqintv_)->set(0,0);
    int datainpidx = -1;
    bool found = false;
    for ( int idx=0; idx<formula_->nrInputs(); idx++ )
    {
	if ( !formula_->isConst(idx) && !formula_->isSpec(idx) )
	{
	    datainpidx++;
	    if ( datainpidx == inp )
	    {
		found = true;
		const_cast< Interval<int>* >
		    (&reqintv_)->include( formula_->shiftRange( inp ) );
	    }
	 }
    }

    return found ? &reqintv_ : 0;
}


const Interval<float>* Attrib::Mathematics::desZMargin( int inp, int ) const
{
    return &desintv_;
}
