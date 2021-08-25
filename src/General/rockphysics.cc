/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/


#include "rockphysics.h"

#include "ascstream.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "mathproperty.h"
#include "safefileio.h"
#include "separstr.h"

static const char* filenamebase = "RockPhysics";
static const char* sKeyDef = "Formula";
static const char* sKeyVar = "Variable";
static const char* sKeyConst = "Constant";
static const char* sKeyRg = "Typical Range";


RockPhysics::Formula::~Formula()
{
    deepErase(constdefs_);
    deepErase(vardefs_);
}


RockPhysics::Formula* RockPhysics::Formula::get( const IOPar& iop )
{
    auto* fm = new Formula( Mnemonic::Other );
    if ( !fm->usePar(iop) )
	{ delete fm; return nullptr; }

    return fm;
}


RockPhysics::Formula& RockPhysics::Formula::operator =(
					const RockPhysics::Formula& fm )
{
    if ( this != &fm )
    {
	setName( fm.name() );
	type_ = fm.type_;
	def_ = fm.def_;
	desc_ = fm.desc_;
	src_ = fm.src_;
	unit_ = fm.unit_;
	deepCopy( vardefs_, fm.vardefs_ );
	deepCopy( constdefs_, fm.constdefs_ );
    }
    return *this;
}


static BufferString getStrFromFMS( const char* inp )
{
    SeparString fms( inp, '`' );
    fms.setSepChar( '\n' );
    return BufferString( fms );
}


bool RockPhysics::Formula::usePar( const IOPar& iop )
{
    if ( iop.isEmpty() ) return false;
    BufferString nm( iop.getKey(0) );
    if ( nm.isEmpty() ) return false;

    setName( nm );
    type_ = Mnemonic::parseEnumStdType( iop.getValue(0) );
    iop.get( sKeyDef, def_ );
    iop.get( sKey::Unit(), unit_ );
    iop.get( sKey::Desc(), desc_ );
    desc_ = getStrFromFMS( desc_ );

    deepErase( vardefs_ );
    for ( int idx=0; ; idx++ )
    {
	IOPar* subpar = iop.subselect( IOPar::compKey(sKeyVar,idx) );
	if ( !subpar || subpar->isEmpty() )
	    { delete subpar; break; }

	nm = subpar->find( sKey::Name() );
	if ( !nm.isEmpty() )
	{
	    const PropType typ =
		    Mnemonic::parseEnumStdType( subpar->find(sKey::Type()) );
	    auto* vd = new VarDef( nm, typ );
	    subpar->get( sKey::Unit(), vd->unit_ );
	    subpar->get( sKey::Desc(), vd->desc_ );
	    vd->desc_ = getStrFromFMS( vd->desc_ );

	    vardefs_ += vd;
	}
	delete subpar;
    }

    deepErase( constdefs_ );
    for ( int idx=0; ; idx++ )
    {
	IOPar* subpar = iop.subselect( IOPar::compKey(sKeyConst,idx) );
	if ( !subpar || subpar->isEmpty() )
	    { delete subpar; break; }
	nm = subpar->find( sKey::Name() );
	if ( !nm.isEmpty() )
	{
	    ConstDef* cd = new ConstDef( nm );
	    subpar->get( sKey::Desc(), cd->desc_ );
	    cd->desc_ = getStrFromFMS( cd->desc_ );
	    subpar->get( sKeyRg, cd->typicalrg_ );
	    subpar->get( sKey::Default(), cd->defaultval_ );
	    constdefs_ += cd;
	}
	delete subpar;
    }

    return true;
}


static void setIOPWithNLs( IOPar& iop, const char* ky, const char* val )
{
    SeparString fms( val, '\n' );
    fms.setSepChar( '`' );
    iop.set( ky, fms );
}


void RockPhysics::Formula::fillPar( IOPar& iop ) const
{
    iop.setEmpty();
    iop.set( name(), toString(type_) );
    iop.set( sKeyDef, def_ );
    setIOPWithNLs( iop, sKey::Desc(), desc_ );
    iop.set( sKey::Unit(), unit_ );
    for ( int idx=0; idx<vardefs_.size(); idx++ )
    {
	const VarDef& vd = *vardefs_[idx];
	const BufferString keybase( IOPar::compKey(sKeyVar,idx) );
	iop.set( IOPar::compKey(keybase,sKey::Type()), toString(vd.type_) );
	iop.set( IOPar::compKey(keybase,sKey::Name()), vd.name() );
	iop.set( IOPar::compKey(keybase,sKey::Unit()), vd.unit_ );
	setIOPWithNLs( iop, IOPar::compKey(keybase,sKey::Desc()), vd.desc_ );
    }
    for ( int idx=0; idx<constdefs_.size(); idx++ )
    {
	const ConstDef& cd = *constdefs_[idx];
	const BufferString keybase( IOPar::compKey(sKeyConst,idx) );
	iop.set( IOPar::compKey(keybase,sKey::Name()), cd.name() );
	iop.set( IOPar::compKey(keybase,sKeyRg), cd.typicalrg_ );
	iop.set( IOPar::compKey(keybase,sKey::Default()), cd.defaultval_ );
	setIOPWithNLs( iop, IOPar::compKey(keybase,sKey::Desc()), cd.desc_ );
    }
}


