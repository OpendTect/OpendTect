/*+
________________________________________________________________________

    (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
    Author:        Bruno
    Date:          May 2011
________________________________________________________________________

-*/

static const char* rcsID = "$Id: elasticpropsel.cc,v 1.4 2011-07-29 14:38:58 cvsbruno Exp $";

#include "elasticpropsel.h"
#include "math.h"
#include "mathexpression.h"


static const char* sKeyFormulaName = "Name of formula";
static const char* sKeyMathExpr = "Mathetmatic Expression";

static const char* sKeySelVarIdxs = "Indices of selected properties";
static const char* sKeySelCteVars = "Values of constant";
static const char* sKeyTypeNr = "Type";


int ElasticFormula::type2Int( Type tp ) 
{
    return  tp == ElasticFormula::Den ? 0 
	  : tp == ElasticFormula::PVel ? 1 
	  : tp == ElasticFormula::SVel ? 2 
	  : -1;
}


ElasticFormula::Type ElasticFormula::int2Type( int tpnr ) 
{
    return tpnr == 1 ? ElasticFormula::PVel 
	 : tpnr == 2 ? ElasticFormula::SVel 
		     : ElasticFormula::Den;
}


ElasticFormulaRepository* ElasticFormulaRepository::elasticrepos_ = 0;
ElasticFormulaRepository& elasticFormulas()
{
    if ( !ElasticFormulaRepository::elasticrepos_ )
	ElasticFormulaRepository::elasticrepos_ = new ElasticFormulaRepository;
    return *::ElasticFormulaRepository::elasticrepos_;
}


void ElasticFormulaPropSel::fillPar( IOPar& par ) const
{
    par.set( sKeyFormulaName, formula_.name() );
    par.set( sKeyMathExpr, formula_.expression() );
    par.set( sKeyTypeNr,  ElasticFormula::type2Int( formula_.type() ) );
    par.set( sKeySelVarIdxs, selidxs_ );
    par.set( sKeySelCteVars, ctes_ );
}


void ElasticFormulaPropSel::usePar( const IOPar& par ) 
{
    BufferString nm, expr; 
    int typenr;
    par.get( sKeyFormulaName, nm );
    par.get( sKeyMathExpr, expr );
    par.get( sKeyTypeNr, typenr );
    formula_ = ElasticFormula( expr, nm, ElasticFormula::int2Type( typenr ) );
    par.get( sKeySelVarIdxs, selidxs_ );
    par.get( sKeySelCteVars, ctes_ );
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



void ElasticFormulaRepository::addFormula( const char* nm, const char* expr, 
					    ElasticFormula::Type tp )
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


