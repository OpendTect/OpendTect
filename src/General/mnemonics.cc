/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Khushnood Qadir
 * DATE     : Aug 2020
-*/



#include "mnemonics.h"
#include "ascstream.h"
#include "ioman.h"
#include "globexpr.h"
#include "mathproperty.h"
#include "safefileio.h"
#include "separstr.h"
#include "unitofmeasure.h"

static const char* filenamebase = "Mnemonics";


mDefineEnumUtils(Mnemonic,Scale,"Plot Scale")
{
    "Linear",
    "Logarithmic",
    0
};


Mnemonic::~Mnemonic()
{
    delete mathdef_;
}


Mnemonic& Mnemonic::operator =( const Mnemonic& mnc )
{
    if ( this != &mnc )
    {
	setName( mnc.name() );
	this->logtypename_ = mnc.logtypename_;
	this->aliases_ = mnc.aliases();
	this->stdtype_ =mnc.stdType();
	this->disp_ = mnc.disp_;
    }
    return *this;
}


bool Mnemonic::operator ==( const Mnemonic& mnc ) const
{
    return name() == mnc.name();
}


bool Mnemonic::operator !=( const Mnemonic& mnc ) const
{
    return name() != mnc.name();
}


void Mnemonic::setFixedDef( const MathProperty* mp )
{
    delete mathdef_;
    mathdef_ = mp ? mp->clone() : nullptr;
}


bool Mnemonic::isKnownAs( const char* nm ) const
{
    if ( !nm || !*nm )
	return false;

    if ( caseInsensitiveEqual(nm,name().buf(),0) )
	return true;

    for ( int idx=0; idx<aliases_.size(); idx++ )
    {
	const GlobExpr ge( aliases_.get(idx), CaseInsensitive );
	if ( ge.matches(nm) )
	    return true;
    }
    return false;
}


void Mnemonic::usePar( const IOPar& iop )
{
    aliases_.erase();
    FileMultiString fms( iop.find(name()) );
    int sz = fms.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( idx == 0 )
	    logtypename_ = fms[idx];
	else if ( idx == 1 )
	    stdtype_ = PropertyRef::StdTypeDef().parse( fms[idx] );
	else if ( idx >=2 && idx <=7 )
	{
	    disp_.scale_ = Mnemonic::ScaleDef().parse( fms[2] );
	    disp_.range_.start = fms.getFValue( 3 );
	    disp_.range_.stop = fms.getFValue( 4 );
	    disp_.typicalrange_.start = fms.getFValue( 5 );
	    disp_.typicalrange_.stop = fms.getFValue( 6 );
	    if ( idx == 7 )
		disp_.unit_ = fms[7];
	}
	else if ( idx >= 8 && idx <= 10 )
	    disp_.color_.set( fms.getFValue(8), fms.getFValue(9),
						fms.getFValue(10) );
	else
	    aliases_.add( fms[idx] );
    }
}


void Mnemonic::fillPar( IOPar& iop ) const
{
    FileMultiString fms( logtypename_ );
    fms += PropertyRef::StdTypeDef().toString( stdType() );
    fms += Mnemonic::ScaleDef().toString( disp_.scale_ );
    Interval<float> vrange( disp_.range_ );
    Interval<float> vtypicalrange( disp_.typicalrange_ );

    fms.add( vrange.start );
    fms.add( vrange.stop );
    fms.add( vtypicalrange.start );
    fms.add( vtypicalrange.stop );
    if ( !disp_.unit_.isEmpty() )
	fms += disp_.unit_;
    else
	fms += "";

    fms += disp_.color_.r();
    fms += disp_.color_.g();
    fms += disp_.color_.b();
    if ( !aliases_.isEmpty() )
    {
	for ( int idx=0; idx<aliases_.size(); idx++ )
	    fms += aliases_.get(idx);
    }

    iop.set( name(), fms );
}


class MnemonicSetMgr : public CallBacker
{
public:

MnemonicSetMgr()
    : mns_( nullptr )
{
    mAttachCB( IOM().surveyChanged, MnemonicSetMgr::doNull );
}


~MnemonicSetMgr()
{
    detachAllNotifiers();
    delete mns_;
}


void doNull( CallBacker* )
{
    delete mns_;
    mns_ = nullptr;
}


void createSet()
{
    Repos::FileProvider rfp( filenamebase, true );
    rfp.setSource(Repos::Source::ApplSetup);
    while ( rfp.next() )
    {
	const BufferString fnm( rfp.fileName() );
	SafeFileIO sfio( fnm );
	if ( !sfio.open(true) )
	    continue;

	ascistream astrm( sfio.istrm(), true );
	MnemonicSet* oldmns_ = mns_;
	mns_ = new MnemonicSet;
	mns_->readFrom( astrm );
	if ( mns_->isEmpty() )
	{
	    delete mns_;
	    mns_ = oldmns_;
	}
	else
	{
	    delete oldmns_;
	    sfio.closeSuccess();
	    break;
	}
    }

    if ( !mns_ )
	mns_ = new MnemonicSet;
}

    MnemonicSet* mns_;

};


const MnemonicSet& MNC()
{
    mDefineStaticLocalObject( MnemonicSetMgr, msm, );
    if ( !msm.mns_)
	msm.createSet();

    return *msm.mns_;
}


/* ----MnemonicSet---- */

MnemonicSet::~MnemonicSet()
{
    deepErase( *this );
}


