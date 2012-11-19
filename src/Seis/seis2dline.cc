/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : June 2004
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seis2dline.h"
#include "seis2dlineio.h"
#include "seisselection.h"
#include "seisbuf.h"
#include "surv2dgeom.h"
#include "linesetposinfo.h"
#include "cubesampling.h"
#include "safefileio.h"
#include "ascstream.h"
#include "bufstringset.h"
#include "binidvalset.h"
#include "survinfo.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "zdomain.h"

#include <sstream>


struct Seis2DLineSetCache
{
    Threads::Mutex	lock_;
    BufferString	name_;
    BufferString	fname_;
    BufferString	typestr_;
    ObjectSet<IOPar>	pars_;
};

static Seis2DLineSetCache cache;


Seis2DLineSet::Seis2DLineSet( const IOObj& ioobj )
    : NamedObject("")
{
    init( ioobj.fullUserExpr(true) );
}


Seis2DLineSet::~Seis2DLineSet()
{
    deepErase( pars_ );
}


static BufferStringSet& preSetFiles()
{
    static BufferStringSet* psf = 0; if ( !psf ) psf = new BufferStringSet;
    return *psf;
}
static BufferStringSet& preSetContents()
{
    static BufferStringSet* psc = 0; if ( !psc ) psc = new BufferStringSet;
    return *psc;
}


void Seis2DLineSet::addPreSetLS( const char* fnm, const char* def )
{
    preSetFiles().add( fnm );
    preSetContents().add( def );
}


Seis2DLineSet& Seis2DLineSet::operator =( const Seis2DLineSet& lset )
{
    if ( &lset == this ) return *this;
    fname_ = lset.fname_;
    readFile( false );
    return *this;
}


void Seis2DLineSet::init( const char* fnm )
{
    readonly_ = false;
    fname_ = fnm;
    BufferString typestr = "CBVS";

    cache.lock_.lock();

    if ( fname_ == cache.fname_ )
    {
	deepCopy( pars_, cache.pars_ );
	setName( cache.name_ );
	typestr = cache.typestr_;
    }
    else
    {
	readFile( false, &typestr );
	deepCopy( cache.pars_, pars_ );
	cache.name_ = name();
	cache.fname_ = fnm;
	cache.typestr_ = typestr;
    }

    cache.lock_.unLock();

    liop_ = 0;
    const ObjectSet<Seis2DLineIOProvider>& liops = S2DLIOPs();
    for ( int idx=0; idx<liops.size(); idx++ )
    {
	const Seis2DLineIOProvider* liop = liops[idx];
	if ( typestr == liop->type() )
	    { liop_ = const_cast<Seis2DLineIOProvider*>(liop); break; }
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
		    ? pars_[idx]->find(sKey::Attribute()) : 0;
    return res ? res : LineKey::sKeyDefAttrib();
}


const char* Seis2DLineSet::datatype( int idx ) const
{
    const char* res = idx >=0 && idx < pars_.size()
		    ? pars_[idx]->find(sKey::DataType()) : 0;
    return res;
}


const char* Seis2DLineSet::zDomainKey( int idx ) const
{
    const char* res = idx >=0 && idx < pars_.size()
		    ? pars_[idx]->find(ZDomain::sKey()) : 0;
    return res && *res ? res : ZDomain::SI().key();
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


int Seis2DLineSet::indexOfFirstOccurrence( const char* linenm ) const
{
    for ( int idx=0; idx<pars_.size(); idx++ )
    {
	if ( pars_[idx]->name() == linenm )
	    return idx;
    }

    return -1;
}


static const char* sKeyFileType = "2D Line Group Data";

void Seis2DLineSet::readFile( bool mklock, BufferString* typestr )
{
    deepErase( pars_ );
    if ( getPre(typestr) )
	return;

    SafeFileIO sfio( fname_, true );
    if ( !sfio.open(true) )
    {
	if ( name().isEmpty() )
	{
	    FilePath fp( fname_ );
	    fp.setExtension( 0 );
	    setName( fp.fileName() );
	}
	return;
    }

    getFrom( sfio.istrm(), typestr );
    sfio.closeSuccess( mklock );
}


bool Seis2DLineSet::getPre( BufferString* typestr )
{
    if ( preSetFiles().size() < 1 )
	return false;

    int psidx = preSetFiles().indexOf( fname_.buf() );
    if ( psidx < 0 )
    {
	FilePath fp( fname_ );
	psidx = preSetFiles().indexOf( fp.fileName() );
	if ( psidx < 0 )
	    return false;
    }

    std::string stdstr( preSetContents().get(psidx).buf() );
    std::istringstream istrstrm( stdstr );
    getFrom( istrstrm, typestr );
    readonly_ = true;
    return true;
}


void Seis2DLineSet::getFrom( std::istream& strm, BufferString* typestr )
{
    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(sKeyFileType) )
	return;

    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(sKey::Name()) )
	    setName( astrm.value() );
	if ( astrm.hasKeyword(sKey::Type()) && typestr )
	    *typestr = astrm.value();
    }

    while ( astrm.type() != ascistream::EndOfFile )
    {
	IOPar* newpar = new IOPar;
	while ( !atEndOfSection(astrm.next()) )
	{
	    if ( astrm.hasKeyword(sKey::Name()) )
		newpar->setName( astrm.value() );
	    else if ( !astrm.hasValue("") )
		newpar->set( astrm.keyWord(), astrm.value() );
	}
	if ( newpar->size() < 1 )
	    delete newpar;
	else
	    pars_ += newpar;
    }
}


