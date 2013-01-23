/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "mathproperty.h"
#include "propertyref.h"
#include "mathexpression.h"
#include "unitofmeasure.h"
#include "keystrs.h"
#include "iopar.h"
#include "separstr.h"
#include "errh.h"
#include <typeinfo>

static const PropertyRef depthpropref( "Depth", PropertyRef::Dist );
static const ValueProperty depthprop( depthpropref, 0 );
static const PropertyRef xpospropref( "XPos", PropertyRef::Volum );
static const ValueProperty xposprop( xpospropref, 0 );


const char* Property::name() const
{
    return ref_.name().buf();
}


bool Property::isEqualTo( const Property& oth ) const
{
    if ( typeid(*this) != typeid(oth) )
	return false;

    const BufferString mydef( def() ), othdef( oth.def() );
    return mydef == othdef;
}


void Property::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Name, name() );
    iop.set( sKey::Type, type() );
    iop.set( sKey::Value, def() );
}


void Property::usePar( const IOPar& iop )
{
    const char* res = iop.find( sKey::Value );
    if ( res && *res )
	setDef( res );
}


Property* Property::get( const IOPar& iop )
{
    const char* nm = iop.find( sKey::Name );
    if ( !nm || !*nm ) return 0;

    const PropertyRef* ref = PROPS().find( nm );
    if ( !ref && PropertyRef::thickness().name() == nm )
	ref = &PropertyRef::thickness();
    if ( !ref ) return 0;

    const char* typ = iop.find( sKey::Type );
    if ( !typ || !*typ ) typ = ValueProperty::typeStr();
    Property* prop = factory().create( typ, *ref );
    if ( prop )
	prop->usePar( iop );

    return prop;
}


const char* ValueProperty::def() const
{
    return ::toString( val_ );
}


void ValueProperty::setDef( const char* defstr )
{
    if ( defstr && *defstr )
	val_ = toFloat( defstr );
}


bool ValueProperty::isUdf() const
{
    return mIsUdf(val_);
}


float ValueProperty::gtVal( Property::EvalOpts ) const
{
    return val_;
}


const char* RangeProperty::def() const
{
    if ( isUdf() )
	return "1e30`0";

    static FileMultiString fms;
    fms = ::toString(rg_.start);
    fms += ::toString(rg_.stop);
    return fms.buf();
}


void RangeProperty::setDef( const char* defstr )
{
    if ( !defstr || !*defstr )
	rg_.start = mUdf(float);
    else
    {
	FileMultiString fms( defstr );
	rg_.start = toFloat( fms[0] );
	rg_.stop = toFloat( fms[1] );
    }
}


bool RangeProperty::isUdf() const
{
    return mIsUdf(rg_.start);
}


float RangeProperty::gtVal( Property::EvalOpts eo ) const
{
    if ( isUdf() )
	return mUdf(float);
    else if ( eo.isAvg() )
    {
	Interval<float> sanerg( rg_ );
	if ( mIsUdf(sanerg.start) )
	    sanerg.start = sanerg.stop;
	else if ( mIsUdf(sanerg.stop) )
	    sanerg.stop = sanerg.start;
	if ( &ref() == &PropertyRef::thickness() )
	{
	    if ( sanerg.start < 0 ) sanerg.start = 0;
	    if ( sanerg.stop < 0 ) sanerg.stop = 0;
	}
	return 0.5f * (sanerg.start + sanerg.stop);
    }

    return rg_.start + eo.relpos_ * (rg_.stop - rg_.start);
}


MathProperty::MathProperty( const PropertyRef& pr, const char* df )
    : Property(pr)
    , expr_(0)
    , uom_(0)
{
    inps_.allowNull( true ); inpunits_.allowNull( true );
    if ( df && *df )
	setDef( df );
}


MathProperty::MathProperty( const MathProperty& mp )
    : Property(mp.ref())
    , expr_(0)
    , uom_(mp.uom_)
{
    inps_.allowNull( true ); inpunits_.allowNull( true );
    if ( !mp.def_.isEmpty() )
	setDef( mp.def_ );
    inpunits_.erase();
    for ( int idx=0; idx<mp.inpunits_.size(); idx++ )
	inpunits_ += mp.inpunits_[idx];
}


