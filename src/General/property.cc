/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/

static const char* rcsID = "$Id: property.cc,v 1.17 2010-09-27 10:00:11 cvsbert Exp $";

#include "propertyimpl.h"
#include "propertyref.h"
#include "mathexpression.h"
#include "survinfo.h"
#include "ascstream.h"
#include "safefileio.h"
#include "ioman.h"
#include "separstr.h"
#include "globexpr.h"
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


const PropertyRef& PropertyRef::undef()
{
    static PropertyRef* udf = 0;
    if ( !udf )
    {
	udf = new PropertyRef( "Undef" );
	udf->aliases().add( "" );
	udf->aliases().add( "undef*" );
	udf->aliases().add( "?undef?" );
	udf->aliases().add( "?undefined?" );
	udf->aliases().add( "udf" );
    }
    return *udf;
}


PropertyRef::StdType PropertyRef::surveyZType()
{
    return SI().zIsTime() ? Time : Dist;
}


bool PropertyRef::isKnownAs( const char* nm ) const
{
    if ( !nm || !*nm )
	return this == &undef();

    if ( caseInsensitiveEqual(nm,name().buf(),0) )
	return true;
    for ( int idx=0; idx<aliases_.size(); idx++ )
    {
	GlobExpr ge( aliases_.get(idx), false );
	if ( ge.matches(nm) )
	    return true;
    }
    return false;
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


int PropertyRefSet::indexOf( const char* nm ) const
{
    if ( nm && *nm )
    {
	for ( int idx=0; idx<size(); idx++ )
	{
	    const PropertyRef* pr = (*this)[idx];
	    if ( (*this)[idx]->isKnownAs(nm) )
		return idx;
	}
    }
    return -1;
}


PropertyRef* PropertyRefSet::gt( const char* nm ) const
{
    const int idx = indexOf( nm );
    return idx < 0 ? 0 : const_cast<PropertyRef*>( (*this)[idx] );
}


int PropertyRefSet::add( PropertyRef* pr )
{
    if ( !pr ) return -1;

    const int idx = indexOf( pr->name() );
    if ( idx < 0 )
	{ *this += pr; return size()-1; }

    return -1;
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
	PropertyRef* pr = new PropertyRef( astrm.keyWord(), st );
	for ( int ifms=1; ifms<sz; ifms++ )
	    pr->aliases().add( fms[ifms] );

	if ( add(pr) < 0 )
	    delete pr;
    }
}


bool PropertyRefSet::writeTo( ascostream& astrm ) const
{
    astrm.putHeader( "Properties" );
    for ( int idx=0; idx<size(); idx++ )
    {
	const PropertyRef& pr = *(*this)[idx];
	FileMultiString fms( eString(PropertyRef::StdType,pr.stdType()) );
	for ( int ial=0; ial<pr.aliases().size(); ial++ )
	    fms += pr.aliases().get( ial );
	astrm.put( pr.name(), fms );
    }
    return astrm.stream().good();
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
    if ( p == this )
	return true;
    else if ( !p )
	return false;

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
    if ( !expr_ )
	return mUdf(float);

    for ( int idx=0; idx<inps_.size(); idx++ )
    {
	const Property* p = inps_[idx];
	if ( !p )
	    return mUdf(float);

	const float v = inps_[idx]->value();
	if ( mIsUdf(v) )
	    return mUdf(float);

	expr_->setVariableValue( idx, v );
    }

    return expr_->getValue();
}


int PropertySet::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const Property& p = *(*this)[idx];
	if ( p.ref().name() == nm )
	    return idx;
    }
    for ( int idx=0; idx<size(); idx++ )
    {
	const Property& p = *(*this)[idx];
	if ( p.ref().isKnownAs(nm) )
	    return idx;
    }
    return -1;
}


Property* PropertySet::gt( const char* nm ) const
{
    const int idx = indexOf(nm);
    return idx < 0 ? 0 : const_cast<Property*>( (*this)[idx] );
}


bool PropertySet::prepareEval()
{
    for ( int idx=0; idx<size(); idx++ )
    {
	Property* p = (*this)[idx];
	mDynamicCastGet(MathProperty*,mp,p)
	if ( !mp ) continue;

	const int nrinps = mp->nrInputs();
	for ( int idep=0; idep<nrinps; idep++ )
	{
	    const char* nm = mp->inputName( idep );
	    const Property* depp = get( nm );
	    if ( !depp )
	    {
		errmsg_ = "Missing input for '";
		errmsg_.add(mp->ref().name()).add("': '").add(nm).add("'");
		return false;
	    }
	    mp->setInput( idep, depp );
	}
    }

    return true;
}
