/*+
________________________________________________________________________

    (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
    Author:        Bruno
    Date:          May 2011
________________________________________________________________________

-*/

static const char* rcsID = "$Id: elasticpropsel.cc,v 1.5 2011-08-01 15:41:04 cvsbruno Exp $";

#include "elasticpropsel.h"
#include "math.h"
#include "mathexpression.h"


static const char* sKeyFormulaName 	= "Name of formula";
static const char* sKeyMathExpr 	= "Mathetmatic Expression";
static const char* sKeySelVarIdxs 	= "Indices of selected properties";
static const char* sKeySelCteVars 	= "Values of constant";

static const char* sKeyDen 	= "Density";
static const char* sKeyPVel 	= "P-Wave";
static const char* sKeySVel 	= "S-Wave";


ElasticFormula& ElasticFormula::operator =( const ElasticFormula& ef )
{
    if ( this != &ef )
    {
	setName( ef.name() );
	type_ = ef.type_;
	expression_ = ef.expression_;
    }
    return *this;
}


const char* ElasticFormula::type2Char( ElasticType tp ) 
{
    return tp == ElasticFormula::PVel ? sKeyPVel 
	 : tp == ElasticFormula::SVel ? sKeySVel
	 : sKeyDen; 
}


ElasticFormula::ElasticType ElasticFormula::char2Type( const char* tp ) 
{
    return !strcmp(tp,sKeyPVel) ? ElasticFormula::PVel 
	 : !strcmp(tp,sKeySVel) ? ElasticFormula::SVel 
	 : ElasticFormula::Den;
}


ElasticFormulaRepository* ElasticFormulaRepository::elasticrepos_ = 0;
ElasticFormulaRepository& elasticFormulas()
{
    if ( !ElasticFormulaRepository::elasticrepos_ )
	ElasticFormulaRepository::elasticrepos_ = new ElasticFormulaRepository;
    return *::ElasticFormulaRepository::elasticrepos_;
}


const char* ElasticFormulaPropSel::subjectName() const
{ return ElasticFormula::type2Char( formula_.type() ); }


void ElasticFormulaPropSel::fillPar( IOPar& par ) const
{
    par.set( IOPar::compKey(subjectName(),sKeyFormulaName), formula_.name() );
    par.set( IOPar::compKey(subjectName(),sKeyMathExpr), formula_.expression());
    par.set( IOPar::compKey(subjectName(),sKeySelVarIdxs), selidxs_ );
    par.set( IOPar::compKey(subjectName(),sKeySelCteVars), ctes_ );
}


void ElasticFormulaPropSel::usePar( const IOPar& par ) 
{
    BufferString nm, expr; 
    if ( !par.get( IOPar::compKey(subjectName(),sKeyFormulaName), nm ) 
    	|| !par.get( IOPar::compKey(subjectName(),sKeyMathExpr), expr ) )
	return;
    formula_ = ElasticFormula( nm, expr, formula_.type() );
    par.get( IOPar::compKey(subjectName(),sKeySelVarIdxs), selidxs_ );
    par.get( IOPar::compKey(subjectName(),sKeySelCteVars), ctes_ );
}


void ElasticPropSelection::fillPar( IOPar& iop ) const
{
    denformula_.fillPar( iop ); 
    pvelformula_.fillPar( iop );
    svelformula_.fillPar( iop );
}


void ElasticPropSelection::usePar( const IOPar& iop )
{
    denformula_.usePar( iop ); 
    pvelformula_.usePar( iop );
    svelformula_.usePar( iop );
}


bool ElasticPropSelection::isValidInput() const
{
    return !denformula_.selidxs_.isEmpty() && !pvelformula_.selidxs_.isEmpty(); 
    //TODO support SWave 
}


void ElasticFormulaRepository::addFormula( const char* nm, const char* expr, 
					    ElasticFormula::ElasticType tp )
{
    if ( tp == ElasticFormula::PVel )
	pvelformulas_ += ElasticFormula( nm, expr, tp );
    else if ( tp == ElasticFormula::SVel )
	svelformulas_ += ElasticFormula( nm, expr, tp );
    else
	denformulas_ += ElasticFormula( nm, expr, tp );

}


void ElasticFormulaRepository::addFormula( const ElasticFormula& fm )
{
    if ( fm.hasType( ElasticFormula::PVel ) )
	pvelformulas_ += fm;
    else if ( fm.hasType( ElasticFormula::SVel ) )
	svelformulas_ += fm;
    else
	denformulas_ += fm;
}


