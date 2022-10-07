/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
static const char* sKeyElastic		= "Elastic";
static const char* sKeyFormulaName	= "Name of formula";
static const char* sKeyMathExpr		= "Mathematic Expression";
static const char* sKeySelVars		= "Selected properties";
static const char* sKeyInpMns		= "Input Mnemonics";
static const char* sKeyUnits		= "Units";
static const char* sKeyType		= "Type";
static const char* sKeyPropertyName	= "Property name";

const char* ElasticPropSelection::sKeyElasticProp()
{ return "Elastic Properties"; }
const char* ElasticPropSelection::sKeyElasticPropSel()
{ return "Elastic Property Selection"; }


// ElasticFormula

mDefineEnumUtils(ElasticFormula,Type,"Elastic Property")
{
    "Density",
    "PWave",
    "SWave",
    "Fracture Density",
    "Fracture Orientation",
    nullptr
};


ElasticFormula::ElasticFormula( const char* nm, const char* exp, Type tp )
    : Math::Formula()
{
    setName( nm );
    setText( exp );
    if ( tp == Undef )
	{ pErrMsg("Wrong type for creating an ElasticFormula"); }
    else
	setOutputMnemonic( &getMnemonic(tp) );
}


ElasticFormula::~ElasticFormula()
{
}


ElasticFormula* ElasticFormula::getFrom( const RockPhysics::Formula& form )
{
    const Mnemonic* mn = form.outputMnemonic();
    if ( !mn )
	return nullptr;

    const Type tp = getType( *mn );
    if ( !mn || getType(*mn) == Undef )
	return nullptr;

    auto* ret = new ElasticFormula( form.name(), "", tp );
    static_cast<Math::Formula&>(*ret) = form;
    for ( int iinp=0; iinp<ret->nrInputs(); iinp++ )
    {
	if ( ret->isConst(iinp) || ret->isSpec(iinp) )
	    continue;

	ret->setInputDef( iinp, "" );
    }

    return ret;
}


void ElasticFormula::setExpression( const char* expr )
{
    setText( expr );
}


const char* ElasticFormula::expression() const
{
    return text();
}


const Mnemonic& ElasticFormula::getMnemonic( Type tp )
{
    if ( tp == Den )
	return Mnemonic::defDEN();
    else if ( tp == PVel )
	return Mnemonic::defPVEL();
    else if ( tp == SVel )
	return Mnemonic::defSVEL();
    else if ( tp == FracRho )
	return Mnemonic::defFracDensity();
    else if ( tp == FracAzi )
	return Mnemonic::defFracOrientation();

    return Mnemonic::undef();
}


ElasticFormula::Type ElasticFormula::getType( const Mnemonic& mn )
{
    if ( &mn == &Mnemonic::defDEN() )
	return Den;
    else if ( &mn == &Mnemonic::defPVEL() )
	return PVel;
    else if ( &mn == &Mnemonic::defSVEL() )
	return SVel;
    else if ( &mn == &Mnemonic::defFracDensity() )
	return FracRho;
    else if ( &mn == &Mnemonic::defFracOrientation() )
	return FracAzi;

    return Undef;
}


Mnemonic::StdType ElasticFormula::getStdType( Type tp )
{
    return getMnemonic( tp ).stdType();
}


ElasticFormula::Type ElasticFormula::type() const
{
    const Mnemonic* mn = outputMnemonic();
    if ( mn == &Mnemonic::defDEN() )
	return Den;
    else if ( mn == &Mnemonic::defPVEL() )
	return PVel;
    else if ( mn == &Mnemonic::defSVEL() )
	return SVel;
    else if ( mn == &Mnemonic::defFracDensity() )
	return FracRho;
    else if ( mn == &Mnemonic::defFracOrientation() )
	return FracAzi;

    pErrMsg( "Invalid mnemonic for ElasticFormula" );
    return Undef;
}


bool ElasticFormula::hasType( Type tp ) const
{
    return type() == tp && tp != Undef;
}


void ElasticFormula::setType( Type tp )
{
    setOutputMnemonic( &getMnemonic(tp) );
}


