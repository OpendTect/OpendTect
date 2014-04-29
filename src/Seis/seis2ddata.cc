/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Salil Agarwal
 * DATE     : Dec 2012
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seis2ddata.h"

#include "seis2dlineio.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "survgeom2d.h"
#include "survgeom.h"
#include "linesetposinfo.h"
#include "ioman.h"
#include "seis2dline.h"
#include "safefileio.h"
#include "ascstream.h"
#include "binidvalset.h"
#include "posinfo2dsurv.h"
#include "dirlist.h"
#include "survinfo.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "od_ostream.h"



Seis2DDataSet::Seis2DDataSet( const IOObj& ioobj )
    : NamedObject(ioobj.name())
    , ioobjpars_(*new IOPar(ioobj.pars()))
{
    ioobjpars_.get( sKey::Type(), datatype_ );
    init( ioobj.fullUserExpr(true) );
}


Seis2DDataSet::Seis2DDataSet( const Seis2DDataSet& s2dds )
    : NamedObject(s2dds.name())
    , datatype_(s2dds.dataType())
    , ioobjpars_(*new IOPar(s2dds.ioobjpars_))
{
    init( s2dds.fname_ );
}


Seis2DDataSet::~Seis2DDataSet()
{
    delete &ioobjpars_;
    deepErase( pars_ );
}


Seis2DDataSet& Seis2DDataSet::operator =( const Seis2DDataSet& dset )
{
    if ( &dset == this ) return *this;
    fname_ = dset.fname_;
    readDir();
    return *this;
}


void Seis2DDataSet::init( const char* fnm )
{
    readonly_ = false;
    fname_ = fnm;
    BufferString typestr = "CBVS";
    readDir();

    liop_ = 0;
    const ObjectSet<Seis2DLineIOProvider>& liops = S2DLIOPs();
    for ( int idx=0; idx<liops.size(); idx++ )
    {
	const Seis2DLineIOProvider* liop = liops[idx];
	if ( typestr == liop->type() )
	    { liop_ = const_cast<Seis2DLineIOProvider*>(liop); break; }
    }
}


const char* Seis2DDataSet::type() const
{
    return liop_ ? liop_->type() : "CBVS";
}


const char* Seis2DDataSet::lineName( int idx ) const
{
    return pars_.validIdx(idx) ? Survey::GM().getName( geomID(idx) ) : "";
}


Pos::GeomID Seis2DDataSet::geomID( int idx ) const
{
    if ( !pars_.validIdx(idx) )
	return Survey::GM().cUndefGeomID();

    int ret = Survey::GM().cUndefGeomID();
    pars_[idx]->get( sKey::GeomID(), ret );
    return ret;
}


Pos::GeomID Seis2DDataSet::geomID( const char* filename ) const
{
    FilePath fp( filename );
    BufferString fnm( fp.fileName() );
    int cidx = 0;
    while ( cidx < fnm.size() && fnm[cidx] != '^')
	cidx++;
    int pointidx = cidx;
    while ( pointidx < fnm.size() && fnm[pointidx] != '.')
	pointidx++;
    fnm[pointidx] = '\0';
    BufferString strgeomid( fnm + cidx+1 );
    return strgeomid.toInt();
}


int Seis2DDataSet::indexOf( const char* linename ) const
{
    const int geomid = Survey::GM().getGeomID( linename );
    return indexOf( geomid );
}


int Seis2DDataSet::indexOf( Pos::GeomID geomid ) const
{
    for ( int idx=0; idx<pars_.size(); idx++ )
	if ( geomID(idx) == geomid )
	    return idx;

    return -1;
}


