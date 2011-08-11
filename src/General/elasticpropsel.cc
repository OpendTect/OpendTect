/*+
________________________________________________________________________

    (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
    Author:        Bruno
    Date:          May 2011
________________________________________________________________________

-*/

static const char* rcsID = "$Id: elasticpropsel.cc,v 1.10 2011-08-11 13:47:30 cvsbruno Exp $";


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


#define mFileType "Elastic Property Selection"


static const char* filenamebase 	= "ElasticFormulas";

static const char* sKeyFormulaName 	= "Name of formula";
static const char* sKeyMathExpr 	= "Mathetmatic Expression";
static const char* sKeySelVars 		= "Selected properties";
static const char* sKeyType 		= "Type";
static const char* sKeyPropertyName 	= "Property name";

mDefSimpleTranslators(ElasticPropSelection,mFileType,od,Seis);

DefineEnumNames(ElasticFormula,Type,0,"Elastic Property")
{ "Density", "P-Wave", "S-Wave", 0 };


ElasticFormula& ElasticFormula::operator =( const ElasticFormula& ef )
{
    if ( this != &ef )
    {
	setName( ef.name() );
	type_ = ef.type_;
	expression_ = ef.expression_;
	variables_ = ef.variables_;
    }
    return *this;
}


void ElasticFormula::fillPar( IOPar& par ) const
{
    par.set( sKeyFormulaName, name() );
    par.set( sKeyType, getTypeString( type_ ) );
    par.set( sKeyMathExpr, expression_ );
    par.set( sKeySelVars, variables_ );
}


void ElasticFormula::usePar( const IOPar& par ) 
{
    BufferString nm; par.get( sKeyFormulaName, nm ); setName( nm );
    parseEnumType( par.find( sKeyType ), type_ );
    par.get( sKeyMathExpr, expression_ );
    par.get( sKeySelVars, variables_ );
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
    }
    return *elasticrepos;
}


ElasticFormulaRepository::ElasticFormulaRepository()
{
    Repos::FileProvider rfp( filenamebase );
    while ( rfp.next() )
	addFormulasFromFile( rfp.fileName(), rfp.source() );
}


void ElasticFormulaRepository::addFormulasFromFile( const char* fnm, 
						    Repos::Source src )
{
    if ( !File::exists(fnm) ) return;
    StreamData sd = StreamProvider( fnm ).makeIStream();
    if ( !sd.usable() ) return;

    ascistream stream( *sd.istrm, true );
    while ( !atEndOfSection( stream.next() ) )
    {
	IOPar iop; iop.getFrom( stream );
	if ( iop.isEmpty() )
	    continue;
	ElasticFormula fm("","",ElasticFormula::Den); 
	fm.usePar( iop );
	formulas_ += fm;
    }

    sd.close();
}