void ElasticFormula::fillPar( IOPar& par ) const
{
    par.set( sKeyFormulaName, name() );
    par.set( sKeyType, getTypeString( type() ) );
    par.set( sKeyMathExpr, text() );
    BufferStringSet variables, units, mnemonics;
    bool hasunits = false, hasmnemonics = false;
    for ( int iinp=0; iinp<nrInputs(); iinp++ )
    {
	variables.add( inputDef( iinp ) );
	mnemonics.add( new BufferString );
	units.add( new BufferString );
	if ( isConst(iinp) || isSpec(iinp) )
	    continue;

	const Mnemonic* mn = inputMnemonic( iinp );
	if ( mn )
	{
	    mnemonics.last()->set( mn->name() );
	    hasmnemonics = true;
	}

	const UnitOfMeasure* uom = inputFormUnit( iinp );
	if ( uom && !uom->scaler().isEmpty() )
	{
	    units.last()->set( uom->name() );
	    hasunits = true;
	}
    }

    if ( !hasunits )
	units.setEmpty();

    par.set( sKeySelVars, variables );
    par.set( sKeyUnits, units );
    if ( hasmnemonics )
	par.set( sKeyInpMns, mnemonics );
}


void ElasticFormula::usePar( const IOPar& par )
{
    BufferString nm;
    if ( par.get(sKeyFormulaName,nm) )
	setName( nm );

    Type type = Den;
    parseEnumType( par.find( sKeyType ), type );
    setType( type );
    BufferString def;
    par.get( sKeyMathExpr, def );
    setText( def );
    BufferStringSet variables, units, mnemonics;
    par.get( sKeySelVars, variables );
    par.get( sKeyUnits, units );
    par.get( sKeyInpMns, mnemonics );
    for ( int iinp=0; iinp<nrInputs(); iinp++ )
    {
	setInputDef( iinp, variables.get(iinp) );
	if ( isConst(iinp) || isSpec(iinp) )
	    continue;

	const BufferString varnm( variableName(iinp) );
	if ( units.validIdx(iinp) )
	    setInputFormUnit( iinp, UoMR().get( units.get(iinp) ) );

	const Mnemonic* mn = nullptr;
	if ( mnemonics.validIdx(iinp) )
	{
	    mn = MNC().getByName( mnemonics.get(iinp), false );
	    if ( mn )
	    {
		setInputMnemonic( iinp, mn );
		continue;
	    }
	}

	mn = MNC().getByName( varnm, false );
	if ( mn )
	{
	    setInputMnemonic( iinp, mn );
	    continue;
	}

	mn = MNC().getByName( varnm, true );
	if ( mn )
	    setInputMnemonic( iinp, mn );
    }
}


BufferStringSet ElasticFormula::variables()
{
    BufferStringSet ret;
    for ( int iinp=0; iinp<nrInputs(); iinp++ )
	ret.add( inputDef(iinp) );

    return ret;
}


BufferStringSet ElasticFormula::units()
{
    BufferStringSet ret;
    for ( int iinp=0; iinp<nrInputs(); iinp++ )
    {
	ret.add( new BufferString );
	if ( isConst(iinp) || isSpec(iinp) )
	    continue;

	const UnitOfMeasure* uom = inputFormUnit( iinp );
	if ( !uom )
	    continue;

	ret.last()->set( uom->name() );
    }

    return ret;
}

mStartAllowDeprecatedSection
const BufferStringSet ElasticFormula::variables() const
{ return const_cast<ElasticFormula&>(*this).variables(); }
const BufferStringSet ElasticFormula::units() const
{ return const_cast<ElasticFormula&>(*this).units(); }
mStopAllowDeprecatedSection

const char* ElasticFormula::parseVariable( int idx, float& val ) const
{
    return nullptr;
}


// ElasticFormulaRepository

ElasticFormulaRepository& ElFR()
{
    mDefineStaticLocalObject( PtrMan<ElasticFormulaRepository>,
			      elasticrepos, = nullptr );
    if ( !elasticrepos )
    {
	auto* newrepos = new ElasticFormulaRepository;
	if ( elasticrepos.setIfNull(newrepos,true) )
	    newrepos->addRockPhysicsFormulas();
    }

    return *elasticrepos;
}


ElasticFormulaRepository::ElasticFormulaRepository()
{
}


ElasticFormulaRepository::~ElasticFormulaRepository()
{
    deepErase( formulas_ );
}


