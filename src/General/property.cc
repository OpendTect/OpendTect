/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/

static const char* rcsID = "$Id: property.cc,v 1.4 2004-02-20 16:10:07 bert Exp $";

#include "propertyimpl.h"
#include "mathexpression.h"
#include "filegen.h"
#include "ascstream.h"
#include "strmprov.h"
#include "separstr.h"
#include "debug.h"
#include "errh.h"


DefineEnumNames(PropertyRef,StdType,0,"Standard Property")
{
	"Other",
	"Time",
	"Distance/Depth",
	"Density",
	"Velocity",
	"Sonic travel time",
	"Acoustic Impedance",
	"Porosity",
	"Permeability",
	"Saturation",
	"Gamma Ray",
	"Electrical Potential",
	"Resistivity",
	"Poisson's Ratio",
	"Compressibility",
	"Temperature",
	"Pressure",
	0
};


PropertyRefRepository& PrRR()
{
    static PropertyRefRepository* prrepo = 0;
    if ( !prrepo )
    {
	if ( DBG::isOn() ) DBG::message( "Creating PropertyRefRepository" );
	prrepo = new PropertyRefRepository;
	if ( DBG::isOn() )
	{
	    BufferString msg( "Total properties found: " );
	    msg += prrepo->all().size();
	    DBG::message( msg );
	}
    }
    return *prrepo;
}


PropertyRefRepository::PropertyRefRepository()
{
    BufferString fnm = GetDataFileName( "StdProperties" );
    addFromFile( fnm, 0 );

    fnm = GetSettingsDir();
    fnm = File_getFullPath( fnm, "properties" );
    addFromFile( fnm, 1 );

    getSurvPropFileNm( fnm );
    addFromFile( fnm, 2 );
}


void PropertyRefRepository::getSurvPropFileNm( BufferString& fnm ) const
{
    fnm = GetDataDir();
    fnm = File_getFullPath( fnm, ".properties" );
}


void PropertyRefRepository::addFromFile( const char* fnm, int orign )
{
    if ( !File_exists(fnm) ) return;
    StreamData sd = StreamProvider( fnm ).makeIStream();
    if ( !sd.usable() ) return;

    ascistream stream( *sd.istrm, true );
    while ( !atEndOfSection( stream.next() ) )
    {
	FileMultiString fms( stream.value() );
	const int sz = fms.size();
	if ( sz < 1 ) continue;

	BufferString ptypestr = fms[0];
	PropertyRef::StdType st = eEnum(PropertyRef::StdType,ptypestr);
	bool hc = sz > 1 ? yesNoFromString(fms[1]) : false;
	PropertyRef pr( stream.keyWord(), st, hc );
	for ( int idx=2; idx<sz; idx++ )
	    pr.specialUnitsOfMeasure().add( fms[idx] );

	pr.origin_ = orign;
	set( pr );
    }

    sd.close();
}


const PropertyRef* PropertyRefRepository::get( const char* nm ) const
{
    if ( !nm || !*nm ) return 0;

    for ( int idx=0; idx<entries.size(); idx++ )
    {
	if ( caseInsensitiveEqual(entries[idx]->name().buf(),nm,0) )
	    return entries[idx];
    }
    return 0;
}


bool PropertyRefRepository::set( const PropertyRef& pr )
{
    const PropertyRef* mypr = get( pr.name() );
    if ( mypr )
    {
	PropertyRef& upd = *const_cast<PropertyRef*>( mypr );
	upd = pr;
	return false;
    }
    else
    {
	entries += new PropertyRef( pr );
	return true;
    }
}


bool PropertyRefRepository::write() const
{
    BufferString fnm; getSurvPropFileNm( fnm );

    bool havesurvspec = false;
    for ( int idx=0; idx<entries.size(); idx++ )
    {
	if ( entries[idx]->surveySpecific() )
	    { havesurvspec = true; break; }
    }
    if ( !havesurvspec )
	return File_remove( fnm, NO );

    StreamData sd = StreamProvider( fnm ).makeOStream();
    if ( !sd.usable() ) return false;

    ascostream strm( *sd.ostrm );
    strm.putHeader( "Survey Properties" );
    for ( int idx=0; idx<entries.size(); idx++ )
    {
	const PropertyRef& pr = *entries[idx];
	if ( !pr.surveySpecific() ) continue;

	FileMultiString fms( eString(PropertyRef::StdType,pr.stdType()) );
	fms += getYesNoString( pr.hcAffected() );
	for ( int iun=0; iun<pr.units_.size(); iun++ )
	    fms += pr.units_.get( iun );
	strm.put( fms );
    }

    sd.close();
    return true;
}


MathProperty::~MathProperty()
{
    delete expr_;
}


void MathProperty::setDef( const char* s )
{
    inps_.erase();
    def_ = s;
    delete expr_; expr_ = MathExpression::parse( def_ );
    if ( !expr_ ) return;
    const int sz = expr_->getNrVariables();
    while ( sz > inps_.size() )
	inps_ += 0;
}


int MathProperty::nrInputs() const
{
    return expr_ ? expr_->getNrVariables() : 0;
}


const char* MathProperty::inputName( int idx ) const
{
    return expr_ ? expr_->getVariableStr(idx) : 0;
}


void MathProperty::setInput( int idx, const Property* p )
{
    if ( p && p->dependsOn(this) )
    {
	BufferString msg( "Invalid cyclic dependency for property " );
	msg += ref()->name();
	ErrMsg( msg );
	p = 0;
    }
    inps_.replace( p, idx );
}


bool MathProperty::dependsOn( const Property* p ) const
{
    if ( p == this ) return true;
    else if ( !p ) return false;

    for ( int idx=0; idx<inps_.size(); idx++ )
    {
	const Property* inp = inps_[idx];
	if ( inp && inp->dependsOn(p) )
	    return true;
    }
    return false;
}


float MathProperty::value() const
{
    if ( !expr_ ) return mUndefValue;

    for ( int idx=0; idx<inps_.size(); idx++ )
    {
	const Property* p = inps_[idx];
	if ( !p ) return mUndefValue;
	expr_->setVariable( idx, inps_[idx]->value() );
    }

    return expr_->getValue();
}
