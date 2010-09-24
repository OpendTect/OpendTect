/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/

static const char* rcsID = "$Id: property.cc,v 1.16 2010-09-24 13:39:22 cvsbert Exp $";

#include "propertyimpl.h"
#include "propertyref.h"
#include "mathexpression.h"
#include "survinfo.h"
#include "ascstream.h"
#include "safefileio.h"
#include "ioman.h"
#include "separstr.h"
#include "repos.h"
#include "errh.h"

static const char* filenamebase = "Properties";

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


PropertyRef::StdType PropertyRef::surveyZType()
{
    return SI().zIsTime() ? Time : Dist;
}


const PropertyRef& PropertyRef::undef()
{
    static PropertyRef udf( "undef" );
    return udf;
}


class PropertyRefSetMgr : public CallBacker
{
public:

PropertyRefSetMgr()
    : prs_(0)
{
    IOM().surveyChanged.notify( mCB(this,PropertyRefSetMgr,doNull) );
}

void doNull( CallBacker* )
{
    prs_ = 0;
}

void getSet()
{
    Repos::FileProvider rfp( filenamebase, true );
    while ( rfp.next() )
    {
	const BufferString fnm( rfp.fileName() );
	SafeFileIO sfio( fnm, true );
	if ( !sfio.open(true) )
	    continue;

	ascistream astrm( sfio.istrm(), true );
	PropertyRefSet* tmp = new PropertyRefSet;
	tmp->readFrom( astrm );
	if ( tmp->isEmpty() )
	    delete tmp;
	else
	    { prs_ = tmp; break; }
    }

    if ( !prs_ )
	prs_ = new PropertyRefSet;
}

    PropertyRefSet*	prs_;

};

const PropertyRefSet& PROPS()
{
    PropertyRefSetMgr rsm;
    if ( !rsm.prs_ )
	rsm.getSet();
    return *rsm.prs_;
}


PropertyRef* PropertyRefSet::gt( const char* nm ) const
{
    if ( !nm || !*nm ) return 0;

    for ( int idx=0; idx<size(); idx++ )
    {
	if ( caseInsensitiveEqual((*this)[idx]->name().buf(),nm,0) )
	    return const_cast<PropertyRef*>((*this)[idx]);
    }
    return 0;
}


void PropertyRefSet::add( PropertyRef* pr )
{
    if ( !pr ) return;

    const PropertyRef* mypr = get( pr->name() );
    if ( mypr )
	delete replace( indexOf(mypr), pr );
    else
	*this += pr;
}


void PropertyRefSet::readFrom( ascistream& astrm )
{
    deepErase( *this );

    while ( !atEndOfSection( astrm.next() ) )
    {
	FileMultiString fms( astrm.value() );
	const int sz = fms.size();
	if ( sz < 1 ) continue;

	BufferString ptypestr = fms[0];
	PropertyRef::StdType st = eEnum(PropertyRef::StdType,ptypestr);
	bool hc = sz > 1 ? yesNoFromString(fms[1]) : false;

	add( new PropertyRef(astrm.keyWord(),st,hc) );
    }
}


bool PropertyRefSet::writeTo( ascostream& astrm ) const
{
    astrm.putHeader( "Properties" );
    for ( int idx=0; idx<size(); idx++ )
    {
	const PropertyRef& pr = *(*this)[idx];
	FileMultiString fms( eString(PropertyRef::StdType,pr.stdType()) );
	fms += getYesNoString( pr.hcAffected() );
	astrm.put( pr.name(), fms );
    }
    return astrm.stream().good();
}


bool PropertyRefSet::save( Repos::Source src ) const
{
    Repos::FileProvider rfp( filenamebase );
    BufferString fnm = rfp.fileName( src );

    SafeFileIO sfio( fnm, true );
    if ( !sfio.open(false) )
    {
	BufferString msg( "Cannot write to " ); msg += fnm;
	ErrMsg( sfio.errMsg() );
	return false;
    }

    ascostream astrm( sfio.ostrm() );
    if ( !writeTo(astrm) )
	{ sfio.closeFail(); return false; }

    sfio.closeSuccess();
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
    MathExpressionParser mep( def_ );
    delete expr_; expr_ = mep.parse();
    if ( !expr_ ) return;
    const int sz = expr_->nrVariables();
    while ( sz > inps_.size() )
	inps_ += 0;
}


int MathProperty::nrInputs() const
{
    return expr_ ? expr_->nrVariables() : 0;
}


const char* MathProperty::inputName( int idx ) const
{
    return expr_ ? expr_->fullVariableExpression(idx) : 0;
}


void MathProperty::setInput( int idx, const Property* p )
{
    if ( p && p->dependsOn(this) )
    {
	BufferString msg( "Invalid cyclic dependency for property " );
	msg += ref().name();
	ErrMsg( msg );
	p = 0;
    }
    inps_.replace( idx, p );
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
    if ( !expr_ ) return mUdf(float);

    for ( int idx=0; idx<inps_.size(); idx++ )
    {
	const Property* p = inps_[idx];
	if ( !p ) return mUdf(float);
	expr_->setVariableValue( idx, inps_[idx]->value() );
    }

    return expr_->getValue();
}
