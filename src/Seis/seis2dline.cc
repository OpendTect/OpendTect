/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : June 2004
-*/

static const char* rcsID = "$Id: seis2dline.cc,v 1.23 2004-10-01 15:29:47 bert Exp $";

#include "seis2dline.h"
#include "seistrctr.h"
#include "seistrcsel.h"
#include "seisbuf.h"
#include "survinfo.h"
#include "strmprov.h"
#include "ascstream.h"
#include "bufstringset.h"
#include "cubesampling.h"
#include "survinfo.h"
#include "filegen.h"
#include "keystrs.h"
#include "iopar.h"
#include "ioobj.h"
#include "hostdata.h"
#include "timefun.h"
#include "errh.h"

const char* Seis2DLineSet::sKeyAttrib = "Attribute";
const char* Seis2DLineSet::sKeyDefAttrib = "Seis";


ObjectSet<Seis2DLineIOProvider>& S2DLIOPs()
{
    static ObjectSet<Seis2DLineIOProvider>* theinst = 0;
    if ( !theinst ) theinst = new ObjectSet<Seis2DLineIOProvider>;
    return *theinst;
}


bool TwoDSeisTrcTranslator::implRemove( const IOObj* ioobj ) const
{
    if ( !ioobj ) return true;
    BufferString fnm( ioobj->fullUserExpr(true) );
    Seis2DLineSet lg( fnm );
    const int nrlines = lg.nrLines();
    for ( int iln=0; iln<nrlines; iln++ )
	lg.remove( 0 );
    File_remove( fnm, NO );
    return true;
}


bool TwoDSeisTrcTranslator::initRead_()
{
    errmsg = 0;
    if ( !conn->ioobj )
	{ errmsg = "Cannot reconstruct 2D filename"; return false; }
    BufferString fnm( conn->ioobj->fullUserExpr(true) );
    if ( !File_exists(fnm) ) return false;

    Seis2DLineSet lset( fnm );
    lset.getTxtInfo( 0, pinfo->usrinfo, pinfo->stdinfo );

    const char* linekey = seldata ? seldata->linekey_.buf() : 0;
    int linenr = linekey && *linekey ? lset.indexOf( linekey ) : -1;
    CubeSampling cs( false );
    errmsg = lset.getCubeSampling( cs, linenr );
    if ( errmsg && *errmsg )
	return false;

    insd.start = cs.zrg.start; insd.step = cs.zrg.step;
    innrsamples = (int)((cs.zrg.stop-cs.zrg.start) / cs.zrg.step + 1.5);
    pinfo->inlrg.start = cs.hrg.start.inl; pinfo->inlrg.stop = cs.hrg.stop.inl;
    pinfo->inlrg.step = cs.hrg.step.inl; pinfo->crlrg.step = cs.hrg.step.crl;
    pinfo->crlrg.start = cs.hrg.start.crl; pinfo->crlrg.stop = cs.hrg.stop.crl;
    addComp( DataCharacteristics(), pinfo->stdinfo, Seis::UnknowData );
    return true;
}


//------


Seis2DLineSet::~Seis2DLineSet()
{
    deepErase( pars_ );
}


Seis2DLineSet& Seis2DLineSet::operator =( const Seis2DLineSet& lset )
{
    if ( &lset == this ) return *this;
    fname_ = lset.fname_;
    readFile();
    return *this;
}


void Seis2DLineSet::init( const char* fnm )
{
    fname_ = fnm;
    BufferString type = "CBVS";
    readFile( false, &type );

    liop_ = 0;
    const ObjectSet<Seis2DLineIOProvider>& liops = S2DLIOPs();
    for ( int idx=0; idx<liops.size(); idx++ )
    {
	Seis2DLineIOProvider* liop = liops[idx];
	if ( type == liop->type() )
	    { liop_ = liop; break; }
    }
}


const char* Seis2DLineSet::type() const
{
    return liop_ ? liop_->type() : "CBVS";
}


const char* Seis2DLineSet::lineName( const IOPar& iop )
{
    return iop.name().buf();
}


const char* Seis2DLineSet::lineName( int idx ) const
{
    return idx >= 0 && idx < pars_.size()
	 ? lineName( *pars_[idx] ) : "";
}


const char* Seis2DLineSet::attribute( const IOPar& iop )
{
    const char* res = iop.find( sKeyAttrib );
    return res ? res : sKeyDefAttrib;
}


const char* Seis2DLineSet::attribute( int idx ) const
{
    return idx >= 0 && idx < pars_.size()
	 ? attribute( *pars_[idx] ) : sKeyDefAttrib;
}


BufferString Seis2DLineSet::lineKey( const IOPar& iop )
{
    return lineKey( lineName(iop), attribute(iop) );
}


