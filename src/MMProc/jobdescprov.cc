/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "jobdescprov.h"

#include "trckeyzsampling.h"
#include "iopar.h"
#include "keystrs.h"
#include "settings.h"
#include "survinfo.h"
#include "survgeom.h"
#include "undefval.h"
#include "od_ostream.h"

const char* InlineSplitJobDescProv::sKeyMaxInlRg()
    { return "Maximum Inline Range"; }
const char* InlineSplitJobDescProv::sKeyMaxCrlRg()
    { return "Maximum Crossline Range"; }

//Network::JobDescProv

JobDescProv::JobDescProv( const IOPar& iop )
	: inpiopar_(*new IOPar(iop))
{
}


JobDescProv::~JobDescProv()
{
    delete &inpiopar_;
}


// Network::KeyReplaceJobDescProv

KeyReplaceJobDescProv::KeyReplaceJobDescProv( const IOPar& iop,
					      const char* key, int nrjobs )
    : JobDescProv(iop)
    , key_(key)
    , nrjobs_(nrjobs)
{
}


KeyReplaceJobDescProv::~KeyReplaceJobDescProv()
{
}


void KeyReplaceJobDescProv::getJob( int jidx, IOPar& iop ) const
{
    iop = inpiopar_;
    iop.set( key_, objName(jidx) );
}


const char* KeyReplaceJobDescProv::objName( int jidx ) const
{
    objnm_ = gtObjName( jidx );
    return objnm_.buf();
}


void KeyReplaceJobDescProv::dump( od_ostream& strm ) const
{
    strm << "\nKey-replace JobDescProv dump.\n"
	    "The following jobs description keys are available:\n";
    for ( int idx=0; idx<nrjobs_; idx++ )
	strm << objName( idx ) << ' ';
    strm << od_endl;
}


// Network::StringKeyReplaceJobDescProv

StringKeyReplaceJobDescProv::StringKeyReplaceJobDescProv( const IOPar& iop,
			const char* key, const BufferStringSet& nms )
    : KeyReplaceJobDescProv(iop,key,nms.size())
{
}


StringKeyReplaceJobDescProv::~StringKeyReplaceJobDescProv()
{
}


const char* StringKeyReplaceJobDescProv::gtObjName( int jidx ) const
{
    return names_.validIdx(jidx) ? names_.get(jidx).buf() : "";
}


// Network::IDKeyReplaceJobDescProv

IDKeyReplaceJobDescProv::IDKeyReplaceJobDescProv( const IOPar& iop,
				const char* ky, const StepInterval<int>& rg )
    : KeyReplaceJobDescProv(iop,ky,rg.nrSteps()+1)
    , idrg_(rg)
{
}


IDKeyReplaceJobDescProv::~IDKeyReplaceJobDescProv()
{
}


const char* IDKeyReplaceJobDescProv::gtObjName( int jidx ) const
{
    return toString( idrg_.atIndex(jidx) );
}


void IDKeyReplaceJobDescProv::dump( od_ostream& strm ) const
{
    strm << "\nID Key-replace JobDescProv: "
	 << idrg_.start_ << '-' << idrg_.stop_ << " (step "
	 << idrg_.step_ << ", " << nrjobs_ << " jobs)." << od_endl;
}


#define mSetInlRgDef() \
    Interval<int> dum; SI().sampling(false).hsamp_.get( inlrg_, dum )


// Network::InlineSplitJobDescProv

InlineSplitJobDescProv::InlineSplitJobDescProv( const IOPar& iop )
    : JobDescProv(iop)
{
    mSetInlRgDef();
    getRange( inlrg_ );
    iop.get( "Nr of Inlines per Job", ninlperjob_ );
}


InlineSplitJobDescProv::InlineSplitJobDescProv( const IOPar& iop,
						const TypeSet<int>& in )
    : JobDescProv(iop)
    , inls_(new TypeSet<int>(in))
{
    mSetInlRgDef();
    iop.get( "Nr of Inlines per Job", ninlperjob_ );
}


InlineSplitJobDescProv::~InlineSplitJobDescProv()
{
    delete inls_;
}


const OD::String& getOutSubSelKey()
{
    mDefineStaticLocalObject( const BufferString, outsubselkey,
		    ( IOPar::compKey(sKey::Output(),sKey::Subsel()) ) );
    return outsubselkey;
}

#define mGetSubselKey(s) IOPar::compKey(getOutSubSelKey().buf(),sKey::s())

void InlineSplitJobDescProv::getRange( StepInterval<int>& rg ) const
{
    rg.step_ = 0;

    inpiopar_.get( mGetSubselKey(FirstInl), rg.start_ );
    inpiopar_.get( mGetSubselKey(LastInl), rg.stop_ );
    inpiopar_.get( mGetSubselKey(StepInl), rg.step_ );

    if ( rg.step_ < 0 ) rg.step_ = -rg.step_;
    if ( !rg.step_ ) rg.step_ = SI().inlStep();

    //if Subsel Type == None : init rg with SI()
    BufferString typestr;
    inpiopar_.get( mGetSubselKey(Type), typestr );
    if ( typestr==sKey::None() )
	rg = SI().inlRange(true);

    rg.sort();

    Interval<int> maxrg( Interval<int>().setFrom(rg) );
    inpiopar_.get( sKeyMaxInlRg(), maxrg );
    if ( !mIsUdf(maxrg.start_) && rg.start_ < maxrg.start_ )
	rg.start_ = maxrg.start_;
    if ( !mIsUdf(maxrg.stop_) && rg.stop_ > maxrg.stop_ )
	rg.stop_ = maxrg.stop_;
}