bool ElasticFormulaRepository::write( Repos::Source src ) const
{
    Repos::FileProvider rfp( filenamebase );
    const BufferString fnm = rfp.fileName( src );

    StreamData sd = StreamProvider( fnm ).makeOStream();
    if ( !sd.usable() )
    {
	BufferString msg( "Cannot write to " ); msg += fnm;
	ErrMsg( fnm );
	return false;
    }

    ascostream strm( *sd.ostrm );
    strm.putHeader( "Elastic formulas" );
    for ( int idx=0; idx<formulas_.size(); idx++ )
    {
	IOPar iop; 
	formulas_[idx].fillPar( iop );
	iop.putTo( strm );
    }

    sd.close();
    return true;
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


const PropertyRef::StdType 
	ElasticPropertyRef::elasticToStdType(ElasticFormula::Type tp ) 
{
    if ( tp == ElasticFormula::PVel || tp == ElasticFormula::SVel )
	return PropertyRef::Vel;
    if ( tp == ElasticFormula::Den )
	return PropertyRef::Den;

    return PropertyRef::Other;
}




ElasticPropSelection::ElasticPropSelection( const char* nm )
    : NamedObject(nm)
{
    const char** props = ElasticFormula::TypeNames();
    for ( int idx=0; props[idx]; idx++ )
    {
	ElasticFormula::Type tp;
	ElasticFormula::parseEnumType( props[idx], tp );
	elasticprops_ += ElasticPropertyRef( props[idx],
					ElasticFormula( "","", tp ) );
    }
}


ElasticPropSelection& 
	ElasticPropSelection::operator =( const ElasticPropSelection& eps )
{
    if ( this != &eps )
    {
	setName( eps.name() );
	elasticprops_.erase();
	for ( int idx=0; idx<eps.getPropertyRefs().size(); idx++ )
	    elasticprops_ += eps.getPropertyRefs()[idx];
    }
    return *this;
}


ElasticPropertyRef& ElasticPropSelection::getPropertyRef(ElasticFormula::Type t)
{
    for ( int idx=0; idx<elasticprops_.size(); idx++ )
    {
	if ( elasticprops_[idx].elasticType() == t )
	    return elasticprops_[idx];
    }
    return elasticprops_[0];
}


const ElasticPropertyRef& ElasticPropSelection::getPropertyRef( 
					ElasticFormula::Type tp ) const
{
    for ( int idx=0; idx<elasticprops_.size(); idx++ )
    {
	if ( elasticprops_[idx].elasticType() == tp )
	return elasticprops_[idx];
    }
    return elasticprops_[0];
}



bool ElasticPropSelection::isValidInput( BufferString* errmsg ) const
{
    for ( int idx=0; idx<elasticprops_.size(); idx++ )
    {
	const char* propnm = elasticprops_[idx].name();
	const BufferStringSet& vars = elasticprops_[idx].formula().variables();
	if ( vars.isEmpty() )
	 {
	    if ( errmsg )
	    {	
		*errmsg = "No variable specified for "; 
		*errmsg += propnm; 
	    }
	    return false; 
	 }

	if ( !elasticprops_[idx].formula().expression() )
	    continue;

	if ( vars.isPresent( elasticprops_[idx].name() ) )
	{ 
	    if ( errmsg )
	    {	
		*errmsg += propnm; 
		*errmsg += " is dependent on itself"; 
	    }
	    return false; 
	}

	for ( int idpr=0; idpr<elasticprops_.size(); idpr++ )
	{
	    if ( idpr == idx )
		continue;

	    const ElasticPropertyRef& elpr = elasticprops_[idpr];
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


bool ElasticPropSelection::isPresent( const char* propname ) const
{
    BufferStringSet elpropnms;
    for ( int idx=0; idx<elasticprops_.size(); idx++ )
	elpropnms.addIfNew( elasticprops_[idx].name() );

    return elpropnms.isPresent( propname );
}


int ElasticPropSelection::indexOf( const char* propname ) const
{
    BufferStringSet elpropnms;
    for ( int idx=0; idx<elasticprops_.size(); idx++ )
	elpropnms.addIfNew( elasticprops_[idx].name() );

    return elpropnms.indexOf( propname );
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
    ElasticFormula& fm = elasticprops_.getPropertyRef( tp ).formula();
    if ( !fm.variables().isEmpty() )
	return;

    for ( int idx=0; idx<pps.size(); idx++ )
    {
	if ( pps[idx]->stdType() == ElasticPropertyRef::elasticToStdType( tp ) )
	    { fm.variables().add( pps[idx]->name() ); break; }

	//TODO search best formula by name from the repos when not found
    }
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
	    val = vals[refprops_.indexOf(var)];
	else if ( elasticprops_.isPresent( var ) )
	{
	    const int propidx = elasticprops_.indexOf(var);
	    val = getVal( elasticprops_.getPropertyRefs()[propidx], vals, sz );
	}

	if ( !expr ) 
	    break;

	expr->setVariableValue( idx, val );
    }
    return expr ? expr->getValue() : val;
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
	    eps->getPropertyRef( tp ).formula().usePar( iop ); 
	    BufferString nm; iop.get( sKeyPropertyName, nm ); 
	    eps->getPropertyRef( tp ).setName(nm);
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
	for ( int idx=0; idx<getPropertyRefs().size(); idx++ )
	{
	    iop.set( sKeyPropertyName, getPropertyRefs()[idx].name() ); 
	    getPropertyRefs()[idx].formula().fillPar( iop ); 
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