void Seis2DDataSet::readDir()
{
    deepErase( pars_ );
    DirList dl( fname_, DirList::FilesOnly );
    if ( dl.size() <= 0 )
	File::createDir( fname_ );

    for ( int idx=0; idx<dl.size(); idx++ )
    {
	FilePath filepath( dl.get(idx) );
	FixedString ext( filepath.extension() );
	if ( !ext.isEqual( "cbvs" ) )
	    continue;

	IOPar* newpar = new IOPar;
	newpar->set( sKey::FileName(), filepath.fileName() );
	filepath.setExtension("");
	BufferString filenm( filepath.fileName() );
	int cidx = 0;
	while ( cidx < filenm.size() && filenm[cidx] != '^')
	    cidx++;

	if ( !cidx || cidx >= filenm.size() )
	    continue;

	BufferString strgeomid( filenm.buf()+cidx+1 );
	Pos::GeomID geomid = strgeomid.toInt();
	newpar->set( sKey::GeomID(), geomid );
	pars_ += newpar;
    }

    return;
}


bool Seis2DDataSet::getGeometry( int ipar, PosInfo::Line2DData& geom ) const
{
    if ( !liop_ )
    {
	ErrMsg("No suitable 2D line information object found");
	return 0;
    }

    if ( ipar < 0 || ipar >= pars_.size() )
    {
	ErrMsg("Line number requested not found in Dataset");
	return 0;
    }

    return liop_->getGeometry( *pars_[ipar], geom );
}


void Seis2DDataSet::getLineNames( BufferStringSet& nms ) const
{
    nms.erase();
    for ( int idx=0; idx<nrLines(); idx++ )
	nms.addIfNew( lineName(idx) );
}


Executor* Seis2DDataSet::lineFetcher( int ipar, SeisTrcBuf& tbuf, int ntps,
				      const Seis::SelData* sd ) const
{
    if ( !liop_ )
    {
	ErrMsg("No suitable 2D line extraction object found");
	return 0;
    }

    if ( ipar < 0 || ipar >= pars_.size() )
    {
	ErrMsg("Line number requested not found in Dataset");
	return 0;
    }

    return liop_->getFetcher( *pars_[ipar], tbuf, ntps, sd );
}


Seis2DLinePutter* Seis2DDataSet::linePutter( IOPar* newiop )
{
    if ( !newiop )
    {
	ErrMsg("No data for line add provided");
	return 0;
    }
    else if ( !liop_ )
    {
	ErrMsg("No suitable 2D line creation object found");
	return 0;
    }

    int newgeomid;
    if ( !newiop->get(sKey::GeomID(),newgeomid) )
	return 0; // TODO: add error message

    newiop->merge( ioobjpars_ );
    BufferString typestr( "CBVS" );

    Seis2DLinePutter* res = 0;
    int paridx = indexOf( newgeomid );
    if ( paridx >= 0 )
    {
	pars_[paridx]->merge( *newiop );
	*newiop = *pars_[paridx];
	delete pars_.replace( paridx, newiop );
	res = liop_->getReplacer( *pars_[paridx] );
    }
    else if ( !readonly_ )
    {
	pars_ += newiop;
	res = liop_->getAdder( *newiop, 0, name() );
    }
    else
    {
	BufferString msg( "Read-only Dataset chg req: " );
	msg += Survey::GM().getName(newgeomid); msg += " not yet in set ";
	msg += name();
	pErrMsg( msg );
    }

    // Phew! Made it.
    return res;
}