int InlineSplitJobDescProv::nrJobs() const
{
    const int nrinl = inls_ ? inls_->size() : inlrg_.nrSteps() + 1;
    int ret = nrinl / ninlperjob_;
    if ( nrinl % ninlperjob_ )
	ret += 1;

    return ret;
}


int InlineSplitJobDescProv::firstInlNr( int jidx ) const
{
    if ( inls_ )
    {
	const int startidx = jidx*ninlperjob_;
	return inls_->validIdx(startidx) ? (*inls_)[startidx] : -1;
    }
    else
	return inlrg_.start_ + jidx * inlrg_.step_ * ninlperjob_;
}


int InlineSplitJobDescProv::lastInlNr( int jidx ) const
{
    const int lastinl = firstInlNr(jidx) + inlrg_.step_ * (ninlperjob_ - 1);
    return lastinl > inlrg_.stop_ ? inlrg_.stop_ : lastinl;
}


void InlineSplitJobDescProv::getJob( int jidx, IOPar& iop ) const
{
    Interval<float> tmprg;
    const bool isfullrange = inpiopar_.get(mGetSubselKey(ZRange), tmprg);
    iop = inpiopar_;
    iop.set( mGetSubselKey(Type), sKey::Range() );
    iop.set( mGetSubselKey(FirstInl), firstInlNr(jidx) );
    iop.set( mGetSubselKey(LastInl), lastInlNr(jidx) );

    if ( !isfullrange )
    {
	iop.set( mGetSubselKey(FirstCrl), SI().crlRange(true).start_ );
	iop.set( mGetSubselKey(LastCrl), SI().crlRange(true).stop_ );
	iop.set( mGetSubselKey(StepCrl), SI().crlStep() );
	iop.set( mGetSubselKey(ZRange), SI().zRange(true) );
    }
}


const char* InlineSplitJobDescProv::objName( int jidx ) const
{
    const int firstinl = firstInlNr( jidx );
    const int lastinl = lastInlNr( jidx );
    objnm_ = ""; objnm_ += firstinl;
    if ( lastinl > firstinl )
	{ objnm_ += "-"; objnm_ += lastinl; }

    return objnm_.buf();
}


void InlineSplitJobDescProv::dump( od_ostream& strm ) const
{
    strm << "\nInline-split JobDescProv dump.\n";
    if ( !inls_ )
	strm << "Inline range: " << inlrg_.start_ << '-' << inlrg_.stop_
	     << " / " << inlrg_.step_;
    else
    {
	strm << "The following inlines are requested:\n";
	for ( int idx=0; idx<inls_->size(); idx++ )
	    strm << (*inls_)[idx] << ' ';
    }
    strm << od_endl;
}


#define mMMKey		"MultiMachine"
#define mNrInlPerJobKey	"Nr inline per job"

int InlineSplitJobDescProv::defaultNrInlPerJob()
{
    int nrinljob = 25;
    Settings::common().get( IOPar::compKey(mMMKey,mNrInlPerJobKey), nrinljob );
    return nrinljob;
}


void InlineSplitJobDescProv::setDefaultNrInlPerJob( int nr )
{
    if ( nr <= 0 ) return;

    Settings::common().set( IOPar::compKey(mMMKey,mNrInlPerJobKey), nr );
    Settings::common().write();
}


// Network::ParSubselJobDescProv

ParSubselJobDescProv::ParSubselJobDescProv( const IOPar& iop,
					    const char* subselkey )
    : JobDescProv(iop)
    , subselkey_(subselkey)
{
    int idx = 0;
    while ( true )
    {
	const BufferString curselkey = IOPar::compKey( subselkey_, idx++ );
	IOPar* curselpar = inpiopar_.subselect( curselkey );
	if ( !curselpar )
	    break;

	subselpars_ += curselpar;
    }
}


ParSubselJobDescProv::~ParSubselJobDescProv()
{
}


void ParSubselJobDescProv::getJob( int jidx, IOPar& iop ) const
{
    if ( !subselpars_.validIdx(jidx) )
	return;

    iop = inpiopar_;
    iop.removeSubSelection( subselkey_ );
    iop.mergeComp( *subselpars_[jidx], IOPar::compKey(subselkey_,0) );
}


void ParSubselJobDescProv::dump( od_ostream& strm ) const
{
    strm << "\nIOPar Subselection based JobDescProv dump.\n"
	    "The following jobs description keys are available:\n";
    for ( int idx=0; idx<nrJobs(); idx++ )
	strm << objName( idx ) << ' ';
    strm << od_endl;
}


// Network::ine2DSubselJobDescProv

Line2DSubselJobDescProv::Line2DSubselJobDescProv( const IOPar& iop )
    : ParSubselJobDescProv(iop,IOPar::compKey(getOutSubSelKey(),sKey::Line()))
{
}


Line2DSubselJobDescProv::~Line2DSubselJobDescProv()
{
}


const char* Line2DSubselJobDescProv::objName( int jidx ) const
{
    if ( !subselpars_.validIdx(jidx) )
	return nullptr;

    Pos::GeomID geomid;
    subselpars_[jidx]->get( sKey::GeomID(), geomid );
    return Survey::GM().getName( geomid );
}
