/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "emsurfaceauxdata.h"

#include "arrayndimpl.h"
#include "binidvalset.h"
#include "emhorizon3d.h"
#include "emsurfacegeometry.h"
#include "emsurfacetr.h"
#include "emsurfauxdataio.h"
#include "executor.h"
#include "file.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "iostrm.h"
#include "parametricsurface.h"
#include "ptrman.h"
#include "settings.h"
#include "strmprov.h"
#include "varlenarray.h"

namespace EM
{

SurfaceAuxData::SurfaceAuxData( Horizon3D& horizon )
    : horizon_( horizon )
    , changed_( 0 )
{
    auxdatanames_.allowNull(true);
    auxdatainfo_.allowNull(true);
    auxdata_.allowNull(true);
}


SurfaceAuxData::~SurfaceAuxData()
{
    removeAll();
}


void SurfaceAuxData::removeAll()
{
    deepErase( auxdatanames_ );
    deepErase( auxdatainfo_ );
    auxdatashift_.erase();

    deepErase( auxdata_ );
    changed_ = true;
}


int SurfaceAuxData::nrAuxData() const
{ return auxdatanames_.size(); }


const char* SurfaceAuxData::auxDataName( int dataidx ) const
{
    if ( nrAuxData() && auxdatanames_[dataidx] )
	return auxdatanames_[dataidx]->buf();

    return 0;
}


float SurfaceAuxData::auxDataShift( int dataidx ) const
{ return auxdatashift_[dataidx]; }


void SurfaceAuxData::setAuxDataName( int dataidx, const char* name )
{
    if ( auxdatanames_[dataidx] )
	auxdatanames_.replace( dataidx, new BufferString(name) );
}


void SurfaceAuxData::setAuxDataShift( int dataidx, float shift )
{
    if ( auxdatanames_[dataidx] )
	auxdatashift_[dataidx] = shift;
}


int SurfaceAuxData::auxDataIndex( const char* nm ) const
{
    for ( int idx=0; idx<auxdatanames_.size(); idx++ )
	if ( auxdatanames_[idx] && auxdatanames_.get(idx) == nm )
	    return idx;
    return -1;
}


int SurfaceAuxData::addAuxData( const char* name )
{
    auxdatanames_.add( name );
    auxdatashift_ += 0.0;

    for ( int idx=0; idx<auxdata_.size(); idx++ )
    {
	if ( auxdata_[idx] )
	    auxdata_[idx]->setNrVals( nrAuxData(), true );
    }

    changed_ = true;
    return auxdatanames_.size()-1;
}


void SurfaceAuxData::removeAuxData( int dataidx )
{
    delete auxdatanames_[dataidx];
    auxdatanames_.replace( dataidx, 0 );
    auxdatashift_[dataidx] = 0.0;

    for ( int idx=0; idx<auxdata_.size(); idx++ )
    {
	if ( auxdata_[idx] )
	    auxdata_[idx]->removeVal( dataidx );
    }

    changed_ = true;
}


float SurfaceAuxData::getAuxDataVal( int dataidx, const PosID& posid ) const
{
    if ( !auxdatanames_.validIdx(dataidx) )
	return mUdf(float);

    const int sectionidx = horizon_.sectionIndex( posid.sectionID() );
    if ( !auxdata_.validIdx(sectionidx) || !auxdata_[sectionidx] )
	return mUdf(float);

    const BinID geomrc( RowCol(posid.subID()) );
    const BinIDValueSet::Pos pos = auxdata_[sectionidx]->findFirst( geomrc );
    if ( !pos.valid() )
	return mUdf(float);

    return auxdata_[sectionidx]->getVals( pos )[dataidx];
}


void SurfaceAuxData::setAuxDataVal( int dataidx, const PosID& posid, float val)
{
    if ( !auxdatanames_.validIdx(dataidx) )
	return;

    const int sectionidx = horizon_.sectionIndex( posid.sectionID() );
    if ( !auxdata_.validIdx(sectionidx) )
    {
	for ( int idx=auxdata_.size(); idx<horizon_.nrSections(); idx++ )
	{
	    auxdata_ += 0;
	}
    }
    
    if ( !auxdata_[sectionidx] )
	auxdata_.replace( sectionidx, new BinIDValueSet( nrAuxData(), false ) );

    const BinID geomrc( RowCol(posid.subID()) );
    const BinIDValueSet::Pos pos = auxdata_[sectionidx]->findFirst( geomrc );
    if ( !pos.valid() )
    {
	mAllocVarLenArr( float, vals, auxdata_[sectionidx]->nrVals() );
	for ( int idx=0; idx<auxdata_[sectionidx]->nrVals(); idx++ )
	    vals[idx] = mUdf(float);

	vals[dataidx] = val;
	auxdata_[sectionidx]->add( geomrc, vals );
    }
    else
    {
	auxdata_[sectionidx]->getVals( pos )[dataidx] = val;
    }

    changed_ = true;
}


bool SurfaceAuxData::isChanged(int idx) const
{ return changed_; }


void SurfaceAuxData::resetChangedFlag()
{
    changed_ = false;
}


Executor* SurfaceAuxData::auxDataLoader( int selidx )
{
    PtrMan<IOObj> ioobj = IOM().get( horizon_.multiID() );
    if ( !ioobj )
	{ horizon_.errmsg_ = "Cannot find surface"; return 0; }

    PtrMan<EMSurfaceTranslator> tr = 
			(EMSurfaceTranslator*)ioobj->getTranslator();
    if ( !tr || !tr->startRead(*ioobj) )
    { horizon_.errmsg_ = tr ? tr->errMsg() : "Cannot find Translator";return 0;}

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
	rdr->setSurface( horizon_ );
	grp->add( rdr );
    }