void Seis2DLineSet::writeFile() const
{
    if ( !readonly_ )
    {
	SafeFileIO sfio( fname_, true );
	if ( sfio.open(false,true) )
	{
	    putTo( sfio.ostrm() );
	    sfio.closeSuccess();
	}
    }
}


void Seis2DLineSet::putTo( std::ostream& strm ) const
{
    cache.lock_.lock();
    if ( cache.fname_==fname_ || cache.name_==name() )
	cache.fname_.setEmpty();			/* Invalidate cache */
    cache.lock_.unLock();

    ascostream astrm( strm );
    if ( !astrm.putHeader(sKeyFileType) )
	return;

    astrm.put( sKey::Name(), name() );
    astrm.put( sKey::Type(), type() );
    astrm.put( "Number of lines", pars_.size() );
    astrm.newParagraph();

    for ( int ipar=0; ipar<pars_.size(); ipar++ )
    {
	const IOPar& iopar = *pars_[ipar];
	astrm.put( sKey::Name(), iopar.name() );
	for ( int idx=0; idx<iopar.size(); idx++ )
	{
	    const char* val = iopar.getValue(idx);
	    if ( !val || !*val ) continue;
	    astrm.put( iopar.getKey(idx), iopar.getValue(idx) );
	}
	astrm.newParagraph();
    }
}


void Seis2DLineSet::removeLock() const
{
    SafeFileIO sfio( fname_, true );
    sfio.open( true, true );
    sfio.closeFail( false );
}


bool Seis2DLineSet::getGeometry( int ipar, PosInfo::Line2DData& geom ) const
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


bool Seis2DLineSet::getGeometry( PosInfo::LineSet2DData& lsgeom ) const
{
    for ( int idx=0; idx<pars_.size(); idx++ )
    {
	const char* lnm = pars_[idx]->name();
	if ( lsgeom.getLineData(lnm) )
	    continue;

	PosInfo::Line2DData& ld = lsgeom.addLine( lnm );
	getGeometry( idx, ld );
	if ( ld.positions().isEmpty() )
	{
	    lsgeom.removeLine( lnm );
	    return false;
	}
    }
    
    return true;
}


Executor* Seis2DLineSet::lineFetcher( int ipar, SeisTrcBuf& tbuf, int ntps,
					const Seis::SelData* sd ) const
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

    return liop_->getFetcher( *pars_[ipar], tbuf, ntps, sd );
}


Seis2DLinePutter* Seis2DLineSet::linePutter( IOPar* newiop )
{
    if ( !newiop || newiop->name().isEmpty() )
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

    readFile( true );

    Seis2DLinePutter* res = 0;
    int paridx = indexOf( newlinekey );
    if ( paridx >= 0 )
    {
	pars_[paridx]->merge( *newiop );
	*newiop = *pars_[paridx];
	delete pars_.replace( paridx, newiop );
	res = liop_->getReplacer( *pars_[paridx] );
    }
    else if ( !readonly_ )
    {
	const IOPar* previop = pars_.size() ? pars_[pars_.size()-1] : 0;
	pars_ += newiop;
	res = liop_->getAdder( *newiop, previop, name() );
    }
    else
    {
	BufferString msg( "Read-only line set chg req: " );
	msg += newlinekey; msg += " not yet in set ";
	msg += name();
	pErrMsg( msg );
    }

    if ( res )
	writeFile();
    else
	removeLock();

    // Phew! Made it.
    return res;
}