void Seis2DLineSet::setLineKey( IOPar& iop, const char* lk )
{
    iop.setName( lineNamefromKey(lk) );
    iop.set( sKeyAttrib, attrNamefromKey(lk) );
}


BufferString Seis2DLineSet::lineKey( const char* lnm, const char* attrnm )
{
    BufferString ret( lnm );
    if ( attrnm && *attrnm && strcmp(attrnm,sKeyDefAttrib) )
	{ ret += "|"; ret += attrnm; }
    return ret;
}


BufferString Seis2DLineSet::lineNamefromKey( const char* key )
{
    BufferString ret( key );
    char* ptr = strchr( ret.buf(), '|' );
    if ( ptr ) *ptr = '\0';
    return ret;
}


BufferString Seis2DLineSet::attrNamefromKey( const char* key )
{
    BufferString ret;
    char* ptr = key ? strchr( key, '|' ) : 0;
    if ( ptr ) ret = ptr + 1;
    return ret;
}


int Seis2DLineSet::indexOf( const char* key ) const
{
    for ( int idx=0; idx<pars_.size(); idx++ )
    {
	if ( lineKey(*pars_[idx]) == key )
	    return idx;
    }
    return -1;
}


#define mMkLockNm(lockfnm) \
    const BufferString lockfnm( fname_ ); \
    const_cast<BufferString&>(lockfnm) += "_lock"

bool Seis2DLineSet::waitForLock( bool mknew, bool failiflocked ) const
{
    mMkLockNm(lockfnm);

    // Wait for max 5 seconds.
    for ( int idx=0; idx<50; idx++ )
    {
	if ( !File_exists(lockfnm.buf()) )
	    break;
	else if ( failiflocked )
	    return false;

	Time_sleep( 0.1 );
    }

    if ( mknew )
    {
	StreamData sd = StreamProvider(lockfnm).makeOStream();
	if ( sd.usable() )
	{
	    *sd.ostrm << HostData::localHostName() << ":" << getPID();
	    const char* ptr = GetSoftwareUser();
	    if ( ptr && *ptr )
		*sd.ostrm << ' ' << ptr;
	}
	sd.close();
    }
    return true;
}


static const char* sKeyFileType = "2D Line Group Data";

void Seis2DLineSet::readFile( bool mklock, BufferString* type )
{
    deepErase( pars_ );

    waitForLock( mklock, false );
    StreamData sd = StreamProvider( fname_ ).makeIStream();
    if ( !sd.usable() ) return;

    ascistream astrm( *sd.istrm, true );
    if ( !astrm.isOfFileType( sKeyFileType ) )
	{ sd.close(); return; }

    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(sKey::Name) )
	    setName( astrm.value() );
	if ( astrm.hasKeyword(sKey::Type) && type )
	    *type = astrm.value();
    }

    while ( astrm.type() != ascistream::EndOfFile )
    {
	IOPar* newpar = new IOPar;
	while ( !atEndOfSection(astrm.next()) )
	{
	    if ( astrm.hasKeyword(sKey::Name) )
		newpar->setName( astrm.value() );
	    else if ( !astrm.hasValue("") )
		newpar->set( astrm.keyWord(), astrm.value() );
	}
	if ( newpar->size() < 1 )
	    delete newpar;
	else
	    pars_ += newpar;
    }

    sd.close();
}


void Seis2DLineSet::writeFile() const
{
    BufferString wrfnm( fname_ ); wrfnm += "_new";

    StreamData sd = StreamProvider( wrfnm ).makeOStream();
    if ( !sd.usable() ) return;

    ascostream astrm( *sd.ostrm );
    if ( !astrm.putHeader( sKeyFileType ) )
	{ sd.close(); return; }

    astrm.put( sKey::Name, name() );
    astrm.put( sKey::Type, type() );
    astrm.put( "Number of lines", pars_.size() );
    astrm.newParagraph();

    for ( int ipar=0; ipar<pars_.size(); ipar++ )
    {
	const IOPar& iopar = *pars_[ipar];
	astrm.put( sKey::Name, iopar.name() );
	for ( int idx=0; idx<iopar.size(); idx++ )
	{
	    const char* val = iopar.getValue(idx);
	    if ( !val || !*val ) continue;
	    astrm.put( iopar.getKey(idx), iopar.getValue(idx) );
	}
	astrm.newParagraph();
    }

    sd.close();

    if ( File_exists(fname_) )
	File_remove( fname_, 0 );
    File_rename( wrfnm, fname_ );

    removeLock();
}


void Seis2DLineSet::removeLock() const
{
    mMkLockNm(lockfnm);
    File_remove( lockfnm.buf(), 0 );
}


