/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : June 2004
-*/

static const char* rcsID = "$Id: seis2dline.cc,v 1.32 2004-10-21 12:36:59 bert Exp $";

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

#include <iostream>


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


bool TwoDSeisTrcTranslator::initRead_( bool )
{
    errmsg = 0;
    if ( !conn->ioobj )
	{ errmsg = "Cannot reconstruct 2D filename"; return false; }
    BufferString fnm( conn->ioobj->fullUserExpr(true) );
    if ( !File_exists(fnm) ) return false;

    Seis2DLineSet lset( fnm );
    if ( lset.nrLines() < 1 )
	{ errmsg = "Line set is empty"; return false; }
    lset.getTxtInfo( 0, pinfo->usrinfo, pinfo->stdinfo );
    addComp( DataCharacteristics(), pinfo->stdinfo, Seis::UnknowData );

    const char* linekey = seldata ? seldata->linekey_.buf() : 0;
    const bool isall = !linekey || !*linekey;
    int linenr = isall ? 0 : lset.indexOf( linekey );
    if ( linenr < 0 )
	{ errmsg = "Cannot find line key in line set"; return false; }

    CubeSampling cs( true );
    if ( isall )
    {
	cs.hrg.start.inl = cs.hrg.start.crl = 1;
	cs.hrg.stop.inl = cs.hrg.stop.crl = mUndefIntVal;
	cs.hrg.step.inl = cs.hrg.step.crl = 1;
    }
    else
    {
	errmsg = lset.getCubeSampling( cs, linenr );
	if ( errmsg && *errmsg )
	    return false;
    }

    insd.start = cs.zrg.start; insd.step = cs.zrg.step;
    innrsamples = (int)((cs.zrg.stop-cs.zrg.start) / cs.zrg.step + 1.5);
    pinfo->inlrg.start = cs.hrg.start.inl; pinfo->inlrg.stop = cs.hrg.stop.inl;
    pinfo->inlrg.step = cs.hrg.step.inl; pinfo->crlrg.step = cs.hrg.step.crl;
    pinfo->crlrg.start = cs.hrg.start.crl; pinfo->crlrg.stop = cs.hrg.stop.crl;
    return true;
}


//------

Line2DGeometry::Line2DGeometry()
{
    zrg = SI().sampling().zrg;
}


void Line2DGeometry::dump( std::ostream& strm, bool pretty ) const
{
    if ( !pretty )
	strm << zrg.start << '\t' << zrg.stop << '\t' << zrg.step << std::endl;
    else
    {
	const float fac = SI().zFactor();
	strm << "Z range " << SI().getZUnit() << ":\t" << fac*zrg.start
	     << '\t' << fac*zrg.stop << "\t" << fac*zrg.step;
	strm << "\n\nTrace number\tX-coord\tY-coord" << std::endl;
    }

    for ( int idx=0; idx<posns.size(); idx++ )
    {
	const Line2DPos& pos = posns[idx];
	strm << pos.nr << '\t' << pos.coord.x << '\t' << pos.coord.y << '\n';
    }
    strm.flush();
}


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


const char* Seis2DLineSet::lineName( int idx ) const
{
    return idx >= 0 && idx < pars_.size() ? pars_[idx]->name().buf() : "";
}


const char* Seis2DLineSet::attribute( int idx ) const
{
    const char* res = idx >= 0 && idx < pars_.size()
		    ? pars_[idx]->find(sKey::Attribute) : 0;
    return res ? res : LineKey::sKeyDefAttrib;
}


int Seis2DLineSet::indexOf( const char* key ) const
{
    for ( int idx=0; idx<pars_.size(); idx++ )
    {
	if ( LineKey(*pars_[idx],true) == key )
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


bool Seis2DLineSet::getGeometry( int ipar, Line2DGeometry& geom ) const
{
    if ( !liop_ )
    {
	ErrMsg("No suitable 2D line information object found");
	return 0;
    }

    if ( ipar < 0 || ipar >= pars_.size() )
    {
	ErrMsg("Line number requested not found in Line Set");
	return 0;
    }

    return liop_->getGeometry( *pars_[ipar], geom );
}


Executor* Seis2DLineSet::lineFetcher( int ipar, SeisTrcBuf& tbuf,
					const SeisSelData* sd ) const
{
    if ( !liop_ )
    {
	ErrMsg("No suitable 2D line extraction object found");
	return 0;
    }

    if ( ipar < 0 || ipar >= pars_.size() )
    {
	ErrMsg("Line number requested not found in Line Set");
	return 0;
    }

    return liop_->getFetcher( *pars_[ipar], tbuf, sd );
}


Seis2DLinePutter* Seis2DLineSet::linePutter( IOPar* newiop )
{
    if ( !newiop || newiop->name() == "" )
    {
	ErrMsg("No data for line add provided");
	return 0;
    }
    else if ( !liop_ )
    {
	ErrMsg("No suitable 2D line creation object found");
	return 0;
    }

    const BufferString newlinekey = LineKey( *newiop, true );

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


bool Seis2DLineSet::remove( const char* lk )
{
    if ( !waitForLock(false,true) )
	{ ErrMsg("Cannot remove from Lineset: LineSet locked"); return false; }

    // Critical concurrency section using file lock
    readFile( true );
    int ipar = indexOf( lk );
    if ( ipar < 0 )
	{ removeLock(); return true; }

    IOPar* iop = pars_[ipar];
    if ( liop_ )
	liop_->removeImpl(*iop);
    pars_ -= iop;
    delete iop;

    writeFile();
    return true;
}


bool Seis2DLineSet::renameLine( const char* oldlnm, const char* newlnm )
{
    if ( !newlnm || !*newlnm )
	return false;

    if ( !waitForLock(false,true) )
	{ ErrMsg("Cannot edit Lineset: LineSet locked"); return false; }

    // Critical concurrency section using file lock
    readFile( true );

    bool foundone = false;
    for ( int idx=0; idx<pars_.size(); idx++ )
    {
	BufferString lnm = lineName( idx );
	if ( lnm == oldlnm )
	    foundone = true;
	if ( lnm == newlnm )
	{
	    ErrMsg("Cannot rename line to existing line name");
	    removeLock();
	    return false;
	}
    }
    if ( !foundone )
	{ removeLock(); return true; }

    for ( int idx=0; idx<pars_.size(); idx++ )
    {
	IOPar& iop = *pars_[idx];
	LineKey lk( *pars_[idx], true );
	BufferString lnm = lk.lineName();
	if ( lnm == oldlnm )
	{
	    lk.setLineName( newlnm );
	    lk.fillPar( iop, true );
	}
    }

    writeFile();
    return true;
}


bool Seis2DLineSet::rename( const char* lk, const char* newlk )
{
    if ( !newlk || !*newlk )
	return false;

    if ( !waitForLock(false,true) )
	{ ErrMsg("Cannot edit Lineset: LineSet locked"); return false; }

    // Critical concurrency section using file lock
    readFile( true );
    int ipar = indexOf( lk );
    if ( ipar < 0 )
    {
	removeLock();
	ErrMsg("Cannot rename non-existent line key");
	return false;
    }

    int existipar = indexOf( newlk );
    if ( existipar >= 0 )
    {
	removeLock();
	ErrMsg("Cannot rename to existing line key");
	return false;
    }

    LineKey(newlk).fillPar( *pars_[ipar], true );
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
