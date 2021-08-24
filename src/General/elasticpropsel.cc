/*+
________________________________________________________________________

    (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
    Author:        Bruno
    Date:          May 2011
________________________________________________________________________

-*/



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
#include "rockphysics.h"
#include "unitofmeasure.h"

static const char* sKeyElasticsSize	= "Nr of Elastic Properties";
static const char* sKeyElasticProp	= "Elastic Properties";
static const char* sKeyElastic		= "Elastic";
static const char* sKeyFormulaName	= "Name of formula";
static const char* sKeyMathExpr		= "Mathematic Expression";
static const char* sKeySelVars		= "Selected properties";
static const char* sKeyUnits		= "Units";
static const char* sKeyType		= "Type";
static const char* sKeyPropertyName	= "Property name";

mDefSimpleTranslators(ElasticPropSelection,
		      "Elastic Property Selection",od,Seis);

mDefineEnumUtils(ElasticFormula,Type,"Elastic Property")
{ "Density", "PWave", "SWave", 0 };


ElasticFormula& ElasticFormula::operator =( const ElasticFormula& ef )
{
    if ( this != &ef )
    {
	setName( ef.name() );
	type_ = ef.type_;
	expression_ = ef.expression_;
	variables_ = ef.variables_;
	units_	= ef.units_;
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
    getFromString( val, var, mUdf(float) );

    return var;
}


ElasticFormulaRepository& ElFR()
{
    mDefineStaticLocalObject(	PtrMan<ElasticFormulaRepository>,
                              elasticrepos, (0) );
    if ( !elasticrepos )
    {
	ElasticFormulaRepository* newrepos = new ElasticFormulaRepository;
	newrepos->addRockPhysicsFormulas();
	newrepos->addPreDefinedFormulas();

        elasticrepos.setIfNull( newrepos, true );
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
	    const RockPhysics::Formula* rpf =
					ROCKPHYSFORMS().getByName( elasnm );
	    if ( !rpf ) continue;

	    ElasticFormula fm( elasnm, rpf->def_, tp );

	    Math::ExpressionParser mep( rpf->def_ );
	    Math::Expression* me = mep.parse();
	    if ( !me ) continue;

	    int cstidx = 0; int varidx = 0;
	    for ( int idvar=0; idvar<me->nrVariables(); idvar++)
	    {
		if ( me->getType( idvar ) == Math::Expression::Constant )
		{
		    if ( rpf->constdefs_.validIdx( cstidx ) )
			fm.variables().add(
			    toString( rpf->constdefs_[cstidx]->defaultval_ ) );
		    cstidx++;
		}
		else
		{
		    if ( rpf->vardefs_.validIdx( varidx ) )
			fm.variables().add( rpf->vardefs_[varidx]->desc_ );
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


//---

ElasticPropSelection::ElasticPropSelection( bool withswave )
{
    mkEmpty(); // get rid of thickness

    const char** props = ElasticFormula::TypeNames();
    for ( int idx=0; props[idx]; idx++ )
    {
	ElasticFormula::Type tp;
	ElasticFormula::parseEnumType( props[idx], tp );
	if ( withswave || tp != ElasticFormula::SVel )
	    *this += new ElasticPropertyRef( props[idx],
				    ElasticFormula(props[idx],"", tp) );
    }
}


ElasticPropSelection& ElasticPropSelection::operator =(
					const ElasticPropSelection& oth )
{
    if ( this != &oth )
    {
	mkEmpty();
	for ( int idx=0; idx<oth.size(); idx++ )
	    *this += new ElasticPropertyRef( oth.gt(idx) );
    }
    return *this;
}


ElasticPropSelection::~ElasticPropSelection()
{
    mkEmpty();
}


static ElasticPropertyRef emptyepr( "Empty",
		ElasticFormula("","",ElasticFormula::Den) );


void ElasticPropSelection::mkEmpty()
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const PropertyRef* pr = (*this)[idx];
	mDynamicCastGet(const ElasticPropertyRef*,epr,pr)
	if ( epr && epr != &emptyepr )
	    delete const_cast<ElasticPropertyRef*>( epr );
    }

    erase();
}


ElasticPropertyRef& ElasticPropSelection::gt( int idx ) const
{
    const PropertyRef* pr = validIdx(idx) ? (*this)[idx] : 0;
    mDynamicCastGet(const ElasticPropertyRef*,epr,pr);
    return const_cast<ElasticPropertyRef&> ( epr ? *epr : emptyepr );
}



ElasticPropertyRef& ElasticPropSelection::gt( ElasticFormula::Type tp ) const
{
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



bool ElasticPropSelection::isValidInput( uiString* errmsg ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const ElasticPropertyRef& epr = getByIdx( idx );
	const char* propnm = epr.name();
	const BufferStringSet& vars = epr.formula().variables();
	if ( vars.isEmpty() )
	 {
	    if ( errmsg )
		*errmsg = tr("No variable specified for %1").arg(propnm);
	    return false;
	 }

	if ( !epr.formula().expression() )
	    continue;

	if ( vars.isPresent( epr.name() ) )
	{
	    if ( errmsg )
		*errmsg = tr("%1 is dependent on itself").arg(propnm);
	    return false;
	}

	for ( int idpr=0; idpr<size(); idpr++ )
	{
	    if ( idpr == idx )
		continue;

	    const ElasticPropertyRef& elpr = getByIdx( idpr );
	    const char* nm = elpr.name();
	    const ElasticFormula& form = elpr.formula();
	    if ( vars.isPresent(nm) && form.variables().isPresent( propnm ) )
	    {
		if ( errmsg )
		    *errmsg = tr("%1 and %2 depend on each other").arg(propnm)
			      .arg(nm);
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
    if ( tp == ElasticFormula::SVel )
    {
	int svelidx = pps.find( "Swave velocity" );
	int shearsonidx = pps.find( "Shear Sonic" );
	if ( svelidx>= 0 )
	    guessQuantity( *pps[svelidx], tp );
	else if ( shearsonidx >= 0 )
	    guessQuantity( *pps[shearsonidx], tp );
	else
	{
	    for ( int idx=0; idx<pps.size(); idx++ )
		if ( guessQuantity( *pps[idx], tp ) )
		    break;
	}
    }
    else
	for ( int idx=0; idx<pps.size(); idx++ )
	    if ( guessQuantity( *pps[idx], tp ) )
		break;
}


bool ElasticPropGuess::guessQuantity( const PropertyRef& pref,
					ElasticFormula::Type tp )
{
    ElasticFormula& fm = elasticprops_.getByType( tp ).formula();
    if ( !fm.variables().isEmpty() )
	return false;

    if ( pref.stdType() == ElasticPropertyRef::elasticToStdType( tp ) ||
	 pref.stdType() == PropertyRef::Son )
    {
	if ( tp == ElasticFormula::SVel )
	{
	    if ( pref.aliases().isPresent( "SVel" ) ||
		 pref.name() == "Swave velocity" )
		fm.variables().add( pref.name() );
	    else if ( pref.aliases().isPresent( "DTS" ) ||
		      pref.name() == "Shear Sonic" )
	    {
		TypeSet<ElasticFormula> efs; ElFR().getByType( tp, efs );
		if ( !efs.isEmpty() )
		{
		    const int ownformidx = efs.size()-1;
		    fm = efs[ownformidx];
		    fm.variables().add( pref.name() );
		}
	    }
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
    const ElasticPropertyRef& denref =
				elasticprops_.getByType(ElasticFormula::Den);
    const ElasticPropertyRef& pvref =
				elasticprops_.getByType(ElasticFormula::PVel);
    const ElasticPropertyRef& svref =
				elasticprops_.getByType(ElasticFormula::SVel);

    den  = getVal( denref.formula(), vals, sz );
    pvel = getVal( pvref.formula(), vals, sz );
    svel = getVal( svref.formula(), vals, sz );
}



float ElasticPropGen::getVal( const ElasticFormula& ef,
			      const float* vals, int sz ) const
{
    const BufferStringSet& selvars = ef.variables();
    if ( selvars.isEmpty() )
	return mUdf( float );

    Math::Expression* expr = 0;
    if ( ef.expression() )
    {
	Math::ExpressionParser mep( ef.expression() ); expr = mep.parse();
    }

    float val = mUdf(float);
    for ( int idx=0; idx<selvars.size(); idx++ )
    {
	const char* var = ef.parseVariable( idx, val );
	if ( caseInsensitiveEqual(var,"P-wave velocity" ))
	    var = "Pwave velocity"; // ugly temporary fix needs rework #1748

	if ( refprops_.isPresent(var) )
	{
	    const int pridx = refprops_.indexOf(var);
	    if ( pridx<0 || pridx>=sz )
		val = mUdf(float);
	    else
		val = vals[pridx];
	}
	else if ( elasticprops_.isPresent(var) && ef.name() != var )
	{
	    const int propidx = elasticprops_.indexOf(var);
	    val = getVal( elasticprops_.getByIdx(propidx), vals, sz );
	}

	if ( !expr )
	    break;

	if ( ef.variables().size() == ef.units().size() )
	{
	    const char* uoms = ef.units().get( idx ).buf();
	    const UnitOfMeasure* uom = UnitOfMeasure::getGuessed( uoms );
	    val = uom && !mIsUdf(val) ? uom->getSIValue( val ) : val;
	}
	expr->setVariableValue( idx, val );
    }
    val = expr ? mCast(float,expr->getValue()) : val;
    delete expr;
    return val;
}




ElasticPropSelection* ElasticPropSelection::getByDBKey( const MultiID& mid )
{
    const IOObj* obj = mid.isEmpty() ? 0 : IOM().get( mid );
    return obj ? getByIOObj( obj ) : 0;
}


ElasticPropSelection* ElasticPropSelection::getByIOObj( const IOObj* ioobj )
{
    if ( !ioobj ) return 0;
    PtrMan<ElasticPropSelectionTranslator> translator =
		(ElasticPropSelectionTranslator*)ioobj->createTranslator();

    if ( !translator ) return 0;
    ElasticPropSelection* eps = 0;

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( conn && !conn->isBad() )
    {
	eps = new ElasticPropSelection;

	if ( !conn->forRead() || !conn->isStream() )
	    return 0;
	StreamConn& strmconn = static_cast<StreamConn&>( *conn );
	ascistream astream( strmconn.iStream() );
	if ( !astream.isOfFileType(mTranslGroupName(ElasticPropSelection)) )
	    return 0;

	while ( !atEndOfSection( astream.next() ) )
	{
	    IOPar iop; iop.getFrom( astream );
	    ElasticFormula::Type tp;
	    ElasticFormula::parseEnumType( iop.find( sKeyType ), tp );
	    eps->getByType( tp ).formula().usePar( iop );
	    BufferString nm; iop.get( sKeyPropertyName, nm );
	    eps->getByType( tp ).setName(nm);
	}
	if ( !astream.isOK() )
	    ErrMsg( "Problem reading Elastic property selection from file" );
    }
    else
	ErrMsg( "Cannot open elastic property selection file" );

    return eps;
}


bool ElasticPropSelection::put( const IOObj* ioobj ) const
{
    if ( !ioobj ) return false;
    PtrMan<ElasticPropSelectionTranslator> translator =
		(ElasticPropSelectionTranslator*)ioobj->createTranslator();
    if ( !translator ) return false;
    bool retval = false;

    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( conn && !conn->isBad() )
    {
	if ( !conn->forWrite() || !conn->isStream() )
	    return false;
	StreamConn& strmconn = static_cast<StreamConn&>( *conn );
	ascostream astream( strmconn.oStream() );
	const BufferString head(
			mTranslGroupName(ElasticPropSelection), " file" );
	if ( !astream.putHeader( head ) ) return false;

	IOPar iop;
	for ( int idx=0; idx<size(); idx++ )
	{
	    iop.set( sKeyPropertyName, getByIdx(idx).name() );
	    getByIdx(idx).formula().fillPar( iop );
	    iop.putTo( astream ); iop.setEmpty();
	}
	if ( astream.isOK() )
	    retval = true;
	else
	    ErrMsg( "Cannot write Elastic property selection" );
    }
    else
	ErrMsg( "Cannot open elastic property selection file for write" );

    return retval;
}


void ElasticPropSelection::fillPar( IOPar& par ) const
{
    IOPar elasticpar;
    elasticpar.set( sKeyElasticsSize, size() );
    for ( int idx=0; idx<size(); idx++ )
    {
	IOPar elasticproprefpar;
	elasticproprefpar.set( sKey::Name(), getByIdx(idx).name().buf() );
	getByIdx(idx).formula().fillPar( elasticproprefpar );
	elasticpar.mergeComp( elasticproprefpar,
			      IOPar::compKey(sKeyElastic,idx) );
    }

    par.mergeComp( elasticpar, sKeyElasticProp );
}


bool ElasticPropSelection::usePar( const IOPar& par )
{
    PtrMan<IOPar> elasticpar = par.subselect( sKeyElasticProp );
    if ( !elasticpar ) return false;
    int elasticsz = 0;
    elasticpar->get( sKeyElasticsSize, elasticsz );
    if ( !elasticsz ) return false;

    deepErase( *this );
    bool errocc = false;
    BufferStringSet faultynms;
    BufferStringSet corrnms;
    for ( int idx=0; idx<elasticsz; idx++ )
    {
	PtrMan<IOPar> elasticproprefpar =
	    elasticpar->subselect( IOPar::compKey(sKeyElastic,idx) );
	if ( !elasticproprefpar ) continue;
	BufferString elasticnm;
	elasticproprefpar->get( sKey::Name(), elasticnm );
	ElasticFormula formulae( 0, 0, ElasticFormula::Type(idx) );
	formulae.usePar( *elasticproprefpar );

	if ( !checkForValidSelPropsDesc(formulae,faultynms,corrnms) )
	{
	    errocc = true;
	    continue;
	}
	if ( !errocc )
	    (*this) += new ElasticPropertyRef( elasticnm, formulae );

	if ( errocc )
	{
	    errmsg_ = tr("Input model contains faulty description strings for "
						    "'Selected Properties'.");
	    errmsg_.append( tr("Output might not be correctly displayed"),
									true );
	    errmsg_.append( tr("Following strings are faulty : "), true );
	    errmsg_.append( toUiString(faultynms.getDispString()), true );
	    errmsg_.append(
		tr("You can try replacing them following strings : ") , true );
	    errmsg_.append( toUiString(corrnms.getDispString()), true );
	    return false;
	}
    }

    return true;
}


bool ElasticPropSelection::checkForValidSelPropsDesc(
		    const ElasticFormula& formula, BufferStringSet& faultynms,
						    BufferStringSet& corrnms )
{
    bool noerror = true;
    BufferStringSet variables = formula.variables();
    if ( variables.isEmpty() )
	return noerror;

    const PropertyRefSelection prs(PropertyRefSelection::getAll(false));
    BufferStringSet dispnms;

    for ( int idx=0; idx<prs.size(); idx++ )
	dispnms.add(prs[idx]->name());

    for ( int jidx=0; jidx<variables.size(); jidx++ )
    {
	BufferString varnm;
	varnm = variables.get( jidx );
	if ( dispnms.indexOf(varnm) < 0 )
	{
	    faultynms.add( varnm );
	    noerror = false;
	    bool foundmatch = false;
	    for ( int kidx=0; kidx<dispnms.size(); kidx++ )
	    {
		const BufferString dispnm = dispnms.get( kidx );
		foundmatch = varnm.isEqual( dispnm, CaseInsensitive );
		if ( foundmatch )
		{
		    corrnms.add( dispnm );
		    break;
		}

	    }
	    if ( !foundmatch )
		corrnms.add( "<No Valid Suggestion>" );

	    continue;
	}
    }

    return noerror;
}
