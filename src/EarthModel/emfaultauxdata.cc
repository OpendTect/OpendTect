/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          Aug 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "emfaultauxdata.h"

#include "arrayndimpl.h"
#include "ascstream.h"
#include "binidvalset.h"
#include "emfault3d.h"
#include "emsurfacegeometry.h"
#include "emsurfacetr.h"
#include "emsurfauxdataio.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "parametricsurface.h"
#include "ptrman.h"
#include "safefileio.h"
#include "settings.h"
#include "strmprov.h"
#include "varlenarray.h"

namespace EM
{

FaultAuxData::FaultAuxData( const Fault3D& flt )
    : fault_(flt)
    , surfdata_(0)  
    , changed_(false)
{
    fltfullnm_.setEmpty();
    init();
}


FaultAuxData::~FaultAuxData()
{
    delete surfdata_;
    deepErase( sdusernames_ );
    deepErase( sdfilenames_ );
}


void FaultAuxData::init()
{
    PtrMan<IOObj> ioobj = IOM().get( fault_.multiID() );
    if ( !ioobj ) 
	return;
    
    FilePath fp( ioobj->fullUserExpr(true) );
    fp.setExtension( "" );
    fltfullnm_ = fp.fullPath();

    deepErase( sdusernames_ );
    deepErase( sdfilenames_ );
    
    ObjectSet<IOPar> pars;
    readSDInfoFile( pars );
    for ( int idx=pars.size()-1; idx>=0; idx-- )
    {
	BufferString unm, fnm;
	pars[idx]->get( sKey::Name(), unm );
	pars[idx]->get( sKey::FileName(), fnm );
	sdusernames_.add( unm.buf() );
	sdfilenames_.add( fnm.buf() );
    }
    deepErase( pars );
}


const BufferStringSet& FaultAuxData::auxDataList()  
{ 
    if ( fltfullnm_.isEmpty() )
	init();

    return sdusernames_; 
}


const char* FaultAuxData::dataName( int sdidx ) const
{ 
    return sdidx<0 || sdidx>=sdusernames_.size() ? 0 : 
	sdusernames_[sdidx]->buf(); 
}


int FaultAuxData::dataIndex( const char* nm ) const
{
    for ( int idx=0; idx<sdusernames_.size(); idx++ )
    {
	if ( sdusernames_[idx] && sdusernames_.get(idx)==nm )
	    return idx;
    }

    return -1;
}


int FaultAuxData::addData( const char* name )
{
    updateDataInfoFile( Add, -1, name );
    return sdusernames_.size()-1;
}


void FaultAuxData::removeData( int sdidx )
{ updateDataInfoFile( Remove, sdidx, 0 ); }


void FaultAuxData::setDataName( int sdidx, const char* name )
{ updateDataInfoFile( Rename, sdidx, name ); }


void FaultAuxData::updateDataInfoFile( Action act, int sdidx,
	const char* sdname )
{
    if ( fltfullnm_.isEmpty() )
	init();

    FilePath fp( fltfullnm_ );
    fp.setExtension( sKeyExtension() );
    FilePath backupfp(fp);
    backupfp.setExtension(".old");

    ObjectSet<IOPar> pars;
    int existparidx = -1;
    if ( File::exists(fp.fullPath()) )
    {
	readSDInfoFile(pars);
	for ( int idx=pars.size()-1; idx>=0; idx-- )
	{
	    BufferString unm, snm;
	    pars[idx]->get( sKey::Name(), unm );
	    pars[idx]->get( sKey::FileName(), snm );
	    if ( unm==sdname )
	    {
		existparidx = idx;
		break;
	    }
	}

	if ( (existparidx!=-1 && act==Add) || (existparidx==-1 && act!=Add) )
	    return;

        File::rename(fp.fullPath(), backupfp.fullPath());
    }

    SafeFileIO sfio( fp.fullPath(), true );
    if ( !sfio.open(false,true) )
    {
	deepErase( pars );
	File::remove( backupfp.fullPath() );
	return;
    }

    ascostream astream( sfio.ostrm() );
    astream.putHeader( sKeyFaultAuxData() );
    
    BufferString enm( sdname );
    cleanupString( enm.buf(), false, false, false );
    BufferString filenm( fault_.name() );
    cleanupString( filenm.buf(), false, false, false );
    filenm.add( "." );
    filenm.add( enm );

    if ( act==Add )
    {
	IOPar* par = new IOPar();
	par->set( sKey::Name(), sdname );
	par->set( sKey::FileName(), filenm );
	pars += par;
	
	sdusernames_.add( sdname );
	sdfilenames_.add( filenm );
    }
    else
    {
	if ( act==Remove )
	{
	    delete pars.removeSingle(existparidx);
	    sdusernames_.removeSingle( existparidx );
    	    sdfilenames_.removeSingle( existparidx );
	}
	else
	{
	    pars[existparidx]->set( sKey::Name(), sdname );
	    pars[existparidx]->set( sKey::FileName(), filenm );
	    delete sdusernames_.replace( existparidx, 
		    new BufferString(sdname) );
    	    delete sdfilenames_.replace( existparidx, 
		    new BufferString(filenm) );
	}
    }

    astream.put( "Number of surface data", pars.size() );
    astream.newParagraph();
    
    IOPar allpars;
    for ( int pidx=0; pidx<pars.size(); pidx++ )
	allpars.mergeComp( *pars[pidx], toString(pidx) );

    allpars.putTo( astream );
    
    deepErase( pars );
    File::remove( backupfp.fullPath() );
    
    sfio.closeSuccess();
    changed_ = true;
}


void FaultAuxData::readSDInfoFile( ObjectSet<IOPar>& sdnmpars )
{
    if ( fltfullnm_.isEmpty() )
	init();

    deepErase( sdnmpars );
    FilePath fp( fltfullnm_ );
    fp.setExtension( sKeyExtension() );

    SafeFileIO sfio( fp.fullPath(), false );
    if ( !sfio.open(true) )
	return;

    std::istream& istrm = sfio.istrm();
    ascistream astrm( istrm, true );
    if ( !astrm.isOfFileType( sKeyFaultAuxData() ) )
    {
	sfio.closeSuccess();
	return;
    }
    
    while ( !atEndOfSection(astrm.next()) )
	continue;

    IOPar allpars;
    allpars.getFrom( astrm );
    
    int idx = 0;
    while ( true )
    {
	IOPar* par = allpars.subselect( idx );
	if ( !par ) break;
	sdnmpars += par;
	idx++;
    }
    sfio.closeSuccess();
}


float FaultAuxData::getDataVal( int sdidx, const PosID& posid ) const
{
    if ( !sdusernames_.validIdx(sdidx) )
	return mUdf(float);

    const BinID geomrc( posid.getRowCol() );
    const BinIDValueSet::Pos pos = surfdata_->findFirst( geomrc );
    if ( !pos.valid() )
	return mUdf(float);

    return surfdata_->getVals( pos )[sdidx];
}


void FaultAuxData::setDataVal( int sdidx, const PosID& posid, float val )
{
    if ( !sdusernames_.validIdx(sdidx) )
	return;

    const BinID geomrc( posid.getRowCol() );
    const BinIDValueSet::Pos pos = surfdata_->findFirst( geomrc );
    if ( !pos.valid() )
    {
	mAllocVarLenArr( float, vals, surfdata_->nrVals() );
	for ( int idx=0; idx<surfdata_->nrVals(); idx++ )
	    vals[idx] = mUdf(float);

	vals[sdidx] = val;
	surfdata_->add( geomrc, vals );
    }
    else
    {
	surfdata_->getVals( pos )[sdidx] = val;
    }

    changed_ = true;
}


Executor* FaultAuxData::dataLoader( int selidx )
{
    if ( fltfullnm_.isEmpty() )
	init();
    
    PtrMan<IOObj> ioobj = IOM().get( fault_.multiID() );
    if ( !ioobj ) 
	return 0; 

    PtrMan<EMSurfaceTranslator> tr = 
	(EMSurfaceTranslator*)ioobj->createTranslator();
    if ( !tr || !tr->startRead(*ioobj) )
	return 0;

    SurfaceIODataSelection& sel = tr->selections();
    int nrauxdata = sel.sd.valnames.size();
    if ( !nrauxdata || selidx >= nrauxdata ) return 0;

    ExecutorGroup* grp = new ExecutorGroup( "Surface attributes reader" );
    for ( int validx=0; validx<sel.sd.valnames.size(); validx++ )
    {
	if ( selidx>=0 && selidx != validx ) continue;

	BufferString filenm = getFileName( *ioobj, 
		sel.sd.valnames[validx]->buf() );
	if ( filenm.isEmpty() ) continue;

	dgbSurfDataReader* rdr = new dgbSurfDataReader(filenm.buf());
	//rdr->setSurface( fault_ );
	grp->add( rdr );
    }

    return grp;
}


Executor* FaultAuxData::dataSaver( int dataidx, bool overwrite )
{
    PtrMan<IOObj> ioobj = IOM().get( fault_.multiID() );
    if ( !ioobj )
    	return 0; 

    bool binary = true;
    mSettUse(getYN,"dTect.Surface","Binary format",binary);

    BufferString fnm;
    if ( overwrite )
    {
	if ( dataidx<0 ) dataidx = 0;
	fnm = getFileName( *ioobj, dataName(dataidx) );
	if ( !fnm.isEmpty() )
	    return 0;//new dgbSurfDataWriter(fault_,dataidx,0,binary,fnm.buf());
    }

    ExecutorGroup* grp = new ExecutorGroup( "Surface attributes saver" );
    grp->setNrDoneText( "Nr done" );
    for ( int selidx=0; selidx<sdusernames_.size(); selidx++ )
    {
	if ( dataidx >= 0 && dataidx != selidx ) continue;
	//fnm = getFreeFileName( *ioobj );
	Executor* exec = 0;
	    //new dgbSurfDataWriter(fault_,selidx,0,binary,fnm.buf());
	grp->add( exec );
    }

    return grp;
}


BufferString FaultAuxData::getFileName( const IOObj& ioobj, 
					    const char* attrnm )
{ return getFileName( ioobj.fullUserExpr(true), attrnm ); }


BufferString FaultAuxData::getFileName( const char* fulluserexp, 
					    const char* attrnm )
{
    const BufferString basefnm( fulluserexp );
    BufferString fnm; int gap = 0;
    for ( int idx=0; ; idx++ )
    {
	if ( gap > 100 ) return "";

	fnm = EM::dgbSurfDataWriter::createHovName(basefnm,idx);
	if ( File::isEmpty(fnm.buf()) )
	    { gap++; continue; }

	EM::dgbSurfDataReader rdr( fnm.buf() );
	if ( !strcmp(rdr.dataName(),attrnm) )
	    break;
    }

    return fnm;
}

/*
bool FaultAuxData::removeFile( const IOObj& ioobj, const char* attrnm )
{
    const BufferString fnm = getFileName( ioobj, attrnm );
    return !fnm.isEmpty() ? File::remove( fnm ) : false;
}
 

BufferString FaultAuxData::getFileName( const char* attrnm ) const
{
    PtrMan<IOObj> ioobj = IOM().get( fault_.multiID() );
    return ioobj ? FaultAuxData::getFileName( *ioobj, attrnm ) : "";
}


bool FaultAuxData::removeFile( const char* attrnm ) const
{
    PtrMan<IOObj> ioobj = IOM().get( fault_.multiID() );
    return ioobj ? FaultAuxData::removeFile( *ioobj, attrnm ) : false;
}*/


Array2D<float>* FaultAuxData::createArray2D( int sdidx ) const
{
    if ( fault_.geometry().sectionGeometry(0)->isEmpty() )
	return 0;
/*
    const StepInterval<int> rowrg = fault_.geometry().rowRange( 0 );
    const StepInterval<int> colrg = fault_.geometry().colRange( 0, -1 );

    PosID posid( fault_.id(), sid );
    Array2DImpl<float>* arr =
	new Array2DImpl<float>( rowrg.nrSteps()+1, colrg.nrSteps()+1 );
    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	{
	    posid.setSubID( RowCol(row,col).toInt64() );
	    const float val = getDataVal( sdidx, posid);
	    arr->set( rowrg.getIndex(row), colrg.getIndex(col), val );
	}
    }

    return arr;*/
    return 0;
}


void FaultAuxData::setArray2D( int sdidx, const Array2D<float>& arr2d )
{
    const Geometry::RowColSurface* rcs = fault_.geometry().sectionGeometry(0);
    if ( !rcs || rcs->isEmpty() )
	return;
/*
    const StepInterval<int> rowrg = rcs->rowRange();
    const StepInterval<int> colrg = rcs->colRange();
    PosID posid( fault_.id(), sid );
    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	{
	    posid.setSubID( RowCol(row,col).toInt64() );
	    const float val = arr2d.get( rowrg.getIndex(row),
		    			 colrg.getIndex(col) );
	    setDataVal( sdidx, posid, val );
	}
    }*/
}


}; //namespace