bool Seis2DLineSet::addLineKeys( Seis2DLineSet& ls, const char* attrnm,
				 const char* lnm, const char* dtyp )
{
    if ( !ls.liop_ )
    {
	ErrMsg("No suitable 2D line creation object found");
	return 0;
    }

    readFile( true );
    if ( pars_.size() < 1 )
	{ removeLock(); return true; }

    BufferStringSet lnms;
    for ( int ipar=0; ipar<pars_.size(); ipar++ )
	lnms.addIfNew( pars_[ipar]->name() );

    ObjectSet<LineKey> lkstoadd;
    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	const BufferString curlnm( lnms.get(idx) );
	if ( !lnm || !*lnm || curlnm == lnm )
	{
	    LineKey* newlk = new LineKey( curlnm, attrnm );
	    if ( ls.indexOf( *newlk ) < 0 )
		lkstoadd += newlk;
	    else
		delete newlk;
	}
    }

    if ( lkstoadd.size() == 0 )
	{ removeLock(); return true; }

    if ( &ls != this )
    {
	removeLock();
	ls.readFile( true );
    }

    IOPar iop( *pars_[0] ); iop.setName( ls.name() );
    iop.removeWithKey( sKey::FileName() );
    for ( int idx=0; idx<lkstoadd.size(); idx++ )
    {
	IOPar* newiop = new IOPar( iop );
	lkstoadd[idx]->fillPar( *newiop, true );
	if ( dtyp )
	    newiop->set( sKey::DataType(), dtyp );

	const IOPar* previop = ls.pars_.size() ? ls.pars_[ls.pars_.size()-1]
	    					: 0;
	ls.pars_ += newiop;
	delete ls.liop_->getAdder( *newiop, previop, ls.name() );
    }

    ls.writeFile();
    // End critical section - lock released
    return true;
}


bool Seis2DLineSet::isEmpty( int ipar ) const
{
    return liop_ ? liop_->isEmpty( *pars_[ipar] ) : true;
}


bool Seis2DLineSet::remove( const char* lk )
{
    if ( readonly_ ) return false;

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
    if ( readonly_ ) return false;

    if ( !newlnm || !*newlnm )
	return false;

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
	    PosInfo::POS2DAdmin().renameLine(oldlnm,newlnm); 
	}
    }

    writeFile();
    return true;
}


bool Seis2DLineSet::rename( const char* lsnm )
{
    if ( !lsnm )
	return false;

    setName( lsnm );
    writeFile();

    return true;
}


