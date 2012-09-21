/*+
________________________________________________________________________

    (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
    Author:        Bruno
    Date:          May 2011
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id$";


#include "elasticpropsel.h"
#include "elasticpropseltransl.h"

#include "ascstream.h"
#include "streamconn.h"
#include "keystrs.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "math.h"
#include "mathexpression.h"
#include "strmprov.h"
#include "rockphysics.h"
#include "unitofmeasure.h"


#define mFileType "Elastic Property Selection"


static const char* sKeyFormulaName 	= "Name of formula";
static const char* sKeyMathExpr 	= "Mathematic Expression";
static const char* sKeySelVars 		= "Selected properties";
static const char* sKeyUnits 		= "Units";
static const char* sKeyType 		= "Type";
static const char* sKeyPropertyName 	= "Property name";

mDefSimpleTranslators(ElasticPropSelection,mFileType,od,Seis);

DefineEnumNames(ElasticFormula,Type,0,"Elastic Property")
{ "Density", "PWave", "SWave", 0 };


ElasticFormula& ElasticFormula::operator =( const ElasticFormula& ef )
{
    if ( this != &ef )
    {
	setName( ef.name() );
	type_ = ef.type_;
	expression_ = ef.expression_;
	variables_ = ef.variables_;
	units_ 	= ef.units_;
    }
    return *this;
}


void ElasticFormula::fillPar( IOPar& par ) const
{
    par.set( sKeyFormulaName, name() );
    par.set( sKeyType, getTypeString( type_ ) );
    par.set( sKeyMathExpr, expression_ );
    par.set( sKeySelVars, variables_ );
    par.set( sKeyUnits, units_ );
}


void ElasticFormula::usePar( const IOPar& par ) 
{
    BufferString nm; par.get( sKeyFormulaName, nm ); setName( nm );
    parseEnumType( par.find( sKeyType ), type_ );
    par.get( sKeyMathExpr, expression_ );
    par.get( sKeySelVars, variables_ );
    par.get( sKeyUnits, units_ );
}


const char* ElasticFormula::parseVariable( int idx, float& val ) const
{
    if ( !variables_.validIdx( idx ) ) 
	return 0;

    val = mUdf( float );
    const char* var = variables_.get( idx );
    getFromString( val, var );

    return var;
}


ElasticFormulaRepository& ElFR()
{
    static ElasticFormulaRepository* elasticrepos = 0;
    if ( !elasticrepos )
    {
	elasticrepos = new ElasticFormulaRepository;
	elasticrepos->addRockPhysicsFormulas();
	elasticrepos->addPreDefinedFormulas();
    }
    return *elasticrepos;
}

void ElasticFormulaRepository::addPreDefinedFormulas()
{
    BufferString ai = "AI";
    BufferString den = "Density";
    BufferString vel = "Velocity";
    BufferString son = "Sonic";
    BufferString shearson = "ShearSonic";

    BufferStringSet vars; 
    vars.erase(); vars.add( ai ); vars.add( vel );
    addFormula( "AI derived", "AI/Velocity", ElasticFormula::Den, vars );  

    vars.erase(); vars.add( ai ); vars.add( den );
    addFormula( "AI derived", "AI/Density", ElasticFormula::PVel, vars );  

    vars.erase(); vars.add( son );
    addFormula( "Sonic derived", "1/Sonic", ElasticFormula::PVel, vars );  

    vars.erase(); vars.add( shearson );
    addFormula("Shear Sonic derived","1/ShearSonic",ElasticFormula::SVel,vars);
}


void ElasticFormulaRepository::addRockPhysicsFormulas() 
{
    const ObjectSet<RockPhysics::Formula> forms;

    const char** props = ElasticFormula::TypeNames();
    for ( int idx=0; props[idx]; idx++ )
    {
	ElasticFormula::Type tp;
	ElasticFormula::parseEnumType( props[idx], tp );
	BufferStringSet fnms;
	ROCKPHYSFORMS().getRelevant( 
			    ElasticPropertyRef::elasticToStdType(tp), fnms );

	for ( int idfor=0; idfor<fnms.size(); idfor ++ ) 
	{
	    BufferString elasnm = fnms.get( idfor );
	    const RockPhysics::Formula* rpf = ROCKPHYSFORMS().get( elasnm );
	    if ( !rpf ) continue;

	    ElasticFormula fm( elasnm, rpf->def_, tp );

	    MathExpressionParser mep( rpf->def_ );
	    MathExpression* me = mep.parse();
	    if ( !me ) continue;

	    int cstidx = 0; int varidx = 0;
	    for ( int idvar=0; idvar<me->nrVariables(); idvar++)
	    {
		if ( me->getType( idvar ) == MathExpression::Constant )
		{
		    if ( rpf->constdefs_.validIdx( cstidx ) )
			fm.variables().add(
			    toString( rpf->constdefs_[cstidx]->defaultval_ ) );
		    cstidx++;
		}
		else
		{
		    if ( rpf->vardefs_.validIdx( varidx ) )
			fm.variables().add( rpf->vardefs_[varidx]->name() );
		    varidx++;
		}
	    }

	    formulas_ += fm;
	}
    }
}


bool ElasticFormulaRepository::write( Repos::Source src ) const
{
    //Not Supported
    return false;
}


void ElasticFormulaRepository::addFormula( const char* nm, const char* expr, 
					    ElasticFormula::Type tp,
					    const BufferStringSet& vars )
{
    ElasticFormula fm( nm, expr, tp );
    fm.variables() = vars;
    formulas_ += fm;
}


void ElasticFormulaRepository::addFormula( const ElasticFormula& fm )
{
    formulas_ += fm;
}


void ElasticFormulaRepository::getByType( ElasticFormula::Type tp,
					    TypeSet<ElasticFormula>& efs ) const
{
    for ( int idx=0; idx<formulas_.size(); idx++ )
    {
	if ( formulas_[idx].type() == tp )
	   efs += formulas_[idx]; 
    }
}


PropertyRef::StdType 
	ElasticPropertyRef::elasticToStdType(ElasticFormula::Type tp ) 
{
    if ( tp == ElasticFormula::PVel || tp == ElasticFormula::SVel )
	return PropertyRef::Vel;
    if ( tp == ElasticFormula::Den )
	return PropertyRef::Den;

    return PropertyRef::Other;
}




ElasticPropSelection::ElasticPropSelection()
{
    remove(0); // get rid of thickness

    const char** props = ElasticFormula::TypeNames();
    for ( int idx=0; props[idx]; idx++ )
    {
	ElasticFormula::Type tp;
	ElasticFormula::parseEnumType( props[idx], tp );
	(*this) += new ElasticPropertyRef( props[idx], 
				ElasticFormula(props[idx],"", tp) );
    }
}


ElasticPropertyRef& ElasticPropSelection::gt( int idx ) const
{
    static ElasticPropertyRef emptyepr("Empty",
	    			ElasticFormula("","",ElasticFormula::Den) );
    mDynamicCastGet(const ElasticPropertyRef*,epr,(*this)[idx]);
    return const_cast<ElasticPropertyRef&> ( epr ? *epr : emptyepr );
}



ElasticPropertyRef& ElasticPropSelection::gt( ElasticFormula::Type tp ) const
{
    static ElasticPropertyRef emptyepr("Empty",
	    			ElasticFormula("","",ElasticFormula::Den) );
    const ElasticPropertyRef* epr = 0;
    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(const ElasticPropertyRef*,curepr,(*this)[idx]);
	if ( curepr && curepr->elasticType() == tp )
	{
	    epr = curepr;
	    break;
	}
    }
    return const_cast<ElasticPropertyRef&> ( epr ? *epr : emptyepr );
}



bool ElasticPropSelection::isValidInput( BufferString* errmsg ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const ElasticPropertyRef& epr = get( idx );
	const char* propnm = epr.name();
	const BufferStringSet& vars = epr.formula().variables();
	if ( vars.isEmpty() ) 
	 {
	    if ( errmsg )
	    {	
		*errmsg = "No variable specified for "; 
		*errmsg += propnm; 
	    }
	    return false; 
	 }

	if ( !epr.formula().expression() )
	    continue;

	if ( vars.isPresent( epr.name() ) )
	{ 
	    if ( errmsg )
	    {	
		*errmsg += propnm; 
		*errmsg += " is dependent on itself"; 
	    }
	    return false; 
	}

	for ( int idpr=0; idpr<size(); idpr++ )
	{
	    if ( idpr == idx )
		continue;

	    const ElasticPropertyRef& elpr = get( idpr );
	    const char* nm = elpr.name();
	    const ElasticFormula& form = elpr.formula();
	    if ( vars.isPresent(nm) && form.variables().isPresent( propnm ) )
	    { 
		if ( errmsg )
		{
		    *errmsg += propnm; *errmsg += " and "; *errmsg += nm;
		    *errmsg += " depend on each other"; 
		}
		return false; 
	    }
	}
    }
    return true;
}


ElasticPropGuess::ElasticPropGuess( const PropertyRefSelection& pps,
				    ElasticPropSelection& sel )
    : elasticprops_(sel)
{
    const char** props = ElasticFormula::TypeNames();
    for ( int idx=0; props[idx]; idx++ )
    {
	ElasticFormula::Type tp;
	ElasticFormula::parseEnumType( props[idx], tp );
	guessQuantity( pps, tp );
    }
}


void ElasticPropGuess::guessQuantity( const PropertyRefSelection& pps, 
					ElasticFormula::Type tp )
{
    for ( int idx=0; idx<pps.size(); idx++ )
	if ( guessQuantity( *pps[idx], tp ) )
	    break;
}


bool ElasticPropGuess::guessQuantity( const PropertyRef& pref, 
					ElasticFormula::Type tp )
{
    ElasticFormula& fm = elasticprops_.get( tp ).formula();
    if ( !fm.variables().isEmpty() )
	return false;

    if ( pref.stdType() == ElasticPropertyRef::elasticToStdType( tp ) )
    { 
	if ( tp == ElasticFormula::SVel )
	{
	    //TODO check on name as well
	    if ( pref.aliases().isPresent( "SVel" ) ) 
		fm.variables().add( pref.name() ); 
	    else
	    {
		TypeSet<ElasticFormula> efs; ElFR().getByType( tp, efs );
		if ( !efs.isEmpty() ) 
		    fm = efs[0];
	    }
	}
	else	    
	    fm.variables().add( pref.name() ); 

	return true;
    }
    return false;
}


void ElasticPropGen::getVals( float& den, float& pvel, float& svel, 
				const float* vals,int sz) const
{
    const ElasticPropertyRef& denref = elasticprops_.get(ElasticFormula::Den);
    const ElasticPropertyRef& pvref = elasticprops_.get(ElasticFormula::PVel);
    const ElasticPropertyRef& svref = elasticprops_.get(ElasticFormula::SVel);

    den  = getVal( denref.formula(), vals, sz );
    pvel = getVal( pvref.formula(), vals, sz );
    svel = getVal( svref.formula(), vals, sz );
}



float ElasticPropGen::getVal(const ElasticFormula& ef,
				const float* vals,int sz) const
{
    const BufferStringSet& selvars = ef.variables();
    if ( selvars.isEmpty() )
	return mUdf( float );

    MathExpression* expr = 0;
    if ( ef.expression() )
    {
	MathExpressionParser mep( ef.expression() ); expr = mep.parse();
    }

    float val = mUdf(float);
    for ( int idx=0; idx<selvars.size(); idx++ )
    {
	const char* var = ef.parseVariable( idx, val );

	if ( refprops_.isPresent( var ) )
	{
	    const int pridx = refprops_.indexOf(var);
	    val = vals[pridx];
	}
	else if ( elasticprops_.isPresent( var ) && strcmp(var,ef.name()) )
	{
	    const int propidx = elasticprops_.indexOf(var);
	    val = getVal( elasticprops_.get( propidx ), vals, sz );
	}

	if ( !expr ) 
	    break;

	if ( ef.variables().size() == ef.units().size() )
	{
	    const char* uoms = ef.units().get( idx ).buf();
	    const UnitOfMeasure* uom = UnitOfMeasure::getGuessed( uoms );
	    val = uom ? uom->getSIValue( val ) : val;
	}
	expr->setVariableValue( idx, val );
    }
    return val = expr ? expr->getValue() : val;
}




ElasticPropSelection* ElasticPropSelection::get( const MultiID& mid ) 
{
    const IOObj* obj = mid.isEmpty() ? 0 : IOM().get( mid );
    return obj ? get( obj ) : 0;
}


ElasticPropSelection* ElasticPropSelection::get( const IOObj* ioobj )
{
    if ( !ioobj ) return 0;
    ElasticPropSelectionTranslator* tr = 
		(ElasticPropSelectionTranslator*)ioobj->getTranslator();

    if ( !tr ) return 0;
    ElasticPropSelection* eps = 0;

    Conn* conn = ioobj->getConn( Conn::Read );
    if ( conn && !conn->bad() )
    {
	eps = new ElasticPropSelection;
	
	if ( !conn->forRead() || !conn->isStream() )  return false;

	ascistream astream( ((StreamConn&)(*conn)).iStream() );
	if ( !astream.isOfFileType(mTranslGroupName(ElasticPropSelection)) )
	    return false;

	while ( !atEndOfSection( astream.next() ) )
	{
	    IOPar iop; iop.getFrom( astream );
	    ElasticFormula::Type tp; 
	    ElasticFormula::parseEnumType( iop.find( sKeyType ), tp );
	    eps->get( tp ).formula().usePar( iop ); 
	    BufferString nm; iop.get( sKeyPropertyName, nm ); 
	    eps->get( tp ).setName(nm);
	}
	if ( !astream.stream().good() )
	    ErrMsg( "Problem reading Elastic property selection from file" );
    }
    else
	ErrMsg( "Cannot open elastic property selection file" );

    delete conn; delete tr;
    return eps;
}


bool ElasticPropSelection::put( const IOObj* ioobj ) const
{
    if ( !ioobj ) return false;
    ElasticPropSelectionTranslator* tr = 
		(ElasticPropSelectionTranslator*)ioobj->getTranslator();
    if ( !tr ) return false;
    bool retval = false;

    Conn* conn = ioobj->getConn( Conn::Write );
    if ( conn && !conn->bad() )
    {
	if ( !conn->forWrite() || !conn->isStream() ) return false;

	ascostream astream( ((StreamConn&)(*conn)).oStream() );
	const BufferString head( 
			mTranslGroupName(ElasticPropSelection), " file" );
	if ( !astream.putHeader( head ) ) return false;

	IOPar iop; 
	for ( int idx=0; idx<size(); idx++ )
	{
	    iop.set( sKeyPropertyName, get(idx).name() ); 
	    get(idx).formula().fillPar( iop ); 
	    iop.putTo( astream ); iop.setEmpty();
	}
	if ( astream.stream().good() )
	    retval = true;
	else
	    ErrMsg( "Cannot write Elastic property selection" );
    }
    else
	ErrMsg( "Cannot open elastic property selection file for write" );

    delete conn; delete tr;
    return retval;
}


