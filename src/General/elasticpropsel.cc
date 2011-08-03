/*+
________________________________________________________________________

    (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
    Author:        Bruno
    Date:          May 2011
________________________________________________________________________

-*/

static const char* rcsID = "$Id: elasticpropsel.cc,v 1.6 2011-08-03 15:17:51 cvsbruno Exp $";

#include "ascstream.h"
#include "streamconn.h"
#include "keystrs.h"
#include "elasticpropsel.h"
#include "file.h"
#include "filepath.h"
#include "math.h"
#include "mathexpression.h"
#include "strmprov.h"


static const char* filenamebase 	= "ElasticFormulas";

static const char* sKeyFormulaName 	= "Name of formula";
static const char* sKeyMathExpr 	= "Mathetmatic Expression";
static const char* sKeySelVars 		= "Selected properties";
static const char* sKeyType 		= "Type";

DefineEnumNames(ElasticFormula,ElasticType,0,"Elastic Property")
{ "Density", "P-Wave", "S-Wave", 0 };

defineTranslatorGroup(ElasticPropSelection,"Elastic Property Selection");

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
    par.set( sKeyType, getElasticTypeString( type_ ) );
    par.set( sKeyMathExpr, expression_ );
    par.set( sKeySelVars, variables_ );
}


void ElasticFormula::usePar( const IOPar& par ) 
{
    BufferString nm; par.get( sKeyFormulaName, nm ); setName( nm );
    parseEnumElasticType( par.find( sKeyType ), type_ );
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
					    ElasticFormula::ElasticType tp,
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


void ElasticFormulaRepository::getByType( ElasticFormula::ElasticType tp,
					    TypeSet<ElasticFormula>& efs ) const
{
    for ( int idx=0; idx<formulas_.size(); idx++ )
    {
	if ( formulas_[idx].type() == tp )
	   efs += formulas_[idx]; 
    }
}



ElasticPropSelection::ElasticPropSelection( const char* nm )
    : NamedObject(nm)
{
    const char** props = ElasticFormula::ElasticTypeNames();
    for ( int idx=0; props[idx]; idx++ )
    {
	ElasticFormula::ElasticType tp;
	ElasticFormula::parseEnumElasticType( props[idx], tp );
	selectedformulas_ += ElasticFormula( "","", tp ); 
    }
}


ElasticFormula& ElasticPropSelection::getFormula(ElasticFormula::ElasticType tp)
{
    for ( int idx=0; idx<selectedformulas_.size(); idx++ )
    {
	if ( selectedformulas_[idx].type() == tp )
	    return selectedformulas_[idx];
    }
    return selectedformulas_[0];
}


const ElasticFormula& ElasticPropSelection::getFormula( 
					ElasticFormula::ElasticType tp ) const
{
    for ( int idx=0; idx<selectedformulas_.size(); idx++ )
    {
	if ( selectedformulas_[idx].type() == tp )
	return selectedformulas_[idx];
    }
    return selectedformulas_[0];
}



bool ElasticPropSelection::isValidInput() const
{
    bool isvalid = true;
    for ( int idx=0; idx<selectedformulas_.size(); idx++ )
    {
	//TODO support SWave 
	if ( selectedformulas_[idx].type() == ElasticFormula::SVel )
	    continue;

	if ( selectedformulas_[idx].variables().isEmpty() )
	    isvalid = false;
    }
    return isvalid;
}



ElasticPropGuess::ElasticPropGuess( const PropertyRefSelection& pps,
				    ElasticPropSelection& sel )
    : elasticprops_(sel)
{
    const char** props = ElasticFormula::ElasticTypeNames();
    for ( int idx=0; props[idx]; idx++ )
    {
	ElasticFormula::ElasticType tp;
	ElasticFormula::parseEnumElasticType( props[idx], tp );
	guessQuantity( pps, tp );
    }
}


void ElasticPropGuess::guessQuantity( const PropertyRefSelection& pps, 
					ElasticFormula::ElasticType tp )
{
    ElasticFormula& fm = elasticprops_.getFormula( tp );
    if ( !fm.variables().isEmpty() )
	return;

    for ( int idx=0; idx<pps.size(); idx++ )
    {
	if ( pps[idx]->stdType() == elasticToStdType( tp ) )
	    { fm.variables().add( pps[idx]->name() ); break; }

	//TODO search best formula by name from the repos when not found
    }
}


PropertyRef::StdType ElasticPropGuess::elasticToStdType( 
					ElasticFormula::ElasticType tp ) const
{
    if ( tp == ElasticFormula::PVel || tp == ElasticFormula::SVel )
	return PropertyRef::Vel;
    if ( tp == ElasticFormula::Den )
	return PropertyRef::Den;

    return PropertyRef::Other;
}





void ElasticPropGen::fill( AILayer& el, const float* vals, int sz )
{
    el.den_ = setVal( elasticprops_.getFormula(ElasticFormula::Den), vals, sz );
    el.vel_ = setVal( elasticprops_.getFormula(ElasticFormula::PVel), vals, sz);
}


void ElasticPropGen::fill( ElasticLayer& el, const float* vals, int sz )
{
    el.den_ = setVal( elasticprops_.getFormula(ElasticFormula::Den), vals, sz);
    el.vel_ = setVal( elasticprops_.getFormula(ElasticFormula::PVel), vals, sz);
    el.svel_ = setVal(elasticprops_.getFormula(ElasticFormula::SVel), vals, sz);
}


float ElasticPropGen::setVal(const ElasticFormula& ef,const float* vals,int sz)
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

	if ( !expr ) 
	    break;

	expr->setVariableValue( idx, val );
    }
    return expr ? expr->getValue() : val;
}