    return grp;
}


BufferString SurfaceAuxData::getFreeFileName( const IOObj& ioobj )
{
    PtrMan<StreamConn> conn =
	dynamic_cast<StreamConn*>(ioobj.getConn(Conn::Read));
    if ( !conn ) return 0;

    BufferString fnm;
    for ( int idx=0; ; idx++ )
    {
	fnm = dgbSurfDataWriter::createHovName( conn->fileName(), idx );
	if ( !File::exists(fnm.buf()) )
	    return fnm;
    }

    return 0;
}


Executor* SurfaceAuxData::auxDataSaver( int dataidx, bool overwrite )
{
    PtrMan<IOObj> ioobj = IOM().get( horizon_.multiID() );
    if ( !ioobj )
	{ horizon_.errmsg_ = "Cannot find surface"; return 0; }
    bool binary = true;
    mSettUse(getYN,"dTect.Surface","Binary format",binary);

    BufferString fnm;
    if ( overwrite )
    {
	if ( dataidx<0 ) dataidx = 0;
	fnm = getFileName( *ioobj, auxDataName(dataidx) );
	if ( !fnm.isEmpty() )
	    return new dgbSurfDataWriter(horizon_,dataidx,0,binary,fnm.buf());
    }

    ExecutorGroup* grp = new ExecutorGroup( "Surface attributes saver" );
    grp->setNrDoneText( "Nr done" );
    for ( int selidx=0; selidx<nrAuxData(); selidx++ )
    {
	if ( dataidx >= 0 && dataidx != selidx ) continue;
	fnm = getFreeFileName( *ioobj );
	Executor* exec =
	    new dgbSurfDataWriter(horizon_,selidx,0,binary,fnm.buf());
	grp->add( exec );
    }

    return grp;
}


void SurfaceAuxData::removeSection( const SectionID& sectionid )
{
    const int sectionidx = horizon_.sectionIndex( sectionid );
    if ( !auxdata_.validIdx( sectionidx ) )
	return;

    delete auxdata_.remove( sectionidx );
}


BufferString SurfaceAuxData::getFileName( const IOObj& ioobj,
					  const char* attrnm )
{
    return getFileName( ioobj.fullUserExpr(true), attrnm );
}


BufferString SurfaceAuxData::getFileName( const char* fulluserexp,
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


bool SurfaceAuxData::removeFile( const IOObj& ioobj, const char* attrnm )
{
    const BufferString fnm = getFileName( ioobj, attrnm );
    return !fnm.isEmpty() ? File::remove( fnm ) : false;
}
 

BufferString SurfaceAuxData::getFileName( const char* attrnm ) const
{
    PtrMan<IOObj> ioobj = IOM().get( horizon_.multiID() );
    return ioobj ? SurfaceAuxData::getFileName( *ioobj, attrnm ) : "";
}


bool SurfaceAuxData::removeFile( const char* attrnm ) const
{
    PtrMan<IOObj> ioobj = IOM().get( horizon_.multiID() );
    return ioobj ? SurfaceAuxData::removeFile( *ioobj, attrnm ) : false;
}


Array2D<float>* SurfaceAuxData::createArray2D( int dataidx, SectionID sid) const
{
    if ( horizon_.geometry().sectionGeometry( sid )->isEmpty() )
	return 0;

    const StepInterval<int> rowrg = horizon_.geometry().rowRange( sid );
    const StepInterval<int> colrg = horizon_.geometry().colRange( sid, -1 );

    PosID posid( horizon_.id(), sid );
    Array2DImpl<float>* arr =
	new Array2DImpl<float>( rowrg.nrSteps()+1, colrg.nrSteps()+1 );
    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	{
	    posid.setSubID( RowCol(row,col).toInt64() );
	    const float val = getAuxDataVal( dataidx, posid);
	    arr->set( rowrg.getIndex(row), colrg.getIndex(col), val );
	}
    }

    return arr;
}


void SurfaceAuxData::setArray2D( int dataidx, SectionID sid,
				 const Array2D<float>& arr2d )
{
    const Geometry::RowColSurface* rcgeom =
	horizon_.geometry().sectionGeometry( sid );
    if ( !rcgeom || rcgeom->isEmpty() )
	return;

    const StepInterval<int> rowrg = rcgeom->rowRange();
    const StepInterval<int> colrg = rcgeom->colRange();
    PosID posid( horizon_.id(), sid );
    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	{
	    posid.setSubID( RowCol(row,col).toInt64() );
	    const float val = arr2d.get( rowrg.getIndex(row),
		    			 colrg.getIndex(col) );
	    setAuxDataVal( dataidx, posid, val );
	}
    }
}


bool SurfaceAuxData::usePar( const IOPar& par )
{ return true; }

void SurfaceAuxData::fillPar( IOPar& par ) const
{}

}; //namespace