void ElasticFormulaRepository::addRockPhysicsFormulas()
{
    const char** props = ElasticFormula::TypeNames();
    for ( int idx=0; props[idx]; idx++ )
    {
	ElasticFormula::Type tp;
	ElasticFormula::parseEnumType( props[idx], tp );
	ObjectSet<const Math::Formula> rpforms;
	const Mnemonic& mn = ElasticFormula::getMnemonic( tp );
	ROCKPHYSFORMS().getRelevant( mn, rpforms );
	for ( const auto* rpform : rpforms )
	{
	    auto* eform = ElasticFormula::getFrom(
				sCast(const RockPhysics::Formula&,*rpform) );
	    formulas_.add( eform );
	}
    }
}


bool ElasticFormulaRepository::write( Repos::Source src ) const
{
    //Not Supported
    return false;
}


void ElasticFormulaRepository::getByType( ElasticFormula::Type tp,
				ObjectSet<const Math::Formula>& efs ) const
{
    ObjectSet<const Math::Formula> eforms;
    for ( const auto* fm : formulas_ )
	eforms.add( fm );

    Math::getRelevant( ElasticFormula::getMnemonic(tp), eforms, efs );
}


// ElasticPropertyRef

ElasticPropertyRef::ElasticPropertyRef( const Mnemonic& mn, const char* nm )
    : PropertyRef(mn,nm)
{
    if ( !isElastic() )
	pErrMsg( "Incorrect type when building ElasticPropertyRef" );
}


ElasticPropertyRef::ElasticPropertyRef( const ElasticPropertyRef& oth )
    : PropertyRef(oth)
{
    *this = oth;
}


ElasticPropertyRef::~ElasticPropertyRef()
{
    delete formula_;
}


ElasticPropertyRef& ElasticPropertyRef::operator=
					(const ElasticPropertyRef& oth )
{
    if ( this != &oth )
    {
	PropertyRef::operator= ( oth );
	pr_ = oth.pr_;
	delete formula_;
	formula_ = oth.formula_ ? new ElasticFormula(*oth.formula_) : nullptr;
    }

    return *this;
}


bool ElasticPropertyRef::operator==( const ElasticPropertyRef& oth ) const
{
    if ( &oth == this )
	return true;

    if ( PropertyRef::operator!= ( oth ) ||
	 pr_ != oth.pr_ || (bool)formula_ != (bool)oth.formula_ )
	return false;

    if ( formula_ && oth.formula_ && *formula_ != *oth.formula_ )
	return false;

    return true;
}


bool ElasticPropertyRef::operator!=( const ElasticPropertyRef& oth ) const
{
    return !(*this == oth);
}


ElasticPropertyRef* ElasticPropertyRef::clone() const
{
    return new ElasticPropertyRef( *this );
}


bool ElasticPropertyRef::isOK( const PropertyRefSelection* prs ) const
{
    if ( pr_ )
	return prs ? (bool)prs->get( pr_ ) : true;

    if ( !formula_ )
	return false;

    for ( int iinp=0; iinp<formula_->nrInputs(); iinp++ )
    {
	if ( formula_->isConst(iinp) || formula_->isSpec(iinp) )
	    continue;

	const StringView inpdef( formula_->inputDef(iinp) );
	if ( inpdef.isEmpty() || (prs && !prs->getByName(inpdef,false)) )
	    return false;
    }

    return true;
}


void ElasticPropertyRef::setRef( const PropertyRef* pr )
{
    pr_ = pr;
    deleteAndZeroPtr( formula_ );
}


void ElasticPropertyRef::setFormula( const ElasticFormula& eform )
{
    pr_ = nullptr;
    delete formula_;
    formula_ = new ElasticFormula( eform );
}


bool ElasticPropertyRef::usePar( const IOPar& iop )
{
    BufferString expr;
    iop.get( sKeyMathExpr, expr );
    if ( expr.isEmpty() )
    {
	BufferStringSet propnms;
	iop.get( sKeySelVars, propnms );
	if ( propnms.size() != 1 )
	    return false;

	const PropertyRef* pr =
			PROPS().getByName( propnms.first()->buf(), false );
	if ( !pr )
	    return false;

	setRef( pr );
	return true;
    }

    ElasticFormula eform( name(), expr, elasticType() );
    eform.usePar( iop );
    if ( !eform.isOK() )
	return false;

    if ( eform.name() == name() )
	setFormNameFromRepos( eform );

    setFormula( eform );

    return true;
}