bool Seis2DLineSet::rename( const char* lk, const char* newlk )
{
    if ( readonly_ ) return false;

    if ( !newlk || !*newlk )
	return false;

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


bool Seis2DLineSet::renameFiles( const char* newlsnm )
{
    BufferString cleannm( newlsnm );
    cleanupString( cleannm.buf(), false, false, false );
    if ( fname_.isEmpty() )
	return false;

    BufferString oldlsnm;
    if ( !pars_.size() )
	return false;

    pars_[0]->get( sKey::FileName(), oldlsnm );
    int index = 0;
    while ( true )
    {
	if ( oldlsnm[index] == '^' || oldlsnm[index] == '.' )
	    break;
	index++;
    }

    oldlsnm[index] = '\0';

    FilePath fp( fname_ );
    for ( int idx=0; idx<nrLines(); idx++ )
    {
	BufferString filenm, oldfilenm;
	pars_[idx]->get( sKey::FileName(), filenm );
	oldfilenm = filenm;
	replaceString( filenm.buf(), oldlsnm.buf(), cleannm.buf() );
	FilePath newfp( fp.pathOnly(), filenm );
	FilePath oldfp( fp.pathOnly(), oldfilenm );
	if ( oldfp.isEmpty() || newfp.isEmpty() || oldfp == newfp )
	    continue;

	if ( !File::rename(oldfp.fullPath(),newfp.fullPath()) )
	{
	    renameFiles( oldlsnm );
	    return false;
	}

	newfp.setExtension( "par" );
	oldfp.setExtension( "par" );
	File::rename( oldfp.fullPath(), newfp.fullPath() );

	pars_[idx]->set( sKey::FileName(), filenm );
    }

    PosInfo::POS2DAdmin().renameLineSet( oldlsnm, newlsnm );
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


void Seis2DLineSet::getAvailableAttributes( BufferStringSet& nms,
       					    const char* datatypptr,
					    bool allowcnstabsent, 
					    bool incl ) const
{
    FixedString datatyp( datatypptr );
    nms.erase();
    const int sz = nrLines();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( datatyp )
	{
	    const char* founddatatype = datatype(idx);

	    if ( !founddatatype)
	    {
		if ( allowcnstabsent )	
		{
		    if ( datatyp!=attribute(idx) )
			nms.addIfNew( attribute(idx) );
		}
		else
		{
		    if ( datatyp==attribute(idx) )
			nms.addIfNew( attribute(idx) );
		}
	    }
	    else
	    {
		if ( datatyp==founddatatype && incl )
		    nms.addIfNew( attribute(idx) );
	    }
	}
	else
	    nms.addIfNew( attribute(idx) );
    }

    nms.sort();
}


void Seis2DLineSet::getZDomainAttrib( BufferStringSet& nms,
				      const char* linenmptr,
				      const char* zdomainstr )
{
    FixedString linenm = linenmptr;
    for ( int idx=0; idx<nrLines(); idx++ )
    {
	if ( linenm==lineName(idx) )
	{
	    BufferString ztype = zDomainKey( idx );
	    if ( zdomainstr && ztype!=zdomainstr )
		continue;
	    nms.add( attribute(idx) );
	}
    }
}


void Seis2DLineSet::getLineNamesWithAttrib( BufferStringSet& nms,
					    const char* attrnm ) const
{
    nms.erase();
    for ( int idx=0; idx<nrLines(); idx++ )
    {
	const LineKey lk( lineName(idx), attrnm );
	if ( lineKey(idx) == lk )
	    nms.addIfNew( lineName(idx) );
    }
}


const char* Seis2DLineSet::getCubeSampling( CubeSampling& cs,
       					    const LineKey& lk ) const
{
    const BufferString lnm( lk.lineName() );
    const BufferString anm( lk.attrName() );
    if ( anm == LineKey::sKeyDefAttrib() )
	const_cast<BufferString&>( anm ) = "";

    const bool haveln = !lnm.isEmpty();
    const bool haveattr = !anm.isEmpty();
    const int nrlines = nrLines();
    const char* errmsg = "No matches found for line selection";
    for ( int iln=0; iln<nrlines; iln++ )
    {
	if ( haveln && lnm != lineName(iln) )
	    continue;
	if ( haveattr )
	{
	    BufferString curanm( attribute(iln) );
	    if ( curanm == LineKey::sKeyDefAttrib() ) curanm = "";
	    if ( anm != curanm )
		continue;
	}

	if ( errmsg )
	    errmsg = getCubeSampling( cs, iln );
	else
	{
	    StepInterval<int> trg; StepInterval<float> zrg;
	    if ( getRanges(iln,trg,zrg) )
	    {
		if ( cs.hrg.start.crl > trg.start ) cs.hrg.start.crl =trg.start;
		if ( cs.hrg.stop.crl < trg.stop ) cs.hrg.stop.crl = trg.stop;
		if ( cs.hrg.step.crl > trg.step ) cs.hrg.step.crl = trg.step;
		if ( cs.zrg.start > zrg.start ) cs.zrg.start = zrg.start;
		if ( cs.zrg.stop < zrg.stop ) cs.zrg.stop = zrg.stop;
		if ( cs.zrg.step > zrg.step ) cs.zrg.step = zrg.step;
	    }
	}
    }

    return errmsg;
}


const char* Seis2DLineSet::getCubeSampling( CubeSampling& cs, int lnr ) const
{
    cs.hrg.step.inl = cs.hrg.step.crl = 1;
    cs.hrg.start.inl = 0; cs.hrg.stop.inl = nrLines()-1;
    cs.hrg.start.crl = 0; cs.hrg.stop.crl = mUdf(int);
    cs.zrg = SI().zRange(false);
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


bool Seis2DLineSet::haveMatch( int ipar, const BinIDValueSet& bivs ) const
{
    S2DPOS().setCurLineSet( name() );
    PosInfo::Line2DData geom( lineName(ipar) );
    if ( !S2DPOS().getGeometry(geom) )
	return false;

    for ( int idx=0; idx<geom.positions().size(); idx++ )
    {
	if ( bivs.includes( SI().transform(geom.positions()[idx].coord_) ) )
	    return true;
    }

    return false;
}


void Seis2DLineSet::preparePreSet( IOPar& iop, const char* reallskey ) const
{
    FilePath fp( fname_ );
    iop.set( IOPar::compKey(reallskey,sKey::FileName()), fp.fileName() );
}


void Seis2DLineSet::installPreSet( const IOPar& iop, const char* reallskey,
				   const char* worklskey )
{
    const char* reallsfnm = iop.find(
	    			IOPar::compKey(reallskey,sKey::FileName()) );
    if ( !reallsfnm ) return;

    const char* worklsfnm = iop.find(
	    			IOPar::compKey(worklskey,sKey::FileName()) );
    if ( !worklsfnm ) return;

    Seis2DLineSet ls( worklsfnm );
    std::ostringstream strstrm;
    ls.putTo( strstrm );
    addPreSetLS( reallsfnm, strstrm.str().c_str() );
}


class Seis2DGeomDumper : public Executor
{
public:

Seis2DGeomDumper( const Seis2DLineSet& l, std::ostream& o, bool inr, float z,
		  const char* lk )
	: Executor("Geometry extraction")
	, ls(l)
	, strm(o)
	, donr(inr)
	, curidx(-1)
	, totalnr(-1)
	, lnshandled(0)
	, ptswritten(0)
	, zval(z)
	, incz(!mIsUdf(z))
	, dolnm(true)
{
    lastidx = ls.nrLines() - 1;
    if ( lastidx < 0 )
    {
	curmsg = "No lines in line set";
	return;
    }

    curidx = 0;
    if ( lk && *lk )
    {
	LineKey lnky( lk );
	attrnm = lnky.attrName();
	if ( !lnky.lineName().isEmpty() )
	{
	    curidx = ls.indexOf( lk );
	    if ( curidx < 0 )
	    {
		curmsg = "Could not find '";
		if ( !attrnm.isEmpty() )
		    { curmsg += attrnm; curmsg += "' for '"; }
		curmsg += lnky.lineName(); curmsg += "' in line set";
		return;
	    }
	    lastidx = curidx;
	    dolnm = false;
	}
    }

    if ( attrnm.isEmpty() )
	attrnm = LineKey::sKeyDefAttrib();
    totalnr = lastidx - curidx + 1;
    curmsg = "Extracting geometry";
}

const char* message() const	{ return curmsg.buf(); }
const char* nrDoneText() const	{ return "Lines handled"; }
od_int64 nrDone() const		{ return lnshandled; }
od_int64 totalNr() const	{ return totalnr; }

int nextStep()
{
    if ( curidx < 0 )
	return ErrorOccurred();
    if ( !strm.good() )
    {
	curmsg = "Cannot write to file";
	return ErrorOccurred();
    }

    for ( ; curidx<=lastidx && attrnm!=ls.attribute(curidx); )
	{ curidx++; totalnr--; }

    if ( curidx > lastidx )
    {
	if ( ptswritten == 0 )
	{
	    curmsg = "No output created";
	    return ErrorOccurred();
	}
	return Finished();
    }

    S2DPOS().setCurLineSet( ls.name() );
    PosInfo::Line2DData geom( ls.lineKey(curidx).lineName() );
    if ( !S2DPOS().getGeometry(geom) )
    {
	curmsg = "Couldn't get geometry for '";
	curmsg += ls.lineKey( curidx );
	curmsg += "'";
	return totalnr == 1		? ErrorOccurred()
	     : (curidx == lastidx	? Finished()
					: WarningAvailable());
    }

    BufferString outstr;
    const float zfac = mCast( float, SI().zDomain().userFactor() );
    const TypeSet<PosInfo::Line2DPos>& posns = geom.positions();
    for ( int idx=0; idx<posns.size(); idx++ )
    {
	const PosInfo::Line2DPos& pos = posns[idx];
	outstr = "";
	if ( dolnm )
	    { outstr += ls.lineKey(curidx); outstr += "\t"; }
	if ( donr )
	    { outstr += pos.nr_; outstr += "\t"; }
	outstr += pos.coord_.x; outstr += "\t";
	outstr += pos.coord_.y;
	if ( incz )
	    { outstr += "\t"; outstr += zval * zfac; }
	strm << outstr << '\n';
	ptswritten++;
    }
    strm.flush();

    if ( !strm.good() )
    {
	curmsg = "Error during write to file";
	return ErrorOccurred();
    }

    lnshandled++;
    curidx++;
    return curidx > lastidx ? Finished() : MoreToDo();
}

    const Seis2DLineSet&	ls;
    std::ostream&		strm;
    BufferString		attrnm;
    const bool			incz;
    const float			zval;
    bool			donr;
    bool			dolnm;
    int				curidx;
    int				lastidx;
    int				totalnr;
    int				lnshandled;
    int				ptswritten;
    BufferString		curmsg;

};


Executor* Seis2DLineSet::geometryDumper( std::ostream& strm, bool incnr,
					 float z, const char* lk ) const
{
    return new Seis2DGeomDumper( *this, strm, incnr, z, lk );
}


void Seis2DLineSet::invalidateCache()
{
    cache.lock_.lock();
    cache.fname_.setEmpty();
    cache.lock_.unLock();
}