void ElasticFormulaRepository::fillPreDefFormulas()
{
    addFormula( "AI", "AI/Velocity", ElasticFormula::Den ); 

    addFormula( "Sonic", "1/Sonic", ElasticFormula::PVel );
    addFormula( "AI", "AI/Density" , ElasticFormula::PVel);
    addFormula( "S-Wave", "sqrt( a*SWave^2 + b )", ElasticFormula::PVel);

    addFormula( "Shear Sonic", "1/ShearSonic" , ElasticFormula::SVel);
    addFormula( "P-Wave", "sqrt( a*PWave^2 + b )" , ElasticFormula::SVel);
}



ElasticPropGuess::ElasticPropGuess( const PropertyRefSelection& pps,
				    ElasticPropSelection& sel )
    : elasticprops_(sel)
{
    guessDen( pps );
    guessPVel( pps ); 
    guessSVel( pps );
}


bool ElasticPropGuess::guessDen( const PropertyRefSelection& pps )
{
    ElasticFormulaPropSel& inpdata = elasticprops_.denformula_;
    if ( !inpdata.selidxs_.isEmpty() )
       return true;

    const int denidx = guessQuantity( pps, PropertyRef::Den );
    if ( denidx >= 0 ) 
	inpdata.selidxs_ += denidx;

    //TODO search best formula by name from the repos when not found

    return !inpdata.selidxs_.isEmpty();
}


bool ElasticPropGuess::guessPVel( const PropertyRefSelection& pps )
{
    ElasticFormulaPropSel& inpdata = elasticprops_.pvelformula_;
    if ( !inpdata.selidxs_.isEmpty() )
       return true;

    const int pvelidx = guessQuantity( pps, PropertyRef::Vel );
    if ( pvelidx >= 0 ) 
	inpdata.selidxs_ += pvelidx;

    //TODO search best formula by name

    return !inpdata.selidxs_.isEmpty();
}


bool ElasticPropGuess::guessSVel( const PropertyRefSelection& pps )
{
    ElasticFormulaPropSel& inpdata = elasticprops_.svelformula_;
    if ( !inpdata.selidxs_.isEmpty() )
       return true;

    const int svelidx = guessQuantity( pps, PropertyRef::Vel );
    if ( svelidx >= 0 ) 
	inpdata.selidxs_ += svelidx;

    //TODO search best formula by name

    return !inpdata.selidxs_.isEmpty();
}


int ElasticPropGuess::guessQuantity( const PropertyRefSelection& pps,
					 const PropertyRef::StdType& stdtype )
{
    for ( int idx=0; idx<pps.size(); idx++ )
    {
	if ( pps[idx]->stdType() == stdtype )
	    return idx;
    }
    return -1;
}





void ElasticPropGen::fill( AILayer& el, const float* vals, int sz )
{
    el.den_ = getDen( vals, sz );
    el.vel_ = getPVel( vals, sz );
}


void ElasticPropGen::fill( ElasticLayer& el, const float* vals, int sz )
{
    el.den_ = getDen( vals, sz );
    el.vel_ = getPVel( vals, sz );
    el.svel_ = getSVel( vals, sz );
}


float ElasticPropGen::getDen( const float* vals, int sz )
{
    return getVal( elasticprops_.denformula_, vals, sz );
}


float ElasticPropGen::getPVel( const float* vals, int sz )
{
    return getVal( elasticprops_.pvelformula_, vals, sz );
}


float ElasticPropGen::getSVel( const float* vals, int sz )  
{
    return getVal( elasticprops_.svelformula_, vals, sz );
}


float ElasticPropGen::getVal( const ElasticFormulaPropSel& efps, 
				const float* vals, int sz )
{
    const TypeSet<int>& selidxs = efps.selidxs_;
    const bool hasexpr = efps.formula_.expression();
    if ( hasexpr ) 
    {
	MathExpressionParser mep( efps.formula_.expression() );
	MathExpression* expr = mep.parse();
	if ( expr ) 
	{
	    for ( int idx=0; idx<selidxs.size(); idx++ )
		expr->setVariableValue( idx, vals[selidxs[idx]] );
	    return expr->getValue();
	}
    }
    return selidxs.size() ? vals[selidxs[0]] : mUdf( float );
}


