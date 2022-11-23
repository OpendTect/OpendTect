/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
#include "posfilter.h"
#include "ptrman.h"
#include "survinfo.h"
#include "uistrings.h"
#include "varlenarray.h"

namespace EM
{

SurfaceAuxData::SurfaceAuxData( Horizon3D& horizon )
    : horizon_(horizon)
    , changed_(false)
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
    auxdatanames_.setEmpty();
    auxdatainfo_.setEmpty();
    auxdatashift_.erase();
    auxdatatypes_.erase();

    deepErase( auxdata_ );
    changed_ = true;
}


bool SurfaceAuxData::validIdx( int idx ) const
{
    return auxdatanames_.validIdx( idx );
}


int SurfaceAuxData::nrAuxData() const
{ return auxdatanames_.size(); }


const char* SurfaceAuxData::auxDataName( int dataidx ) const
{
    if ( nrAuxData() && auxdatanames_[dataidx] )
	return auxdatanames_[dataidx]->buf();

    return nullptr;
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

    init( auxdatanames_.size()-1, false );
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
    auxdatanames_.replace( dataidx, nullptr );
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

    const int sectionidx = 0;
    if ( !auxdata_.validIdx(sectionidx) || !auxdata_[sectionidx] )
	return mUdf(float);

    const BinID geomrc( posid.getRowCol() );
    const BinIDValueSet::SPos pos = auxdata_[sectionidx]->find( geomrc );
    if ( !pos.isValid() )
	return mUdf(float);

    return auxdata_[sectionidx]->getVals( pos )[dataidx];
}


float SurfaceAuxData::getAuxDataVal( int dataidx, const TrcKey& tk ) const
{
    return getAuxDataVal( dataidx, tk.position() );
}


float SurfaceAuxData::getAuxDataVal( int dataidx, const BinID& bid ) const
{
    if ( !auxdatanames_.validIdx(dataidx) ||
	 !auxdata_.validIdx(0) || !auxdata_[0] )
	return mUdf(float);

    const BinIDValueSet::SPos pos = auxdata_[0]->find( bid );
    if ( !pos.isValid() )
	return mUdf(float);

    return auxdata_[0]->getVals( pos )[dataidx];
}


void SurfaceAuxData::setAuxDataVal( int dataidx, const PosID& posid, float val )
{ setAuxDataVal( dataidx, posid, val, false ); }


