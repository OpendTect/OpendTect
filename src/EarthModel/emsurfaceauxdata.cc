/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
#include "uistrings.h"
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
    auxdatatypes_.erase();

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


void SurfaceAuxData::setAuxDataType( int dataidx, AuxDataType type )
{
    if ( auxdatatypes_.validIdx(dataidx) )
	auxdatatypes_[dataidx] = type;
}


SurfaceAuxData::AuxDataType SurfaceAuxData::getAuxDataType( int dataidx ) const
{
    return auxdatatypes_.validIdx(dataidx) ? auxdatatypes_[dataidx] : NoType;
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


bool SurfaceAuxData::hasAuxDataName( const char* nm ) const
{ return auxDataIndex(nm) >= 0; }


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
    auxdatatypes_ += NoType;


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
    auxdatanames_.replace( dataidx, 0 );
    auxdatashift_[dataidx] = 0.0;
    auxdatatypes_[dataidx] = NoType;

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

    const BinID geomrc( posid.getRowCol() );
    const BinIDValueSet::SPos pos = auxdata_[sectionidx]->find( geomrc );
    if ( !pos.isValid() )
	return mUdf(float);

    return auxdata_[sectionidx]->getVals( pos )[dataidx];
}


void SurfaceAuxData::setAuxDataVal( int dataidx, const PosID& posid, float val )
{ setAuxDataVal( dataidx, posid, val, false ); }


void SurfaceAuxData::setAuxDataVal( int dataidx, const PosID& posid, float val,
				    bool onlynewpos )
{
    const TrcKey tk = BinID::fromInt64( posid.subID() );
    if ( !auxdatanames_.validIdx(dataidx) ||
	tk.isUdf() ||
	horizon_.isNodeLocked(tk) )
	return;

    const int sectionidx = horizon_.sectionIndex( posid.sectionID() );
    if ( sectionidx < 0 )
	return;

    if ( !auxdata_.validIdx(sectionidx) )
    {
	for ( int idx=auxdata_.size(); idx<horizon_.nrSections(); idx++ )
	    auxdata_ += 0;
    }

    if ( !auxdata_[sectionidx] )
	auxdata_.replace( sectionidx, new BinIDValueSet( nrAuxData(), false ) );

    const BinID geomrc( posid.getRowCol() );
    if ( geomrc.isUdf() )
	return;

    const BinIDValueSet::SPos pos = auxdata_[sectionidx]->find( geomrc );
    if ( !pos.isValid() )
    {
	mAllocVarLenArr( float, vals, auxdata_[sectionidx]->nrVals() );
	for ( int idx=0; idx<auxdata_[sectionidx]->nrVals(); idx++ )
	    vals[idx] = mUdf(float);

	vals[dataidx] = val;
	auxdata_[sectionidx]->add( geomrc, vals );
    }
    else if ( !onlynewpos )
	auxdata_[sectionidx]->getVals( pos )[dataidx] = val;

    changed_ = true;
}


void SurfaceAuxData::setAuxDataVal( int dataidx, const TrcKey& tk, float val )
{
    if ( auxdata_.isEmpty() || !auxdatanames_.validIdx(dataidx) )
	return;

    const BinIDValueSet::SPos pos = auxdata_[0]->find( tk.pos() );
    if ( pos.isValid() )
	auxdata_[0]->getVals( pos )[dataidx] = val;

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
    {
	horizon_.setErrMsg( uiStrings::sCantFindSurf() );
	return 0;
    }

    PtrMan<EMSurfaceTranslator> transl =
			(EMSurfaceTranslator*)ioobj->createTranslator();
    if ( !transl || !transl->startRead(*ioobj) )
    {
	horizon_.setErrMsg( transl ? transl->errMsg()
				   : tr("Cannot find Translator") );
	return 0;
    }

    SurfaceIODataSelection& sel = transl->selections();
    const int nrauxdata = sel.sd.valnames.size();
    if ( nrauxdata==0 || selidx >= nrauxdata ) return 0;

    return transl->getAuxdataReader( horizon_, selidx );
}


BufferString SurfaceAuxData::getFreeFileName( const IOObj& ioobj )
{
    PtrMan<StreamConn> conn =
	dynamic_cast<StreamConn*>(ioobj.getConn(Conn::Read));
    if ( !conn ) return 0;

    const int maxnrfiles = 100000; // just a big number to make this loop end
    for ( int idx=0; idx<maxnrfiles; idx++ )
    {
	BufferString fnm =
	    dgbSurfDataWriter::createHovName( conn->fileName(), idx );
	if ( !File::exists(fnm.buf()) )
	    return fnm;
    }

    return 0;
}


Executor* SurfaceAuxData::auxDataSaver( int dataidx, bool overwrite )
{
    PtrMan<IOObj> ioobj = IOM().get( horizon_.multiID() );
    if ( !ioobj )
    {
	horizon_.setErrMsg( uiStrings::sCantFindSurf() );
	return 0;
    }

    PtrMan<EMSurfaceTranslator> transl =
			(EMSurfaceTranslator*)ioobj->createTranslator();
    if ( transl && transl->startWrite(horizon_) )
    {
	PtrMan<Executor> exec = transl->writer( *ioobj, false );
	if ( exec )
	    return transl->getAuxdataWriter( horizon_, dataidx, overwrite );
    }

    horizon_.setErrMsg(
	transl ? transl->errMsg() : uiStrings::phrCannotFind(tr("Translator")));
    return 0;
}


void SurfaceAuxData::removeSection( const SectionID& sectionid )
{
    const int sectionidx = horizon_.sectionIndex( sectionid );
    if ( !auxdata_.validIdx( sectionidx ) )
	return;

    delete auxdata_.removeSingle( sectionidx );
}


bool SurfaceAuxData::hasAttribute( const IOObj& ioobj, const char* attrnm )
{ return !getFileName(ioobj,attrnm).isEmpty(); }

BufferString
    SurfaceAuxData::getFileName( const IOObj& ioobj, const char* attrnm )
{ return getFileName( ioobj.fullUserExpr(true), attrnm ); }


BufferString
    SurfaceAuxData::getFileName( const char* fulluserexp, const char* attrnmptr)
{
    FixedString attrnm( attrnmptr );
    const BufferString basefnm( fulluserexp );
    BufferString fnm; int gap = 0;
    for ( int idx=0; ; idx++ )
    {
	if ( gap > 100 ) return "";

	fnm = EM::dgbSurfDataWriter::createHovName(basefnm,idx);
	if ( File::isEmpty(fnm.buf()) )
	    { gap++; continue; }

	EM::dgbSurfDataReader rdr( fnm.buf() );
	if ( attrnm == rdr.dataName() )
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
	    const float val = getAuxDataVal( dataidx, posid );
	    arr->set( rowrg.getIndex(row), colrg.getIndex(col), val );
	}
    }

    return arr;
}


void SurfaceAuxData::init( int dataidx, float val )
{ init( dataidx, false, val ); }


void SurfaceAuxData::init( int dataidx, bool onlynewpos, float val )
{
    const SectionID sid = horizon_.sectionID( 0 );
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
	    if ( dataidx<0 )
	    {
		for ( int aidx=0; aidx<nrAuxData(); aidx++ )
		    setAuxDataVal( aidx, posid, val, onlynewpos );
	    }
	    else
		setAuxDataVal( dataidx, posid, val, onlynewpos );
	}
    }
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