ElasticPropSelection* ElasticPropSelection::get( const IOObj* ioobj )
{
    if ( !ioobj ) return 0;
    ElasticPropSelectionTranslator* tr = 
		(ElasticPropSelectionTranslator*)ioobj->getTranslator();

    if ( !tr ) return 0;
    ElasticPropSelection* newelprop = 0;

    Conn* connptr = ioobj->getConn( Conn::Read );
    if ( connptr && !connptr->bad() )
    {
	newelprop = new ElasticPropSelection;
	if ( !tr->read( newelprop, *connptr ) )
	{
	    ErrMsg( "Problem reading ElasticPropSelection from file" );
	    delete newelprop;
	    newelprop = 0;
	}
    }
    else
	ErrMsg( "Cannot open ElasticPropSelection file" );

    delete connptr; delete tr;
    return newelprop;
}


bool ElasticPropSelection::put( const IOObj* ioobj ) const
{
    if ( !ioobj ) return false;
    ElasticPropSelectionTranslator* tr = 
		(ElasticPropSelectionTranslator*)ioobj->getTranslator();
    if ( !tr ) return false;
    bool retval = false;

    Conn* connptr = ioobj->getConn( Conn::Write );
    if ( connptr && !connptr->bad() )
    {
	if ( tr->write( this, *connptr ) )
	    retval = true;
	else
	    ErrMsg( "Cannot write ElasticPropSelection" );
    }
    else
	ErrMsg( "Cannot open ElasticPropSelection file for write" );

    delete connptr; delete tr;
    return retval;
}



int ElasticPropSelectionTranslatorGroup::selector( const char* key )
{
    int retval = defaultSelector( theInst().userName(), key );
    if ( retval ) return retval;

    if ( defaultSelector("ElasticPropSelection directory",key)
	|| defaultSelector("Seismic directory",key) ) return 1;
    return 0;
}

mDefSimpleTranslatorioContext(ElasticPropSelection,Seis)


bool ElasticPropSelectionTranslator::read( ElasticPropSelection* eps, 
						Conn& conn)
{
    if ( !conn.forRead() || !conn.isStream() )  return false;

    ascistream astream( ((StreamConn&)conn).iStream() );
    if ( !astream.isOfFileType(mTranslGroupName(ElasticPropSelection)) )
	return false;

    while ( !atEndOfSection( astream.next() ) )
    {
	IOPar iop; iop.getFrom( astream );
	/*
	eps->denformula_.usePar( iop ); 
	eps->pvelformula_.usePar( iop );
	eps->svelformula_.usePar( iop );
	if ( astream.hasKeyword( sLength ) )
	else if ( astream.hasKeyword( sIndex ) )
	else if ( astream.hasKeyword(sKey::Name) )
	else if ( astream.hasKeyword( sSampRate ) )
	*/
    }
    return astream.stream().good();
}



bool ElasticPropSelectionTranslator::write( const ElasticPropSelection* eps, 
						Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() ) return false;

    ascostream astream( ((StreamConn&)conn).oStream() );
    const BufferString head( mTranslGroupName(ElasticPropSelection), " file" );
    if ( !astream.putHeader( head ) ) return false;

    /*
    if ( *(const char*)eps->name() ) astream.put( sKey::Name, eps->name() );
    IOPar iop; 
    eps->denformula_.fillPar( iop ); iop.putTo( astream ); iop.setEmpty();
    astream.newParagraph();
    eps->pvelformula_.fillPar( iop ); iop.putTo( astream ); iop.setEmpty();
    astream.newParagraph();
    eps->svelformula_.fillPar( iop ); iop.putTo( astream ); iop.setEmpty();
    astream.newParagraph();
    */

    return astream.stream().good();
}