MnemonicSet& MnemonicSet::operator =( const MnemonicSet& mnc )
{
    if ( this != &mnc )
    {
	deepErase( *this );
	for ( int idx=0; idx<mnc.size(); idx++ )
	    *this += new Mnemonic( *mnc[idx] );
    }

    return *this;
}


int MnemonicSet::indexOf( const char* nm ) const
{
    if ( nm && *nm )
    {
	for ( int idx=0; idx<size(); idx++ )
	{
	    const Mnemonic& mnc = *(*this)[idx];
	    if ( mnc.name() == nm )
		return idx;
	}
	for ( int idx=0; idx<size(); idx++ )
	{
	    const Mnemonic& mnc = *(*this)[idx];
	    if ( mnc.isKnownAs(nm) )
		return idx;
	}
    }

    return -1;
}


MnemonicSet* MnemonicSet::getSet( const PropertyRef* pr )
{
    MnemonicSet* mns = new MnemonicSet();
    for ( int idx=0; idx<size(); idx++ )
    {
	Mnemonic& mnc = *(*this)[idx];
	if ( mnc.hasType(pr->stdType()) )
	    mns->add( &mnc );
    }

    return mns;
}


Mnemonic* MnemonicSet::getGuessed( PropertyRef::StdType stdtype )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	Mnemonic& mnc = *(*this)[idx];
	if ( mnc.hasType(stdtype) )
	    return &mnc;
    }

    return find("OTH");
}


Mnemonic* MnemonicSet::getGuessed( const UnitOfMeasure* uom )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	Mnemonic& mnc = *(*this)[idx];
	if ( uom )
	{
	    if ( mnc.hasType(uom->propType()) )
	       return &mnc;
	}
    }

    return find("OTH");
}


const Mnemonic* MnemonicSet::getGuessed( PropertyRef::StdType stdtype ) const
{
    return const_cast<MnemonicSet*>(this)->getGuessed( stdtype );
}


const Mnemonic* MnemonicSet::getGuessed( const UnitOfMeasure* uom ) const
{
    return const_cast<MnemonicSet*>(this)->getGuessed( uom );
}


void MnemonicSet::getNames( BufferStringSet& names ) const
{
    for ( int idx=0; idx<size(); idx++ )
	names.add( (*this)[idx]->name() );

    names.sort();
}


int MnemonicSet::add( Mnemonic* mn )
{
    if ( !mn )
	return -1;

    if ( !isPresent(mn->name()) )
    {
	ObjectSet<Mnemonic>::doAdd( mn );
	return size()-1;
    }

    return -1;
}


Mnemonic* MnemonicSet::fnd( const char* nm ) const
{
    const int idx = indexOf( nm );
    return idx < 0 ? nullptr : const_cast<Mnemonic*>( (*this)[idx] );
}


bool MnemonicSet::save() const
{
    Repos::FileProvider rfp( filenamebase );
    rfp.setSource(Repos::Rel);
    BufferString fnm = rfp.fileName();

    SafeFileIO sfio( fnm, true );
    if ( !sfio.open(false) )
    {
	BufferString msg( "Cannot write to " ); msg += fnm;
	ErrMsg( sfio.errMsg() );
	return false;
    }

    ascostream astrm( sfio.ostrm() );
    if ( !writeTo(astrm) )
    {
	sfio.closeFail();
	return false;
    }

    return sfio.closeSuccess();
}


void MnemonicSet::readFrom( ascistream& astrm )
{
    while ( !atEndOfSection(astrm.next()) )
    {
	IOPar iop; iop.getFrom(astrm);
	for ( int idx=0; idx<iop.size(); idx++ )
	{
	    const BufferString mnemonicnm( iop.getKey(idx) );
	    if ( find(mnemonicnm) )
		continue;

	    Mnemonic* mnc = new Mnemonic( mnemonicnm );
	    mnc->usePar( iop );

	    if ( add(mnc) < 0 )
		delete mnc;
	}
    }
}


bool MnemonicSet::writeTo( ascostream& astrm ) const
{
    astrm.putHeader( "Mnemonic" );
    IOPar iop;
    for ( int idx=0; idx<size(); idx++ )
    {
	const Mnemonic& mnc = *(*this)[idx];
	iop.setKey( idx, mnc.name() );
	mnc.fillPar( iop );
    }
    iop.putTo( astrm );
    return astrm.isOK();
}


bool MnemonicSelection::operator ==( const MnemonicSelection& oth ) const
{
    if ( size() != oth.size() )
	return false;

    for ( int idx=0; idx<size(); idx++ )
	if ( (*this)[idx] != oth[idx] )
	    return false;

    return true;
}


int MnemonicSelection::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const Mnemonic& mn = *((*this)[idx]);
	if ( mn.name() == nm )
	    return idx;
    }
    return -1;
}


int MnemonicSelection::find( const char* nm ) const
{
    const int idxof = indexOf( nm );
    if ( idxof >= 0 )
	return idxof;

    for ( int idx=0; idx<size(); idx++ )
    {
	const Mnemonic& mn = *((*this)[idx]);
	if ( mn.isKnownAs( nm ) )
	    return idx;
    }

    return -1;
}


MnemonicSelection MnemonicSelection::getAll( const Mnemonic* exclude )
{
    MnemonicSelection ret;
    const MnemonicSet& mns = MNC();
    for ( int idx=0; idx<mns.size(); idx++ )
    {
	const Mnemonic* mn = mns[idx];
	if ( mn != exclude )
	    ret += mn;
    }

    return ret;
}