MathProperty::~MathProperty()
{
    delete expr_;
}


static int getNrVars( const MathExpression* me, bool var )
{
    if ( !me ) return 0;
    const int nrvars = me->nrVariables();
    int ret = 0;
    for ( int idx=0; idx<nrvars; idx++ )
    {
	if (  (var && me->getType(idx) == MathExpression::Variable)
	  || (!var && me->getType(idx) == MathExpression::Constant) )
	    ret++;
    }
    return ret;
}


int MathProperty::nrInputs() const
{
    return getNrVars( expr_, true );
}


int MathProperty::nrConsts() const
{
    return getNrVars( expr_, false );
}


static const char* getVarName( const MathExpression* me, int nr, bool var )
{
    if ( !me || nr < 0 ) return 0;
    const int nrvars = me->nrVariables();
    int varnr = -1;
    for ( int idx=0; idx<nrvars; idx++ )
    {
	if (  (var && me->getType(idx) == MathExpression::Variable)
	  || (!var && me->getType(idx) == MathExpression::Constant) )
	    varnr++;
	if ( varnr == nr )
	    return me->fullVariableExpression( idx );
    }
    return 0;
}


const char* MathProperty::inputName( int idx ) const
{
    return getVarName( expr_, idx, true );
}


PropertyRef::StdType MathProperty::inputType( int idx ) const
{
    const Property* inp = inps_[idx];
    if ( !inp ) return PropertyRef::Other;

    return inp->ref().stdType();
}


const char* MathProperty::constName( int idx ) const
{
    return getVarName( expr_, idx, false );
}


void MathProperty::setInput( int idx, const Property* p )
{
    if ( p && p->dependsOn(*this) )
    {
	BufferString msg( "Invalid cyclic dependency for property " );
	msg += ref().name();
	ErrMsg( msg );
	p = 0;
    }
    inps_.replace( idx, p );
}


const UnitOfMeasure* MathProperty::inputUnit( int idx ) const
{
    return idx >= 0 && idx<inpunits_.size() ? inpunits_[idx] : 0;
}


void MathProperty::setInputUnit( int idx, const UnitOfMeasure* un )
{
    if ( idx < 0 || idx >= inps_.size() )
	return;
    while ( inpunits_.size() <= idx )
	addDefInpUnit();
    inpunits_.replace( idx, un );
}


float MathProperty::constValue( int idx ) const
{
    return idx < 0 || idx >= consts_.size() ? 0 : consts_[idx];
}


void MathProperty::setConst( int idx, float f )
{
    while ( consts_.size() <= idx )
	consts_ += 0;
    consts_[ idx ] = f;
}


bool MathProperty::dependsOn( const Property& p ) const
{
    if ( &p == this )
	return true;

    for ( int idx=0; idx<inps_.size(); idx++ )
    {
	const Property* inp = inps_[idx];
	if ( inp && inp->dependsOn(p) )
	    return true;
    }
    return false;
}


void MathProperty::ensureGoodVariableName( char* nm )
{
    if ( !nm || !*nm )
        { pFreeFnErrMsg("Knurft","ensureGoodVariableName"); return; }

    // squeeze out all crap
    const int len = strlen( nm );
    int curidx = 0;
    for ( int ich=0; ich<len; ich++ )
    {
	const char ch = nm[ich];
	if ( isalnum(ch) || ch == '_' )
	    { nm[curidx] = ch; curidx++; }
    }
    nm[curidx] = '\0';
}


static bool isMathMatch( const BufferString& reqnm, const char* str )
{
    BufferString depnm( str );
    MathProperty::ensureGoodVariableName( depnm.buf() );
    return depnm == reqnm;
}