bool RockPhysics::Formula::setDef( const char* str )
{
    deepErase( vardefs_ ); deepErase( constdefs_ );
    def_ = str;
    MathProperty* mp = getProperty();
    if ( !mp ) return false;

    const int nrvars = mp->nrInputs();
    for ( int idx=0; idx<nrvars; idx++ )
    {
	const char* inpnm = mp->inputName( idx );
	if ( mp->isConst(idx) )
	    constdefs_ += new ConstDef( inpnm );
	else
	    vardefs_ += new VarDef( inpnm, mp->inputType(idx) );
    }

    delete mp;
    return true;
}


MathProperty* RockPhysics::Formula::getProperty( const PropertyRef* pr ) const
{
    if ( !pr )
    {
	const PropertyRefSelection prs( type_ );
	if ( prs.isEmpty() )
	    return nullptr;
	pr = prs.first();
    }

    return new MathProperty( *pr, def_ );
}


class RockPhysicsFormulaMgr : public CallBacker
{
public:

RockPhysicsFormulaMgr()
{
    mAttachCB( IOM().surveyToBeChanged, RockPhysicsFormulaMgr::doNull );
    mAttachCB( IOM().applicationClosing, RockPhysicsFormulaMgr::doNull );
}

~RockPhysicsFormulaMgr()
{
    detachAllNotifiers();
    delete fms_;
}

void doNull( CallBacker* )
{
    deleteAndZeroPtr( fms_ );
}

void createSet()
{
    Repos::FileProvider rfp( filenamebase, true );
    while ( rfp.next() )
    {
	const BufferString fnm( rfp.fileName() );
	SafeFileIO sfio( fnm );
	if ( !sfio.open(true) )
	    continue;

	ascistream astrm( sfio.istrm(), true );
	RockPhysics::FormulaSet* tmp = new RockPhysics::FormulaSet;
	tmp->readFrom( astrm );
	if ( tmp->isEmpty() )
	    delete tmp;
	else
	{
	    delete fms_;
	    fms_ = tmp;
	    sfio.closeSuccess();
	    break;
	}

	sfio.closeSuccess();
    }

    if ( !fms_ )
	fms_ = new RockPhysics::FormulaSet;
}

    RockPhysics::FormulaSet*	fms_ = 0;

};

const RockPhysics::FormulaSet& ROCKPHYSFORMS()
{
    mDefineStaticLocalObject( RockPhysicsFormulaMgr, rfm, );
    if ( !rfm.fms_ )
	rfm.createSet();
    return *rfm.fms_;
}


RockPhysics::FormulaSet::~FormulaSet()
{
    ObjectSet<const RockPhysics::Formula>& fms = *this;
    deepErase( fms );
}


int RockPhysics::FormulaSet::getIndexOf( const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const Formula& fm = *(*this)[idx];
	if ( fm.name() == nm )
	    return idx;
    }
    return -1;
}


void RockPhysics::FormulaSet::getRelevant( Formula::PropType tp,
					   BufferStringSet& nms ) const
{
    nms.erase();
    for ( int idx=0; idx<size(); idx++ )
    {
	const Formula& fm = *(*this)[idx];
	if ( fm.hasPropType(tp) )
	    nms.add( fm.name() );
    }
}


bool RockPhysics::FormulaSet::save( Repos::Source src ) const
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


void RockPhysics::FormulaSet::readFrom( ascistream& astrm )
{
    deepErase( *this );

    while ( !atEndOfSection(astrm.next()) )
    {
	IOPar iop; iop.getFrom(astrm);

	RockPhysics::Formula* fm = RockPhysics::Formula::get( iop );
	if ( fm )
	    *this += fm;
    }
}


bool RockPhysics::FormulaSet::writeTo( ascostream& astrm ) const
{
    astrm.putHeader( "Rock Physics" );
    for ( int idx=0; idx<size(); idx++ )
    {
	const RockPhysics::Formula& fm = *(*this)[idx];
	IOPar iop; fm.fillPar( iop );
	iop.putTo( astrm );
    }
    return astrm.isOK();
}