void ElasticPropertyRef::fillPar( IOPar& iop ) const
{
    if ( formula_ )
    {
	formula_->fillPar( iop );
	return;
    }
    else if ( !pr_ )
	return;

    iop.set( sKeyFormulaName, name() );
    iop.set( sKeyType, ElasticFormula::getTypeString( elasticType() ) );
    iop.set( sKeyMathExpr, BufferString::empty() );
    iop.set( sKeySelVars, pr_->name() );
    iop.set( sKeyUnits, BufferString::empty() );
}


ElasticFormula::Type ElasticPropertyRef::elasticType() const
{
    if ( !isOK(nullptr) )
	return ElasticFormula::getType( mn() );

    const Mnemonic* mn = pr_ ? &pr_->mn() : formula_->outputMnemonic();
    if ( mn )
	return ElasticFormula::getType( *mn );

    return ElasticFormula::Den;

}


const Mnemonic& ElasticPropertyRef::elasticToMnemonic(
						ElasticFormula::Type tp )
{
    return ElasticFormula::getMnemonic( tp );
}


PropertyRef::StdType
	ElasticPropertyRef::elasticToStdType( ElasticFormula::Type tp )
{
    return ElasticFormula::getStdType( tp );
}


void ElasticPropertyRef::setFormNameFromRepos( ElasticFormula& eform )
{
    const Mnemonic* outmn = eform.outputMnemonic();
    if ( !outmn )
	return;

    MnemonicSelection inpmns;
    for ( int iinp=0; iinp<eform.nrInputs(); iinp++ )
    {
	if ( eform.isConst(iinp) || eform.isSpec(iinp) )
	    continue;

	const Mnemonic* inpmn = eform.inputMnemonic( iinp );
	if ( inpmn )
	    inpmns.add( inpmn );
    }

    ObjectSet<const Math::Formula> allrpforms, rpforms;
    ROCKPHYSFORMS().getRelevant( *outmn, allrpforms );
    Math::getRelevant( *outmn, allrpforms, rpforms, &inpmns );
    for ( const auto* rpform : rpforms )
    {
	if ( StringView(rpform->text()) == eform.text() )
	{
	    eform.setName( rpform->name() );
	    return;
	}
    }
}


// ElasticPropSelection

mDefSimpleTranslators(ElasticPropSelection,
		      ElasticPropSelection::sKeyElasticPropSel(),od,Seis);

ElasticPropSelection::ElasticPropSelection( RefLayer::Type reqtyp,
					    const PropertyRefSelection& prs )
    : ElasticPropSelection(reqtyp)
{
    setFor( prs );
}


ElasticPropSelection::ElasticPropSelection( RefLayer::Type reqtyp )
    : PropertyRefSelection(false)
{
    const bool withswave = reqtyp > RefLayer::Acoustic;
    const bool withfracrho = reqtyp > RefLayer::Elastic;
    const bool withfracazi = reqtyp > RefLayer::VTI;

    const char** props = ElasticFormula::TypeNames();
    for ( int idx=0; props[idx]; idx++ )
    {
	ElasticFormula::Type tp;
	ElasticFormula::parseEnumType( props[idx], tp );
	if ( tp == ElasticFormula::SVel && !withswave )
	    continue;
	if ( tp == ElasticFormula::FracRho && !withfracrho )
	    continue;
	if ( tp == ElasticFormula::FracAzi && !withfracazi )
	    continue;

	const Mnemonic* mn = getByType( tp, props[idx] );
	if ( !mn )
	    { pErrMsg("Should not happen"); continue; }

	add( new ElasticPropertyRef( *mn, props[idx] ) );
    }
}


ElasticPropSelection::ElasticPropSelection( bool needswave )
    : ElasticPropSelection(needswave ? RefLayer::Elastic : RefLayer::Acoustic)
{
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
    if ( this == &oth )
	return *this;

    //No deep cloning, to avoid invalidating the pointers
    TypeSet<ElasticFormula::Type> reqtypes, alltypes;
    alltypes.add( ElasticFormula::Den ).add( ElasticFormula::PVel )
	    .add( ElasticFormula::SVel ).add( ElasticFormula::FracRho )
	    .add( ElasticFormula::FracAzi );
    for ( const auto& tp : alltypes )
    {
	if ( oth.getByType(tp) )
	    reqtypes += tp;
	else
	    *this -= getByType(tp);
    }

    for ( const auto& tp : reqtypes )
    {
	const ElasticPropertyRef& othepref =  *oth.getByType(tp);
	ElasticPropertyRef* epref = getByType(tp);
	if ( epref )
	    *epref = othepref;
	else
	    add( new ElasticPropertyRef( othepref ) );
    }

    storedid_ = oth.storedid_;

    return *this;
}