const Property* MathProperty::findInput( const PropertySet& ps, const char* nm,
					 bool mainname ) const
{
    if ( !nm || !*nm || caseInsensitiveEqual(nm,"depth")
	    	     || caseInsensitiveEqual(nm,"z") )
	return &depthprop;
    else if ( caseInsensitiveEqual(nm,"xpos") )
	return &xposprop;

    BufferString reqnm( nm ); ensureGoodVariableName( reqnm.buf() );
    const Property* ret = 0;
    for ( int idx=0; idx<ps.size(); idx++ )
    {
	const Property& depp = ps.get( idx );
	if ( this == &depp ) continue;
	if ( mainname )
	{
	    if ( isMathMatch(reqnm,depp.name()) )
		ret = &depp;
	}
	else
	{
	    for ( int ial=0; ial<depp.ref().aliases().size(); ial++ )
	    {
		if ( isMathMatch(reqnm,depp.ref().aliases().get(ial).buf()) )
		    { ret = &depp; break; }
	    }
	}
	if ( ret ) break;
    }
    if ( !ret )
	return ret;

    mDynamicCastGet(const MathProperty*,mp,ret)
    return mp && mp->isDepOn(*this) ? 0 : ret;
}


bool MathProperty::isDepOn( const Property& p ) const
{
    if ( &p == this ) return true;
    
    for ( int idep=0; idep<inps_.size(); idep++ )
    {
	const Property* inp = inps_[idep];
	if ( inp == &p )
	    return true;

	mDynamicCastGet(const MathProperty*,mpinp,inp)
	if ( mpinp && mpinp->isDepOn(p) )
	    return true;
    }
    return false;
}


void MathProperty::addDefInpUnit() const
{
    const int inpidx = inpunits_.size();
    const Property* pr = inps_[inpidx];
    const UnitOfMeasure* uom = 0;
    if ( pr )
	uom = UoMR().get( pr->ref().stdType(), pr->ref().disp_.unit_ );
    inpunits_ += uom;
}


bool MathProperty::init( const PropertySet& ps ) const
{
    if ( !expr_ )
    {
	errmsg_ = "No valid definition for "; errmsg_.add(name());
	return false;
    }

    const int nrinps = expr_->nrVariables();
    inps_.erase();
    while ( nrinps > inps_.size() )
	inps_ += findInput( ps, inputName(inps_.size()), true );
    while ( inpunits_.size() < nrinps )
	addDefInpUnit();

    for ( int idep=0; idep<nrinps; idep++ )
    {
	if ( inps_[idep] ) continue;
	const char* nm = inputName( idep );
	inps_.replace( idep, findInput( ps, nm, false ) );
	if ( !inps_[idep] )
	{
	    errmsg_ = "Missing input or dependency loop for '";
	    errmsg_.add(name()).add("': '").add(nm).add("'");
	    return false;
	}
    }

    return true;
}


const char* MathProperty::def() const
{
    FileMultiString fms( def_ );
    const int nrconsts = nrConsts();
    for ( int idx=0; idx<nrconsts; idx++ )
    {
	BufferString cdef( "c", idx, "=" );
	cdef.add( constValue(idx) );
	fms += cdef;
    }
    if ( uom_ )
	fms += uom_->name();
    else
	fms.add( "" );

    for ( int idx=0; idx<inpunits_.size(); idx++ )
	fms.add( inpunits_[idx] ? inpunits_[idx]->name() : "" );

    fulldef_ = fms;
    return fulldef_.buf();
}


void MathProperty::setDef( const char* s )
{
    inps_.erase(); consts_.erase(); inpunits_.erase();
    FileMultiString fms( s );
    def_ = fms[0];
    MathExpressionParser mep( def_ );
    delete expr_; expr_ = mep.parse();
    if ( !expr_ ) return;

    const int varsz = getNrVars( expr_, true );
    const int constsz = getNrVars( expr_, false );
    const int fmssz = fms.size();

    for ( int idx=0; idx<fmssz; idx++ )
    {
	BufferString word( fms[idx] );
	const int wordlen = word.size();
	if ( wordlen < 3 || wordlen > 4 )
	    continue;
	if ( word[0] == 'c' && isdigit(word[1]) && word[2] == '=' )
	    consts_ += toFloat( word.buf() + 3 );
    }

    while ( constsz > consts_.size() )
	consts_ += 0;
    while ( varsz > inps_.size() )
	inps_ += 0;

    if ( fmssz > constsz+1 )
	uom_ = UoMR().get( ref_.stdType(), fms[constsz+1] );

    for ( int idx=0; idx<inps_.size(); idx++ )
    {
	const Property* inp = inps_[idx];
	if ( fmssz <= constsz + 2 + idx )
	    addDefInpUnit();
	else if ( !inp )
	    inpunits_ += UoMR().get( fms[constsz+2+idx] );
	else
	    inpunits_ += UoMR().get( inp->ref().stdType(), fms[constsz+2+idx] );
    }
}


