/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/

static const char* rcsID = "$Id: rockphysics.cc,v 1.3 2012-02-03 13:02:03 cvsbert Exp $";

#include "rockphysics.h"
#include "mathproperty.h"
#include "ascstream.h"
#include "safefileio.h"
#include "separstr.h"
#include "keystrs.h"
#include "iopar.h"
#include "ioman.h"

static const char* filenamebase = "RockPhysics";
static const char* sKeyDef = "Formula";
static const char* sKeyVar = "Variable";
static const char* sKeyConst = "Constant";
static const char* sKeyRg = "Typical Range";


RockPhysics::Formula* RockPhysics::Formula::get( const IOPar& iop )
{
    Formula* fm = new Formula( PropertyRef::Other );
    if ( !fm->usePar(iop) )
	{ delete fm; return 0; }
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
	vardefs_ = fm.vardefs_;
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
    type_ = PropertyRef::parseEnumStdType( iop.getValue(0) );
    iop.get( sKeyDef, def_ );
    iop.get( sKey::Desc, desc_ );
    desc_ = getStrFromFMS( desc_ );

    vardefs_.erase();
    for ( int idx=0; ; idx++ )
    {
	const char* res = iop.find( IOPar::compKey(sKeyVar,idx) );
	if ( !res || !*res ) break;
	vardefs_.add( getStrFromFMS(res) );
    }

    deepErase( constdefs_ );
    for ( int idx=0; ; idx++ )
    {
	IOPar* subpar = iop.subselect( IOPar::compKey(sKeyConst,idx) );
	if ( !subpar || subpar->isEmpty() )
	    { delete subpar; break; }
	nm = subpar->find( sKey::Name );
	if ( !nm.isEmpty() )
	{
	    ConstDef* cd = new ConstDef( nm );
	    subpar->get( sKey::Desc, cd->desc_ );
	    cd->desc_ = getStrFromFMS( cd->desc_ );
	    subpar->get( sKeyRg, cd->typicalrg_ );
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
    setIOPWithNLs( iop, sKey::Desc, desc_ );
    for ( int idx=0; idx<vardefs_.size(); idx++ )
	setIOPWithNLs( iop, IOPar::compKey(sKeyVar,idx),
			vardefs_.get(idx) );
    for ( int idx=0; idx<constdefs_.size(); idx++ )
    {
	const BufferString keybase( IOPar::compKey(sKeyConst,idx) );
	iop.set( IOPar::compKey(keybase,sKey::Name), constdefs_[idx]->name() );
	iop.set( IOPar::compKey(keybase,sKeyRg), constdefs_[idx]->typicalrg_ );
	iop.set( IOPar::compKey(keybase,sKey::Desc), constdefs_[idx]->desc_ );
	setIOPWithNLs( iop, IOPar::compKey(keybase,sKey::Desc),
			constdefs_[idx]->desc_ );
    }
}


bool RockPhysics::Formula::setDef( const char* str )
{
    vardefs_.erase(); deepErase( constdefs_ );
    def_ = str;
    MathProperty* mp = getProperty();
    if ( !mp ) return false;

    const int nrconsts = mp->nrConsts();
    delete mp;
    for ( int idx=0; idx<nrconsts; idx++ )
	constdefs_ += new ConstDef( BufferString("c",idx) );
    const int nrvars = mp->nrInputs();
    for ( int idx=0; idx<nrvars; idx++ )
	vardefs_.add( mp->inputName(idx) );

    delete mp;
    return true;
}



MathProperty* RockPhysics::Formula::getProperty( const PropertyRef* pr ) const
{
    if ( !pr )
    {
	const int idxfirst = PROPS().indexOf( type_ );
	if ( idxfirst < 0 )
	    return 0;
	pr = PROPS()[idxfirst];
    }
    return new MathProperty( *pr, def_ );
}


class RockPhysicsFormulaMgr : public CallBacker
{
public:

RockPhysicsFormulaMgr()
    : fms_(0)
{
    IOM().surveyChanged.notify( mCB(this,RockPhysicsFormulaMgr,doNull) );
}

~RockPhysicsFormulaMgr()
{
    delete fms_;
}

void doNull( CallBacker* )
{
    delete fms_; fms_ = 0;
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
	    { fms_ = tmp; break; }
    }

    if ( !fms_ )
	fms_ = new RockPhysics::FormulaSet;
}

    RockPhysics::FormulaSet*	fms_;

};

const RockPhysics::FormulaSet& ROCKPHYSFORMS()
{
    static RockPhysicsFormulaMgr rfm;
    if ( !rfm.fms_ )
	rfm.createSet();
    return *rfm.fms_;
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


void RockPhysics::FormulaSet::getRelevant( PropertyRef::StdType tp,
					   BufferStringSet& nms ) const
{
    nms.erase();
    for ( int idx=0; idx<size(); idx++ )
    {
	const Formula& fm = *(*this)[idx];
	if ( fm.hasOutputType(tp) )
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
    return astrm.stream().good();
}