bool ElasticPropSelection::operator ==( const ElasticPropSelection& oth ) const
{
    if ( &oth == this )
	return true;

    if ( oth.storedid_ != storedid_ )
	return false;

    const int sz = size();
    if ( oth.size() != sz || !oth.isElasticSel() )
	return false;

    for ( int idx=0; idx<sz; idx++ )
    {
	const PropertyRef& pr = *get( idx );
	const PropertyRef& othpr = *oth.get( idx );
	if ( !pr.isElasticForm() || !othpr.isElasticForm() )
	    return false;

	if ( sCast(const ElasticPropertyRef&,pr) !=
	     sCast(const ElasticPropertyRef&,othpr) )
	    return false;
    }

    return true;
}


bool ElasticPropSelection::operator !=( const ElasticPropSelection& oth ) const
{
    return !(*this == oth);
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
    const RefLayer::Type reqtyp =
		RefLayer::getType( tp == ElasticFormula::SVel, false, false );
    const ElasticPropSelection defpropsel( reqtyp );
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
	const BufferString propnm( pr->name() );
	const auto& epr = sCast(const ElasticPropertyRef&,*pr);
	if ( epr.ref() )
	    continue;

	const Math::Formula* form = epr.formula();
	if ( !form || !form->isOK() )
	 {
	    if ( errmsg )
		*errmsg = tr("No variable specified for %1").arg(propnm);
	    return false;
	 }

	BufferStringSet vars;
	for ( int iinp=0; iinp<form->nrInputs(); iinp++ )
	{
	    if ( !form->isConst(iinp) && !form->isSpec(iinp) )
		vars.add( form->inputDef(iinp) );
	}

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

	    const BufferString nm( altpr->name() );
	    const auto& elpr = sCast(const ElasticPropertyRef&,*altpr);
	    const Math::Formula* altform = elpr.formula();
	    if ( !altform )
		continue;

	    BufferStringSet altvars;
	    for ( int iinp=0; iinp<altform->nrInputs(); iinp++ )
	    {
		if ( !altform->isConst(iinp) && !altform->isSpec(iinp) )
		    altvars.add( altform->inputDef(iinp) );
	    }

	    if ( vars.isPresent(nm) && altvars.isPresent(propnm) )
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


// ElasticPropGuess

ElasticPropGuess::ElasticPropGuess( const PropertyRefSelection& prs,
				    ElasticPropSelection& sel )
{
    for ( const auto* elasticprop : sel )
    {
	const auto& epref = sCast(const ElasticPropertyRef&,*elasticprop);
	if ( epref.isOK(&prs) )
	    continue;

	auto& eprefed = const_cast<ElasticPropertyRef&>( epref );
	if ( !guessQuantity(prs,eprefed) )
	    isok_ = false;
    }
}


ElasticPropGuess::~ElasticPropGuess()
{
}


bool ElasticPropGuess::guessQuantity( const PropertyRefSelection& prs,
				      ElasticPropertyRef& epref )
{
    const ElasticFormula::Type tp = epref.elasticType();
    const Mnemonic& formmn = ElasticFormula::getMnemonic( tp );

    for ( const auto* pr : prs )
    {
	const Mnemonic& mn = pr->mn();
	if ( &mn != &formmn )
	    continue;

	epref.setRef( pr );
	return epref.isOK( &prs );
    }

    ObjectSet<const Math::Formula> efs;
    ElFR().getByType( tp, efs );
    ManagedObjectSet<MnemonicSelection> mnsels;

    const PropertyRef* denpr = prs.getByMnemonic( Mnemonic::defDEN() );
    const PropertyRef* pvelpr = prs.getByMnemonic( Mnemonic::defPVEL() );
    const PropertyRef* sonicpr = prs.getByMnemonic( Mnemonic::defDT());
    const PropertyRef* shearpr = prs.getByMnemonic( Mnemonic::defDTS());
    const PropertyRef* phipr = prs.getByMnemonic( Mnemonic::defPHI() );
    const PropertyRef* aipr = prs.getByMnemonic( Mnemonic::defAI() );
    const PropertyRef* sipr = prs.getByMnemonic( Mnemonic::defSI() );
    /* The order in which the selections are added is a pick up formula
       preference */
    if ( tp == ElasticFormula::Den )
    {
	mnsels.add( selection(aipr,pvelpr) );
	mnsels.add( selection(pvelpr) );
	mnsels.add( selection(sonicpr) );
	mnsels.add( selection(phipr) );
    }
    else if ( tp == ElasticFormula::PVel )
    {
	mnsels.add( selection(sonicpr) );
	mnsels.add( selection(aipr,denpr) );
	mnsels.add( selection(denpr) );
    }
    else if ( tp == ElasticFormula::SVel )
    {
	mnsels.add( selection(shearpr) );
	mnsels.add( selection(sipr,denpr) );
	mnsels.add( selection(pvelpr) );
	mnsels.add( selection(sonicpr) );
    }
    else if ( tp == ElasticFormula::FracRho )
    {
    }
    else if ( tp == ElasticFormula::FracAzi )
    {
    }

    const Math::Formula* form = nullptr;
    for ( const auto* mns : mnsels )
    {
	form = Math::getRelevant( formmn, efs, mns );
	if ( form )
	    break;
    }

    if ( !form )
	return false;

    ElasticFormula eform( *(sCast(const ElasticFormula*,form)) );
    for ( int iinp=0; iinp<eform.nrInputs(); iinp++ )
    {
	if ( eform.isConst(iinp) || eform.isSpec(iinp) )
	    continue;

	const Mnemonic* mn = eform.inputMnemonic( iinp );
	if ( !mn )
	    return false;

	const PropertyRef* pr = prs.getByMnemonic( *mn );
	if ( !pr )
	    return false;

	eform.setInputDef( iinp, pr->name() );
    }

    epref.setFormula( eform );
    return epref.isOK( &prs );
}


MnemonicSelection* ElasticPropGuess::selection( const PropertyRef* pr1,
						const PropertyRef* pr2,
						const PropertyRef* pr3,
						const PropertyRef* pr4,
						const PropertyRef* pr5 )
{
    if ( !pr1 )
	return nullptr;

    auto* ret = new MnemonicSelection;
    ret->add( &pr1->mn() );
    if ( pr2 )
	ret->add( &pr2->mn() );
    if ( pr3 )
	ret->add( &pr3->mn() );
    if ( pr4 )
	ret->add( &pr4->mn() );
    if ( pr5 )
	ret->add( &pr5->mn() );

    return ret;
}


// ElasticPropGen::CalcData

ElasticPropGen::CalcData::CalcData( const ElasticPropertyRef& epr )
    : epr_(epr)
{
}

ElasticPropGen::CalcData::~CalcData()
{
}


// ElasticPropGen

ElasticPropGen::ElasticPropGen( const ElasticPropSelection& eps,
				const PropertyRefSelection& prs )
{
    propcalcs_.setNullAllowed();
    TypeSet<ElasticFormula::Type> reqtypes;
    reqtypes += ElasticFormula::Den;
    reqtypes += ElasticFormula::PVel;
    reqtypes += ElasticFormula::SVel; //TODO: should be optional
//    reqtypes += ElasticFormula::FracRho;
//    reqtypes += ElasticFormula::FracAzi;
    for ( const auto& eltyp : reqtypes )
    {
	if ( !init(prs,eps.getByType(eltyp)) )
	    propcalcs_.add( nullptr );
    }
}


ElasticPropGen::~ElasticPropGen()
{
    deepErase( propcalcs_ );
}


bool ElasticPropGen::isOK() const
{
    if ( propcalcs_.size() < 2 )
	return false;

    for ( int idx=0; idx<2; idx++ )
    { // The first two, den and Vp, should not be empty
	if ( !propcalcs_.get(idx) )
	    return false;
    }

    return true;
}


bool ElasticPropGen::init( const PropertyRefSelection& prs,
			   const ElasticPropertyRef* epref )
{
    if ( !epref || !epref->isOK(&prs) )
	return false;

    const PropertyRef* epr = epref->ref();
    if ( epr )
    {
	if ( !prs.isPresent(epr) )
	    return false;

	auto* cd = new CalcData( *epref );
	cd->propidxs_.add( prs.indexOf(epr) );
	cd->pruom_ = epr->unit();
	propcalcs_.add( cd );
	return true;
    }

    const ElasticFormula& form = *epref->formula();
    if ( !form.isOK() )
	return false;

    ElasticFormula formed( form );
    auto* cd = new CalcData( *epref );
    TypeSet<double>& formvals = cd->formvals_;
    for ( int iinp=0; iinp<form.nrInputs(); iinp++ )
    {
	formvals += form.getConstVal( iinp );
	if ( form.isConst(iinp) || form.isSpec(iinp) )
	{
	    cd->propidxs_.add( -1 );
	    continue;
	}

	const PropertyRef* pr = prs.getByName( form.inputDef(iinp) );
	if ( !pr )
	{
	    delete cd;
	    return false;
	}

	cd->propidxs_.add( prs.indexOf(pr) );
	formed.setInputValUnit( iinp, pr->unit() );
    }

    const UnitOfMeasure* outuom =
		UoMR().getInternalFor( form.outputMnemonic()->stdType() );
    formed.setOutputValUnit( outuom );

    const_cast<ElasticFormula&>( form ) = formed;

    propcalcs_.add( cd );
    return true;
}


void ElasticPropGen::getVals( const float* propvals, int propsz,
			      float* elvals, int elsz ) const
{
    for ( int idx=0; idx<elsz; idx++ )
    {
	if ( propcalcs_.validIdx(idx) && propcalcs_.get(idx) )
	    elvals[idx] = getValue( *propcalcs_.get(idx), propvals, propsz );
    }
}


float ElasticPropGen::getValue( const CalcData& cd, const float* vals, int sz )
{
    const ElasticPropertyRef& epr = cd.epr_;
    if ( epr.ref() )
    {
	const int validx = cd.propidxs_.first();
#ifdef __debug__
	if ( validx >= sz )
	    pFreeFnErrMsg("Invalid read index");
#endif
	const float exprval = vals[validx];
	const UnitOfMeasure* pruom = cd.pruom_;
	return pruom ? pruom->getSIValue(exprval) : exprval;
    }

    const ElasticFormula& form = *epr.formula();
    const TypeSet<int>& propidxs = cd.propidxs_;
    TypeSet<double>& formvals = cd.formvals_;
    for ( int iinp=0; iinp<form.nrInputs(); iinp++ )
    {
	if ( form.isConst(iinp) || form.isSpec(iinp) )
	    continue;

	const int validx = propidxs[iinp];
#ifdef __debug__
	if ( validx >= sz )
	    pFreeFnErrMsg("Invalid read index");
#endif
	formvals[iinp] = vals[validx];
    }

    return float(form.getValue( formvals.arr() ));
}



ElasticPropSelection* ElasticPropSelection::getByDBKey( const MultiID& mid )
{
    const IOObj* obj = mid.isUdf() ? nullptr : IOM().get( mid );
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

	    epr->usePar( iop );
	    BufferString nm;
	    if ( iop.get(sKeyPropertyName,nm) )
		epr->setName( nm );
	}

	if ( !eps->isOK(nullptr) )
	{
	    deleteAndZeroPtr( eps );
	    ErrMsg( "Problem reading Elastic property selection from file" );
	}
    }
    else
	ErrMsg( "Cannot open elastic property selection file" );

    eps->storedid_ = ioobj->key();

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
	    epr.fillPar( iop );
	    iop.putTo( astream );
	}
	if ( astream.isOK() )
	    retval = true;
	else
	    ErrMsg( "Cannot write Elastic property selection" );
    }
    else
	ErrMsg( "Cannot open elastic property selection file for write" );

    mSelf().storedid_ = ioobj->key();

    return retval;
}