Executor* Seis2DLineSet::lineFetcher( int ipar, SeisTrcBuf& tbuf,
					const SeisSelData* sd) const
{
    if ( !liop_ )
    {
	ErrMsg("No suitable 2D line extraction object found");
	return 0;
    }
    else if ( ipar >= pars_.size() )
    {
	ErrMsg("Line number requested out of bounds");
	return 0;
    }

    return liop_->getFetcher( *pars_[ipar], tbuf, sd );
}


Seis2DLinePutter* Seis2DLineSet::linePutter( IOPar* newiop )
{
    if ( !newiop || !newiop->size() )
    {
	ErrMsg("No data for line add provided");
	return 0;
    }
    else if ( !liop_ )
    {
	ErrMsg("No suitable 2D line creation object found");
	return 0;
    }

    const BufferString newlinekey = lineKey( *newiop );

    // Critical concurrency section using file lock
    readFile( true );

    Seis2DLinePutter* res = 0;
    int paridx = indexOf( newlinekey );
    if ( paridx >= 0 )
    {
	pars_[paridx]->merge( *newiop );
	*newiop = *pars_[paridx];
	delete pars_.replace( newiop, paridx );
	res = liop_->getReplacer( *pars_[paridx] );
    }
    else
    {
	const IOPar* previop = pars_.size() ? pars_[pars_.size()-1] : 0;
	pars_ += newiop;
	res = liop_->getAdder( *newiop, previop, name() );
    }

    if ( res )
	writeFile();
    else
	removeLock();

    // Phew! Made it.
    return res;
}


bool Seis2DLineSet::isEmpty( int ipar ) const
{
    return liop_ ? liop_->isEmpty( *pars_[ipar] ) : true;
}


bool Seis2DLineSet::remove( int ipar )
{
    if ( ipar > pars_.size() )
	return false;
    if ( !waitForLock(true,true) )
	return false;

    IOPar* iop = pars_[ipar];
    if ( liop_ )
	liop_->removeImpl(*iop);

    pars_ -= iop;
    delete iop;
    writeFile();
    return true;
}


bool Seis2DLineSet::getTxtInfo( int ipar, BufferString& uinf,
				  BufferString& stdinf ) const
{
    return liop_ ? liop_->getTxtInfo(*pars_[ipar],uinf,stdinf) : false;
}


bool Seis2DLineSet::getRanges( int ipar, StepInterval<int>& sii,
				 StepInterval<float>& sif ) const
{
    return liop_ ? liop_->getRanges(*pars_[ipar],sii,sif) : false;
}


void Seis2DLineSet::getAvailableAttributes( BufferStringSet& nms ) const
{
    nms.erase();
    const int sz = nrLines();
    for ( int idx=0; idx<sz; idx++ )
	nms.addIfNew( attribute(idx) );
}


const char* Seis2DLineSet::getCubeSampling( CubeSampling& cs, int lnr ) const
{
    cs.hrg.step.inl = cs.hrg.step.crl = 1;
    cs.hrg.start.inl = 0; cs.hrg.stop.inl = nrLines()-1;
    cs.hrg.start.crl = 0; cs.hrg.stop.crl = mUndefIntVal;
    cs.zrg = SI().zRange();
    const int nrlines = nrLines();
    if ( nrlines < 1 )
	return "No lines in Line Set";

    bool havelinesel = lnr >= 0;
    if ( !havelinesel )
	lnr = 0;
    else
	cs.hrg.start.inl = cs.hrg.stop.inl = lnr;

    StepInterval<int> trg; StepInterval<float> zrg;
    bool foundone = false;

    if ( getRanges(lnr,trg,zrg) )
	foundone = true;

    if ( !havelinesel )
    {
	StepInterval<int> newtrg; StepInterval<float> newzrg;
	for ( int iln=1; iln<nrlines; iln++ )
	{
	    if ( getRanges(iln,newtrg,newzrg) )
	    {
		foundone = true;
		if ( newtrg.start < trg.start ) trg.start = newtrg.start;
		if ( newtrg.stop > trg.stop ) trg.stop = newtrg.stop;
		if ( newtrg.step < trg.step ) trg.step = newtrg.step;
		if ( newzrg.start < zrg.start ) zrg.start = newzrg.start;
		if ( newzrg.stop > zrg.stop ) zrg.stop = newzrg.stop;
		if ( newzrg.step < zrg.step ) zrg.step = newzrg.step;
	    }
	}
    }

    if ( !foundone )
	return "No range info present";

    cs.hrg.start.crl = trg.start; cs.hrg.stop.crl = trg.stop;
    cs.hrg.step.crl = trg.step;
    cs.zrg = zrg;
    return 0;
}