bool MathProperty::isUdf() const
{
    return def_.isEmpty();
}


static int getMathVarIdx( const MathExpression& me, int nr, bool var )
{
    const int nrvars = me.nrVariables();
    int curnr = -1;
    for ( int idx=0; idx<nrvars; idx++ )
    {
	if (  (var && me.getType(idx) == MathExpression::Variable)
	  || (!var && me.getType(idx) == MathExpression::Constant) )
	    curnr++;
	if ( curnr == nr )
	    return idx;
    }
    pFreeFnErrMsg("Huh?","getMathVarIdx");
    return 0;
}


float MathProperty::gtVal( Property::EvalOpts eo ) const
{
    if ( !expr_ )
	return mUdf(float);

    for ( int idx=0; idx<inps_.size(); idx++ )
    {
	const Property* p = inps_[idx];
	if ( !p ) return mUdf(float);
	float v = eo.curz_;
	if ( p == &xposprop )
	    v = eo.relpos_;
	else if ( p != &depthprop )
	{
	    v = p->value( eo );
	    const UnitOfMeasure* uom = inpunits_[idx];
	    if ( uom )
		v = uom->getUserValueFromSI( v );
	}
	else if ( SI().depthsInFeetByDefault() )
	    v *= mToFeetFactorF;
	expr_->setVariableValue( getMathVarIdx(*expr_,idx,true), v );
    }

    for ( int idx=0; idx<consts_.size(); idx++ )
	expr_->setVariableValue( getMathVarIdx(*expr_,idx,false), 
	      			 constValue(idx) );

    float res = expr_->getValue();
    if ( uom_ )
	res = uom_->getUserValueFromSI( res );

    if ( eo.valopt_ == EvalOpts::New )
	eo.valopt_ = EvalOpts::Prev;

    return res;
}


PropertySet::PropertySet( const PropertyRefSelection& prs )
{
    for ( int idx=0; idx<prs.size(); idx++ )
	props_ += new ValueProperty( *prs[idx] );
}


PropertySet& PropertySet::operator =( const PropertySet& ps )
{
    if ( this != &ps )
    {
	erase();
	for ( int idx=0; idx<ps.size(); idx++ )
	    props_ += ps.get(idx).clone();
    }
    return *this;
}


void PropertySet::replace( int idx, Property* p )
{
    if ( p )
	delete props_.replace( idx, p );
}

int PropertySet::indexOf( const char* nm, bool matchaliases ) const
{
    if ( !nm || !*nm ) return -1;

    for ( int idx=0; idx<props_.size(); idx++ )
    {
	const Property& p = *props_[idx];
	if ( p.ref().name() == nm )
	    return idx;
    }
    if ( matchaliases )
    {
	for ( int idx=0; idx<props_.size(); idx++ )
	{
	    const Property& p = *props_[idx];
	    if ( p.ref().isKnownAs(nm) )
		return idx;
	}
    }

    return -1;
}


Property* PropertySet::fnd( const char* nm, bool ma ) const
{
    const int idx = indexOf(nm,ma);
    return idx < 0 ? 0 : const_cast<Property*>( props_[idx] );
}


int PropertySet::indexOf( PropertyRef::StdType st, int occ ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const Property& pr = *props_[idx];
	if ( pr.ref().hasType(st) )
	{
	    occ--;
	    if ( occ < 0 )
		return idx;
	}
    }

    return -1;
}


bool PropertySet::add( Property* p )
{
    if ( !p ) return false;
    if ( indexOf(p->name(),false) >= 0 )
	return false;
    props_ += p;
    return true;
}


int PropertySet::set( Property* p )
{
    if ( !p ) return -1;

    int idxof = indexOf( p->name(), false );
    if ( idxof >= 0 )
    	delete props_.replace( idxof, p );
    else
    {
	idxof = props_.size();
	props_ += p;
    }
    return idxof;
}


void PropertySet::remove( int idx )
{
    delete props_.remove( idx );
}


bool PropertySet::prepareUsage() const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( !props_[idx]->init(*this) )
	    { errmsg_ = props_[idx]->errMsg(); return false; }
    }
    return true;
}
