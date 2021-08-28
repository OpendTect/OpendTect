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
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "keystrs.h"
#include "math.h"
#include "mathexpression.h"
#include "rockphysics.h"
#include "streamconn.h"
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
{ "Density", "PWave", "SWave", nullptr };


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
	return nullptr;

    val = mUdf( float );
    const char* var = variables_.get( idx );
    getFromString( val, var, mUdf(float) );

    return var;
}


ElasticFormulaRepository& ElFR()
{
    mDefineStaticLocalObject( PtrMan<ElasticFormulaRepository>,
			      elasticrepos, = nullptr );
    if ( !elasticrepos )
    {
	auto* newrepos = new ElasticFormulaRepository;
	if ( elasticrepos.setIfNull(newrepos,true) )
	{
	    newrepos->addRockPhysicsFormulas();
	    newrepos->addPreDefinedFormulas();
	}
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
	    const BufferString elasnm = fnms.get( idfor );
	    const RockPhysics::Formula* rpf =
					ROCKPHYSFORMS().getByName( elasnm );
	    if ( !rpf ) continue;

	    ElasticFormula fm( elasnm, rpf->def_, tp );

	    const Math::ExpressionParser mep( rpf->def_ );
	    Math::Expression* me = mep.parse();
	    if ( !me )
		continue;

	    int cstidx = 0; int varidx = 0;
	    for ( int idvar=0; idvar<me->nrVariables(); idvar++)
	    {
		if ( me->getType(idvar) == Math::Expression::Constant )
		{
		    if ( rpf->constdefs_.validIdx(cstidx) )
		    {
			fm.variables().add(
			    toString( rpf->constdefs_[cstidx]->defaultval_ ) );
			fm.units().add( BufferString::empty() );

		    }
		    cstidx++;
		}
		else
		{
		    if ( rpf->vardefs_.validIdx(varidx) )
		    {
			fm.variables().add( rpf->vardefs_[varidx]->desc_ );
			fm.units().add( rpf->vardefs_[varidx]->unit_ );
		    }
		    varidx++;
		}
	    }

	    fm.units().add( rpf->unit_ );

	    delete me;
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


//------- ElasticPropertyRef ----------

ElasticPropertyRef::ElasticPropertyRef( const Mnemonic& mn, const char* nm,
					const ElasticFormula& f )
    : PropertyRef(mn,nm)
    , formula_(f)
{
    if ( !isElastic() )
	pErrMsg( "Incorrect type when building ElasticPropertyRef" );
}


ElasticPropertyRef* ElasticPropertyRef::clone() const
{
    return new ElasticPropertyRef( *this );
}


PropertyRef::StdType
	ElasticPropertyRef::elasticToStdType( ElasticFormula::Type tp )
{
    if ( tp == ElasticFormula::PVel || tp == ElasticFormula::SVel )
	return Mnemonic::Vel;
    if ( tp == ElasticFormula::Den )
	return Mnemonic::Den;

    return Mnemonic::Other;
}


//---

ElasticPropSelection::ElasticPropSelection( bool withswave )
    : PropertyRefSelection(false)
{
    const char** props = ElasticFormula::TypeNames();
    for ( int idx=0; props[idx]; idx++ )
    {
	ElasticFormula::Type tp;
	ElasticFormula::parseEnumType( props[idx], tp );
	if ( tp == ElasticFormula::SVel && !withswave )
	    continue;

	const Mnemonic* mn = getByType( tp, props[idx] );
	if ( !mn )
	    { pErrMsg("Should not happen"); continue; }

	add( new ElasticPropertyRef( *mn, props[idx],
				     ElasticFormula(props[idx],"",tp) ) );
    }
}


ElasticPropSelection::ElasticPropSelection( const ElasticPropSelection& oth )
    : PropertyRefSelection(false)
{
    *this = oth;
}


ElasticPropSelection::~ElasticPropSelection()
{
    erase();
}


ElasticPropSelection& ElasticPropSelection::operator =(
					const ElasticPropSelection& oth )
{
    deepCopyClone( *this, oth );
    return *this;
}


ElasticPropertyRef* ElasticPropSelection::getByType( ElasticFormula::Type tp )
{
    const ElasticPropertyRef* ret =
	  const_cast<const ElasticPropSelection*>( this )->getByType( tp );
    return const_cast<ElasticPropertyRef*>( ret );
}


const ElasticPropertyRef* ElasticPropSelection::getByType(
						ElasticFormula::Type tp ) const
{
    for ( const auto* pr : *this )
    {
	const auto* epr = sCast(const ElasticPropertyRef*,pr);
	if ( epr->elasticType() == tp )
	    return epr;
    }

    return nullptr;
}


bool ElasticPropSelection::ensureHasType( ElasticFormula::Type tp )
{
    const ElasticPropSelection defpropsel( tp == ElasticFormula::SVel );
    const ElasticPropertyRef* defelastpr = defpropsel.getByType( tp );
    if ( !defelastpr )
	return false;

    const ElasticPropertyRef* elastpr = getByType( tp );
    if ( !elastpr )
	add( defelastpr->clone() );

    return true;
}


bool ElasticPropSelection::isValidInput( uiString* errmsg ) const
{
    for ( const auto* pr : *this )
    {
	const auto& epr = sCast(const ElasticPropertyRef&,*pr);
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

	if ( vars.isPresent(propnm) )
	{
	    if ( errmsg )
		*errmsg = tr("%1 is dependent on itself").arg(propnm);
	    return false;
	}

	for ( const auto* altpr : *this )
	{
	    if ( altpr == pr )
		continue;

	    const auto& elpr = sCast(const ElasticPropertyRef&,*altpr);
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


ElasticPropGuess::ElasticPropGuess( const PropertyRefSelection& prs,
				    ElasticPropSelection& sel )
    : elasticprops_(sel)
{
    const char** props = ElasticFormula::TypeNames();
    for ( int idx=0; props[idx]; idx++ )
    {
	ElasticFormula::Type tp;
	ElasticFormula::parseEnumType( props[idx], tp );
	elasticprops_.ensureHasType( tp );
	guessQuantity( prs, tp );
    }
}


void ElasticPropGuess::guessQuantity( const PropertyRefSelection& prs,
				      ElasticFormula::Type tp )
{
    if ( tp == ElasticFormula::SVel )
    {
	const PropertyRef* svelpr = prs.getByMnemonic( Mnemonic::defSVEL() );
	if ( svelpr )
	    guessQuantity( *svelpr, tp );
	else
	{
	    const PropertyRef* shearpr = prs.getByMnemonic( Mnemonic::defDTS());
	    if ( shearpr )
		guessQuantity( *shearpr, tp );
	    else
	    {
		for ( const auto* pr : prs )
		    if ( guessQuantity(*pr,tp) )
			break;
	    }
	}
    }
    else
	for ( const auto* pr : prs )
	    if ( guessQuantity(*pr,tp) )
		break;
}


bool ElasticPropGuess::guessQuantity( const PropertyRef& pref,
				      ElasticFormula::Type tp )
{
    ElasticPropertyRef* epr = elasticprops_.getByType( tp );
    if ( !epr )
	return false;

    ElasticFormula& fm = epr->formula();
    if ( !fm.variables().isEmpty() )
	return false;

    if ( pref.stdType() == ElasticPropertyRef::elasticToStdType(tp) ||
	 pref.stdType() == Mnemonic::Son )
    {
	if ( tp == ElasticFormula::SVel )
	{
	    if ( pref.isCompatibleWith(Mnemonic::defSVEL()) )
		fm.variables().add( pref.name() );
	    else if ( pref.isCompatibleWith(Mnemonic::defDTS()) )
	    {
		TypeSet<ElasticFormula> efs;
		ElFR().getByType( tp, efs );
		if ( !efs.isEmpty() )
		{
		    const int ownformidx = efs.size()-1;
		    fm = efs[ownformidx];
		    fm.variables().add( pref.name() );
		}
	    }
	    else
	    {
		TypeSet<ElasticFormula> efs;
		ElFR().getByType( tp, efs );
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



ElasticPropGen::ElasticPropGen( const ElasticPropSelection& eps,
				const PropertyRefSelection& prs )
{
    exprs_.setNullAllowed();
    const ElasticPropertyRef* denref = eps.getByType( ElasticFormula::Den );
    const ElasticPropertyRef* pvelref = eps.getByType( ElasticFormula::PVel );
    const ElasticPropertyRef* svelref = eps.getByType( ElasticFormula::SVel );
    denform_ = denref ? init( denref->formula(), prs ) : nullptr;
    pvelform_ = pvelref ? init( pvelref->formula(), prs ) : nullptr;
    svelform_ = svelref ? init( svelref->formula(), prs ) : nullptr;
}


ElasticPropGen::~ElasticPropGen()
{
    deepErase( propidxsset_ );
    deepErase( propuomsset_ );
    deepErase( exprs_ );
}


bool ElasticPropGen::isOK() const
{
    return denform_ && pvelform_ && svelform_;
}


const ElasticFormula* ElasticPropGen::init( const ElasticFormula& ef,
					    const PropertyRefSelection& prs )
{
    const BufferStringSet& selvars = ef.variables();
    if ( selvars.isEmpty() )
	return nullptr;

    Math::Expression* expr = nullptr;
    if ( ef.expression() )
    {
	const Math::ExpressionParser mep( ef.expression() );
	expr = mep.parse();
    }

    exprs_.add( expr );
    auto* propuoms = new ObjectSet<const UnitOfMeasure>;
    propuoms->setNullAllowed();
    propuomsset_.add( propuoms );
    auto* propidxs = new TypeSet<int>;
    propidxsset_.add( propidxs );
    if ( !expr )
    {
	const PropertyRef* pr = prs.getByName( selvars.first()->buf() );
	if ( !pr )
	    return nullptr;

	propidxs->add( prs.indexOf(pr) );
	propuoms->add( pr->unit() );
	return &ef;
    }

    for ( int idx=0; idx<selvars.size(); idx++ )
    {
	const BufferString var( selvars.get(idx) );
	const Math::Expression::VarType typ = expr->getType( idx );
	if ( typ == Math::Expression::Constant )
	{
	    expr->setVariableValue( idx, var.toDouble() );
	    propidxs->add( -1 );
	    propuoms->add( nullptr );
	    continue;
	}
	else if ( typ == Math::Expression::Variable )
	{
	    const PropertyRef* pr = prs.getByName( var );
	    if ( !pr )
		return nullptr;

	    propidxs->add( prs.indexOf(pr) );
	    propuoms->add( pr->unit() );
	}
	else
	    return nullptr;
    }

    return &ef;
}


void ElasticPropGen::getVals( float& den, float& pvel, float& svel,
			      const float* vals, int sz) const
{
    den  = getValue( *denform_, *propidxsset_[0], *propuomsset_[0], exprs_[0],
		     vals, sz );
    pvel = getValue( *pvelform_, *propidxsset_[1], *propuomsset_[1], exprs_[1],
		     vals, sz );
    svel = getValue( *svelform_, *propidxsset_[2], *propuomsset_[2], exprs_[2],
		     vals, sz );
}


float ElasticPropGen::getValue( const ElasticFormula& ef,
				const TypeSet<int>& propidxs,
				const ObjectSet<const UnitOfMeasure>& propuoms,
				const Math::Expression* expr,
				const float* vals, int sz )
{
    if ( !expr )
    {
	const float exprval = vals[propidxs.first()];
	const UnitOfMeasure* pruom = propuoms.first();
	return pruom ? pruom->getSIValue(exprval) : exprval;
    }

    const BufferStringSet& selvars = ef.variables();
    const BufferStringSet& units = ef.units();
    auto* edexpr = const_cast<Math::Expression*>( expr );
    const int nrvars = selvars.size();
    for ( int ivar=0; ivar<nrvars; ivar++ )
    {
	if ( expr->getType(ivar) != Math::Expression::Variable )
	    continue;

	float exprval = vals[propidxs[ivar]];
	const UnitOfMeasure* pruom = propuoms[ivar];
	const UnitOfMeasure* formuom = units.validIdx( ivar )
				 ? UoMR().get( units[ivar]->buf() ) : nullptr;
	if ( formuom != pruom )
	    convValue( exprval, pruom, formuom );
	edexpr->setVariableValue( ivar, exprval );
    }

    const UnitOfMeasure* formoutuom = units.validIdx( nrvars )
				    ? UoMR().get( units.last()->buf() )
				    : nullptr;
    const float retval = float(expr->getValue());
    return formoutuom ? formoutuom->getSIValue( retval ) : retval;
}




ElasticPropSelection* ElasticPropSelection::getByDBKey( const MultiID& mid )
{
    const IOObj* obj = mid.isEmpty() ? nullptr : IOM().get( mid );
    return obj ? getByIOObj( obj ) : nullptr;
}


ElasticPropSelection* ElasticPropSelection::getByIOObj( const IOObj* ioobj )
{
    if ( !ioobj )
	return nullptr;

    PtrMan<ElasticPropSelectionTranslator> translator =
		(ElasticPropSelectionTranslator*)ioobj->createTranslator();

    if ( !translator )
	return nullptr;

    ElasticPropSelection* eps = nullptr;

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( conn && !conn->isBad() )
    {
	if ( !conn->forRead() || !conn->isStream() )
	    return nullptr;

	StreamConn& strmconn = static_cast<StreamConn&>( *conn );
	ascistream astream( strmconn.iStream() );
	if ( !astream.isOfFileType(mTranslGroupName(ElasticPropSelection)) )
	    return nullptr;

	eps = new ElasticPropSelection;
	while ( !atEndOfSection(astream.next()) )
	{
	    IOPar iop; iop.getFrom( astream );
	    ElasticFormula::Type tp;
	    ElasticFormula::parseEnumType( iop.find(sKeyType), tp );
	    ElasticPropertyRef* epr = eps->getByType( tp );
	    if ( !epr )
		continue;

	    epr->formula().usePar( iop );
	    BufferString nm;
	    if ( iop.get(sKeyPropertyName,nm) )
		epr->setName(nm);
	}

	if ( !astream.isOK() )
	{
	    deleteAndZeroPtr( eps );
	    ErrMsg( "Problem reading Elastic property selection from file" );
	}
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

	StreamConn& strmconn = sCast(StreamConn&,*conn);
	ascostream astream( strmconn.oStream() );
	const BufferString head(
			mTranslGroupName(ElasticPropSelection), " file" );
	if ( !astream.putHeader(head) )
	    return false;

	IOPar iop;
	for ( const auto* pr : *this )
	{
	    iop.setEmpty();
	    const auto& epr = sCast(const ElasticPropertyRef&,*pr);
	    iop.set( sKeyPropertyName, epr.name() );
	    epr.formula().fillPar( iop );
	    iop.putTo( astream );
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
    for ( const auto* pr : *this )
    {
	const auto& epr = sCast(const ElasticPropertyRef&,*pr);
	IOPar elasticproprefpar;
	elasticproprefpar.set( sKey::Name(), epr.name() );
	epr.formula().fillPar( elasticproprefpar );
	elasticpar.mergeComp( elasticproprefpar,
			      IOPar::compKey(sKeyElastic,indexOf(pr)) );
    }

    par.mergeComp( elasticpar, sKeyElasticProp );
}


bool ElasticPropSelection::usePar( const IOPar& par )
{
    PtrMan<IOPar> elasticpar = par.subselect( sKeyElasticProp );
    if ( !elasticpar )
	return false;

    int elasticsz = 0;
    elasticpar->get( sKeyElasticsSize, elasticsz );
    if ( !elasticsz )
	return false;

    erase();
    bool errocc = false;
    BufferStringSet faultynms;
    BufferStringSet corrnms;
    for ( int idx=0; idx<elasticsz; idx++ )
    {
	PtrMan<IOPar> elasticproprefpar =
	    elasticpar->subselect( IOPar::compKey(sKeyElastic,idx) );
	if ( !elasticproprefpar )
	    continue;

	BufferString elasticnm;
	elasticproprefpar->get( sKey::Name(), elasticnm );
	const ElasticFormula::Type tp = ElasticFormula::Type( idx );
	ElasticFormula formulae( nullptr, nullptr, tp );
	formulae.usePar( *elasticproprefpar );

	const Mnemonic* mn = getByType( tp, elasticnm );
	if ( !mn || mn->isUdf() )
	{
	    errocc = true;
	    continue;
	}

	if ( !checkForValidSelPropsDesc(formulae,*mn,faultynms,corrnms) )
	{
	    errocc = true;
	    continue;
	}

	if ( !errocc )
	    add( new ElasticPropertyRef( *mn, elasticnm, formulae ) );

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


ElasticPropSelection& ElasticPropSelection::doAdd( const PropertyRef* pr )
{
    if ( !pr || !pr->isElasticForm() || getByName(pr->name(),false) )
    {
	delete pr;
	return *this;
    }

    ObjectSet<const PropertyRef>::doAdd( pr );
    return *this;
}


const Mnemonic* ElasticPropSelection::getByType( ElasticFormula::Type tp,
						 const char* nm )
{
    const Mnemonic::StdType stdtype =
				ElasticPropertyRef::elasticToStdType( tp );
    const PropertyRefSelection prs( stdtype );
    const MnemonicSelection mns( stdtype );
    const PropertyRef* pr = prs.getByName( nm );
    return pr ? mns.getByName( pr->mnName() ) : &MNC().getGuessed( stdtype );
}


bool ElasticPropSelection::checkForValidSelPropsDesc(
		    const ElasticFormula& formula, const Mnemonic& mn,
		    BufferStringSet& faultynms, BufferStringSet& corrnms )
{
    bool noerror = true;
    const BufferStringSet& variables = formula.variables();
    if ( variables.isEmpty() )
	return noerror;

    if ( !ePROPS().ensureHasElasticProps() )
    {
	corrnms.add( "<No Valid Suggestion>" );
	return false;
    }

    const BufferString formexptrstr( formula.expression() );
    PtrMan<Math::Expression> expr;
    if ( !formexptrstr.isEmpty() )
    {
	const Math::ExpressionParser parser( formexptrstr );
	expr = parser.parse();
	if ( !expr )
	{
	    corrnms.add( "<Invalid formula>" );
	    return false;
	}
    }

    PropertyRefSelection prs, stdprs, allprs;
    if ( expr )
    {
	prs = PropertyRefSelection( mn );
	stdprs = PropertyRefSelection( mn.stdType() );
	allprs = PropertyRefSelection( true, nullptr );
    }
    else
	prs = PropertyRefSelection( mn );

    for ( int idx=0; idx<variables.size(); idx++ )
    {
	const Math::Expression::VarType vartp = expr ? expr->getType( idx )
						 : Math::Expression::Variable;
	if ( vartp == Math::Expression::Recursive )
	    continue;

	const BufferString& varnm = variables.get( idx );
	if ( vartp == Math::Expression::Constant )
	{
	    if ( !varnm.isNumber() )
	    {
		faultynms.add( varnm );
		noerror = false;
	    }

	    continue;
	}

	const PropertyRef* pr = prs.getByName( varnm );
	if ( !pr && expr )
	    pr = stdprs.getByName( varnm );
	if ( !pr && expr )
	    pr = allprs.getByName( varnm );
	if ( !pr )
	{
	    faultynms.add( varnm );
	    noerror = false;
	    corrnms.add( "<No Valid Suggestion>" );
	    continue;
	}

	if ( varnm != pr->name() )
	{
	    corrnms.add( pr->name() );
	    const_cast<BufferString&>( varnm ).set( pr->name() );
	}
    }

    return noerror;
}