bool Seis2DDataSet::addLineFrom( Seis2DDataSet& ds, const char* lnm,
				 const char* dtyp )
{
    if ( !ds.liop_ )
    {
	ErrMsg("No suitable 2D line creation object found");
	return 0;
    }

    if ( pars_.size() < 1 )
	{ return true; }

    BufferStringSet lnms;
    for ( int ipar=0; ipar<pars_.size(); ipar++ )
	lnms.addIfNew( pars_[ipar]->name() );

    TypeSet<Pos::GeomID> geomidtoadd;
    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	const BufferString curlnm( lnms.get(idx) );
	if ( curlnm == lnm )
	{
	    Pos::GeomID geomid = geomID( curlnm );
	    if ( ds.indexOf(geomid) < 0 )
		geomidtoadd += geomid;
	}
    }

    if ( geomidtoadd.size() == 0 )
	{ return true; }

    for ( int idx=0; idx<geomidtoadd.size(); idx++ )
    {
	IOPar* newiop = new IOPar( ds.name() );

	newiop->set( sKey::GeomID(), geomidtoadd[idx] );
	if ( dtyp )
	    newiop->set( sKey::DataType(), dtyp );

	const IOPar* previop = ds.pars_.size() ? ds.pars_[ds.pars_.size()-1]
						: 0;
	ds.pars_ += newiop;
	delete ds.liop_->getAdder( *newiop, previop, ds.name() );
    }

    return true;
}


bool Seis2DDataSet::isEmpty( int ipar ) const
{
    return liop_ ? liop_->isEmpty( *pars_[ipar] ) : true;
}


bool Seis2DDataSet::remove( Pos::GeomID geomid )
{
    if ( readonly_ ) return false;

    int ipar = indexOf( geomid );
    if ( ipar < 0 )
	{ return true; }

    IOPar* iop = pars_[ipar];
    if ( liop_ )
	liop_->removeImpl(*iop);
    pars_ -= iop;
    delete iop;

    return true;
}


bool Seis2DDataSet::renameFiles( const char* newname )
{
    DirList dl( fname_, DirList::FilesOnly );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	FilePath oldfp = dl.fullPath( idx );
	Pos::GeomID geomid = geomID( oldfp.fileName() );
	const char* ext = oldfp.extension();
	BufferString newfnm( newname );
	newfnm += "^"; newfnm += geomid;
	FilePath newfullfilepath( oldfp.pathOnly() );
	newfullfilepath.add( newfnm );
	newfullfilepath.setExtension( ext );
	if ( !File::rename(oldfp.fullPath(), newfullfilepath.fullPath()) )
	    return false;
    }

    return true;
}


bool Seis2DDataSet::getTxtInfo( int ipar, BufferString& uinf,
				  BufferString& stdinf ) const
{
    return liop_ ? liop_->getTxtInfo(*pars_[ipar],uinf,stdinf) : false;
}


bool Seis2DDataSet::getRanges( int ipar, StepInterval<int>& sii,
				 StepInterval<float>& sif ) const
{
    return liop_ ? liop_->getRanges(*pars_[ipar],sii,sif) : false;
}


bool Seis2DDataSet::haveMatch( int ipar, const BinIDValueSet& bivs ) const
{
    const Survey::Geometry* geometry = Survey::GM().getGeometry( geomID(ipar) );
    mDynamicCastGet( const Survey::Geometry2D*, geom2d, geometry )
    if ( !geom2d ) return false;

    const PosInfo::Line2DData& geom = geom2d->data();
    for ( int idx=0; idx<geom.positions().size(); idx++ )
    {
	if ( bivs.includes( SI().transform(geom.positions()[idx].coord_) ) )
	    return true;
    }

    return false;
}


void Seis2DDataSet::getDataSetsOnLine( const char* lnm, BufferStringSet& ds )
{ Seis2DDataSet::getDataSetsOnLine( Survey::GM().getGeomID(lnm), ds ); }


void Seis2DDataSet::getDataSetsOnLine( const Pos::GeomID geomid,
				       BufferStringSet& ds )
{
    ds.erase();
    TypeSet<BufferStringSet> linenames;
    BufferStringSet datasets;
    SeisIOObjInfo::get2DDataSetInfo( datasets, 0, &linenames );
    for ( int idx=0; idx<datasets.size(); idx++ )
    {
	for ( int lineidx=0; lineidx<linenames[idx].size(); lineidx++ )
	{
	    if ( Survey::GM().getGeomID(linenames[idx].get(lineidx)) == geomid )
	    {
		ds.add( datasets.get(idx) );
		break;
	    }
	}
    }
}