void ElasticPropSelection::fillPar( IOPar& par ) const
{
    if ( !storedid_.isUdf() )
	par.set( sKeyElasticPropSel(), storedid_ );

    IOPar elasticpar;
    elasticpar.set( sKeyElasticsSize, size() );
    for ( const auto* pr : *this )
    {
	const auto& epr = sCast(const ElasticPropertyRef&,*pr);
	IOPar elasticproprefpar;
	elasticproprefpar.set( sKey::Name(), epr.name() );
	epr.fillPar( elasticproprefpar );
	elasticpar.mergeComp( elasticproprefpar,
			      IOPar::compKey(sKeyElastic,indexOf(pr)) );
    }

    par.mergeComp( elasticpar, sKeyElasticProp() );
}


bool ElasticPropSelection::usePar( const IOPar& par )
{
    MultiID storedid;
    if ( par.get(sKeyElasticPropSel(),storedid) && !storedid.isUdf() &&
	 IOM().isUsable(storedid) && IOM().implExists(storedid) )
    {
	PtrMan<ElasticPropSelection> storedelpropsel = getByDBKey( storedid );
	if ( storedelpropsel && storedelpropsel->isOK(nullptr) )
	{
	    *this = *storedelpropsel.ptr();
	    storedid_ = storedid;
	    return true;
	}
    }

    PtrMan<IOPar> elasticpar = par.subselect( sKeyElasticProp() );
    if ( !elasticpar )
	return false;

    int elasticsz = 0;
    elasticpar->get( sKeyElasticsSize, elasticsz );
    if ( elasticsz < 2 )
	return false;

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

	const Mnemonic* mn = getByType( tp, elasticnm );
	if ( !mn || mn->isUdf() )
	{
	    errocc = true;
	    continue;
	}

	PtrMan<ElasticPropertyRef> epref =
				   new ElasticPropertyRef( *mn, elasticnm );
	epref->usePar( *elasticproprefpar );
	if ( !epref->isOK(nullptr) )
	{
	    errocc = true;
	    continue;
	}

	const ElasticFormula* eform = epref->formula();
	if ( eform && !checkForValidSelPropsDesc(*eform,*mn,faultynms,corrnms) )
	{
	    errocc = true;
	    continue;
	}

	if ( !errocc )
	{
	    ElasticPropertyRef* existsepref = getByType( tp );
	    if ( existsepref )
		*existsepref = *epref.ptr();
	    else
		add( epref.release() );
	}

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


bool ElasticPropSelection::setFor( const PropertyRefSelection& prs )
{
    ElasticPropGuess propguess( prs, *this );

    return propguess.isOK();
}


bool ElasticPropSelection::isOK( const PropertyRefSelection* prs ) const
{
    for ( const auto* pr : *this )
    {
	const auto& epr = sCast(const ElasticPropertyRef&,*pr);
	if ( !epr.isOK(prs) )
	    return false;
    }

    return errmsg_.isEmpty();
}


bool ElasticPropSelection::isOK( const TypeSet<ElasticFormula::Type>& reqtypes,
				 const PropertyRefSelection& prs,
				 uiString* msg ) const
{
    for ( const auto& reqtyp : reqtypes )
	if ( !getByType(reqtyp) )
	    return false;

    return isOK( &prs ) && isValidInput( msg );
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
    const Mnemonic& mn = ElasticFormula::getMnemonic( tp );
    const PropertyRefSelection prs( mn );
    const PropertyRef* pr = prs.getByName( nm );
    return pr ? &pr->mn() : &mn;
}


bool ElasticPropSelection::checkForValidSelPropsDesc(
		    const ElasticFormula& formula, const Mnemonic& mn,
		    BufferStringSet& faultynms, BufferStringSet& corrnms )
{
    if ( !ePROPS().ensureHasElasticProps() )
    {
	corrnms.add( "<No Valid Suggestion>" );
	return false;
    }

    const BufferString formexptrstr( formula.text() );
    if ( !formula.isOK() )
    {
	corrnms.add( "<Invalid formula>" );
	return false;
    }

    PropertyRefSelection prs, stdprs, allprs;
    prs = PropertyRefSelection( mn );
    stdprs = PropertyRefSelection( mn.stdType() );
    allprs = PropertyRefSelection( true, nullptr );

    bool noerror = true;
    for ( int iinp=0; iinp<formula.nrInputs(); iinp++ )
    {
	const BufferString& inpdef = formula.inputDef( iinp );
	if ( formula.isConst(iinp) )
	{
	    if ( !inpdef.isNumber() )
	    {
		faultynms.add( inpdef );
		noerror = false;
	    }

	    continue;
	}
	else if ( formula.isSpec(iinp) )
	    continue;

	const PropertyRef* pr = prs.getByName( inpdef );
	if ( !pr )
	    pr = stdprs.getByName( inpdef );
	if ( !pr )
	    pr = allprs.getByName( inpdef );
	if ( !pr )
	{
	    faultynms.add( inpdef );
	    noerror = false;
	    corrnms.add( "<No Valid Suggestion>" );
	    continue;
	}

	if ( inpdef != pr->name() )
	{
	    corrnms.add( pr->name() );
	    const_cast<BufferString&>( inpdef ).set( pr->name() );
	}
    }

    return noerror;
}