void SurfaceAuxData::setAuxDataVal( int dataidx, const PosID& posid, float val,
				    bool onlynewpos )
{
    const TrcKey tk = TrcKey( BinID::fromInt64( posid.subID() ) );
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
	    auxdata_ += nullptr;
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


void SurfaceAuxData::setAuxDataVal( int dataidx, const BinID& bid, float val )
{
    if ( auxdata_.isEmpty() || !auxdatanames_.validIdx(dataidx) )
	return;

    const BinIDValueSet::SPos pos = auxdata_[0]->find( bid );
    if ( pos.isValid() )
	auxdata_[0]->getVals( pos )[dataidx] = val;

    changed_ = true;
}


void SurfaceAuxData::setAuxDataVal( int dataidx, const TrcKey& tk, float val )
{
    setAuxDataVal( dataidx, tk.position(), val );
}


bool SurfaceAuxData::isChanged( int ) const
{ return changed_; }


void SurfaceAuxData::resetChangedFlag()
{
    changed_ = false;
}


static EMSurfaceTranslator* getTranslator( Horizon& horizon )
{
    PtrMan<IOObj> ioobj = IOM().get( horizon.multiID() );
    if ( !ioobj )
    {
	horizon.setErrMsg( uiStrings::sCantFindSurf() );
	return nullptr;
    }

    EMSurfaceTranslator* transl =
			(EMSurfaceTranslator*)ioobj->createTranslator();
    if ( !transl || !transl->startRead(*ioobj) )
    {
	horizon.setErrMsg( transl ? transl->errMsg()
				  : toUiString("Cannot find Translator") );
	delete transl;
	return nullptr;
    }

    return transl;
}


Executor* SurfaceAuxData::auxDataLoader( int selidx )
{
    PtrMan<EMSurfaceTranslator> transl = getTranslator( horizon_ );
    if ( !transl )
	return nullptr;

    SurfaceIODataSelection& sel = transl->selections();
    const int nrauxdata = sel.sd.valnames.size();
    if ( nrauxdata==0 || selidx >= nrauxdata )
	return nullptr;

    return transl->getAuxdataReader( horizon_, selidx );
}


Executor* SurfaceAuxData::auxDataLoader( const char* nm )
{
    PtrMan<EMSurfaceTranslator> transl = getTranslator( horizon_ );
    if ( !transl )
	return nullptr;

    SurfaceIODataSelection& sel = transl->selections();
    const int selidx = sel.sd.valnames.indexOf( nm );
    if ( !sel.sd.valnames.validIdx(selidx) )
	return nullptr;

    return transl->getAuxdataReader( horizon_, selidx );
}


BufferString SurfaceAuxData::getFreeFileName( const IOObj& ioobj )
{
    PtrMan<StreamConn> conn =
	dynamic_cast<StreamConn*>(ioobj.getConn(Conn::Read));
    if ( !conn )
	return nullptr;

    const int maxnrfiles = 100000; // just a big number to make this loop end
    for ( int idx=0; idx<maxnrfiles; idx++ )
    {
	BufferString fnm =
	    dgbSurfDataWriter::createHovName( conn->fileName(), idx );
	if ( !File::exists(fnm.buf()) )
	    return fnm;
    }

    return nullptr;
}


Executor* SurfaceAuxData::auxDataSaver( int dataidx, bool overwrite )
{
    PtrMan<IOObj> ioobj = IOM().get( horizon_.multiID() );
    if ( !ioobj )
    {
	horizon_.setErrMsg( uiStrings::sCantFindSurf() );
	return nullptr;
    }

    PtrMan<EMSurfaceTranslator> transl =
		sCast(EMSurfaceTranslator*,ioobj->createTranslator());
    if ( transl && transl->startWrite(horizon_) )
    {
	PtrMan<Executor> exec = transl->writer( *ioobj, false );
	if ( exec )
	    return transl->getAuxdataWriter( horizon_, dataidx, overwrite );
    }

    horizon_.setErrMsg(
	transl ? transl->errMsg() : uiStrings::phrCannotFind(tr("Translator")));
    return nullptr;
}


void SurfaceAuxData::removeSection( const SectionID& sectionid )
{
    const int sectionidx = horizon_.sectionIndex( sectionid );
    if ( !auxdata_.validIdx( sectionidx ) )
	return;

    delete auxdata_.removeSingle( sectionidx );
}


bool SurfaceAuxData::hasAttribute( const IOObj& ioobj, const char* attrnm )
{
    return !getFileName(ioobj,attrnm).isEmpty();
}


BufferString SurfaceAuxData::getFileName( const IOObj& ioobj,
					  const char* attrnm )
{ return getFileName( ioobj.fullUserExpr(true), attrnm ); }


BufferString SurfaceAuxData::getFileName( const char* fulluserexp,
					  const char* attrnmptr)
{
    StringView attrnm( attrnmptr );
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
    return ioobj ? SurfaceAuxData::getFileName( *ioobj, attrnm )
		 : BufferString::empty();
}


bool SurfaceAuxData::removeFile( const char* attrnm ) const
{
    PtrMan<IOObj> ioobj = IOM().get( horizon_.multiID() );
    return ioobj ? SurfaceAuxData::removeFile( *ioobj, attrnm ) : false;
}


Array2D<float>* SurfaceAuxData::createArray2D( int dataidx ) const
{
    if ( horizon_.geometry().geometryElement()->isEmpty() )
	return nullptr;

    const StepInterval<int> rowrg = horizon_.geometry().rowRange();
    const StepInterval<int> colrg = horizon_.geometry().colRange( -1 );

    PosID posid( horizon_.id() );
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
    const Geometry::RowColSurface* rcgeom =
	horizon_.geometry().geometryElement();
    if ( !rcgeom || rcgeom->isEmpty() )
	return;

    const StepInterval<int> rowrg = rcgeom->rowRange();
    const StepInterval<int> colrg = rcgeom->colRange();
    PosID posid( horizon_.id() );
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


void SurfaceAuxData::setArray2D( int dataidx,
				 const Array2D<float>& arr2d,
				 const TrcKeySampling* arrtks )
{
    TrcKeySampling tks;
    if ( arrtks )
	tks = *arrtks;
    else
    {
	const Geometry::RowColSurface* rcgeom =
		horizon_.geometry().geometryElement();
	if ( !rcgeom || rcgeom->isEmpty() )
	    return;

	tks.set( rcgeom->rowRange(), rcgeom->colRange() );
    }

    PosID posid( horizon_.id() );
    for ( od_int64 gidx=0; gidx<tks.totalNr(); gidx++ )
    {
	const TrcKey tk = tks.trcKeyAt( gidx );
	float val = mUdf(float);
	if ( arr2d.getData() )
	    val = arr2d.getData()[gidx];
	else
	    val = arr2d.get( tks.inlIdx(tk.inl()), tks.crlIdx(tk.crl()) );

	posid.setSubID( tk.binID().toInt64() );
	setAuxDataVal( dataidx, posid, val );
    }
}


bool SurfaceAuxData::usePar( const IOPar& )
{ return true; }


void SurfaceAuxData::fillPar( IOPar& ) const
{}


void SurfaceAuxData::applyPosFilter( const Pos::Filter& pf, int dataidx )
{
    for ( int sidx=0; sidx<auxdata_.size(); sidx++ )
    {
	BinIDValueSet* bvs = auxdata_[sidx];
	if ( !bvs || bvs->nrVals()==0 )
	    continue;

	BinIDValueSet::SPos spos;
	BinID bid; float zval;
	while ( bvs->next(spos) )
	{
	    bid = bvs->getBinID( spos );
	    zval = horizon_.getZ( bid );
	    if ( pf.includes(SI().transform(bid),zval) )
		continue;

	    float* vals = bvs->getVals( spos );
	    for ( int vidx=0; vidx<bvs->nrVals(); vidx++ )
	    {
		if ( dataidx==-1 || vidx==dataidx )
		    vals[vidx] = mUdf(float);
	    }
	}
    }
}

} // namespace EM
