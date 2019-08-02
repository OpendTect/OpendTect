/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/

#include "emsurfaceauxdata.h"

#include "arrayndimpl.h"
#include "binnedvalueset.h"
#include "emhorizon3d.h"
#include "emsurfacegeometry.h"
#include "emsurfacetr.h"
#include "emsurfauxdataio.h"
#include "executor.h"
#include "file.h"
#include "ioobj.h"
#include "iopar.h"
#include "iostrm.h"
#include "parametricsurface.h"
#include "ptrman.h"
#include "uistrings.h"
#include "unitofmeasure.h"
#include "varlenarray.h"

namespace EM
{

SurfaceAuxData::SurfaceAuxData( Horizon3D& horizon )
    : horizon_(horizon)
    , changed_(false)
    , auxdata_(*new BinnedValueSet(0,false))
{
    units_.setNullAllowed( true );
}


SurfaceAuxData::~SurfaceAuxData()
{
    removeAll();
}


void SurfaceAuxData::removeAll()
{
    usable_.setEmpty();
    auxdatanames_.setEmpty();
    auxdatainfo_.setEmpty();
    auxdatafilenames_.setEmpty();
    auxdatashift_.setEmpty();
    auxdatatypes_.setEmpty();
    units_.setEmpty();
    auxdata_.setEmpty();
    changed_ = true;
}


int SurfaceAuxData::nrUsableAuxData() const
{
    int ret = 0;
    for ( bool usabl : usable_  )
	if ( usabl )
	    ret++;
    return ret;
}


bool SurfaceAuxData::isUsable( AuxID auxid ) const
{
    return usable_.validIdx(auxid) && usable_[auxid];
}


const char* SurfaceAuxData::auxDataName( AuxID auxid ) const
{
    return isUsable(auxid) ? auxdatanames_.get(auxid).str() : 0;;
}


void SurfaceAuxData::getUsableAuxDataNames( BufferStringSet& nms ) const
{
    for ( AuxID idx=0; idx<usable_.size(); idx++ )
	if ( usable_[idx] )
	    nms.add( auxDataName(idx) );
}


void SurfaceAuxData::setAuxDataName( AuxID auxid, const char* name )
{
    if ( isUsable(auxid) )
	auxdatanames_.get( auxid ).set( name );
}


SurfaceAuxData::AuxDataType SurfaceAuxData::getAuxDataType( AuxID auxid ) const
{
    return isUsable(auxid) ? auxdatatypes_[auxid] : NoType;
}


void SurfaceAuxData::setAuxDataType( AuxID auxid, AuxDataType type )
{
    if ( isUsable(auxid) )
	auxdatatypes_[auxid] = type;
}


const char* SurfaceAuxData::fileName( AuxID auxid ) const
{
    return auxdatafilenames_.validIdx(auxid)
	 ? auxdatafilenames_.get(auxid).str() : 0;
}


void SurfaceAuxData::setFileName( AuxID auxid, const char* fnm )
{
    if ( isUsable(auxid) )
	auxdatafilenames_.get(auxid) = fnm;
}


float SurfaceAuxData::auxDataShift( AuxID auxid ) const
{
    return isUsable(auxid) ? auxdatashift_[auxid] : 0.f;
}


void SurfaceAuxData::setAuxDataShift( AuxID auxid, float shift )
{
    if ( isUsable(auxid) )
	auxdatashift_[auxid] = shift;
}


const UnitOfMeasure* SurfaceAuxData::unit( AuxID auxid ) const
{
    return isUsable(auxid) ? units_[auxid] : 0;
}


BufferString SurfaceAuxData::unitSymbol( AuxID auxid ) const
{
    const UnitOfMeasure* uom = isUsable(auxid) ? units_[auxid] : 0;
    return BufferString( uom ? uom->symbol() : "" );
}


void SurfaceAuxData::setUnit( AuxID auxid, const UnitOfMeasure* uom )
{
    if ( isUsable(auxid) )
	units_.replace( auxid, uom );
}


bool SurfaceAuxData::hasAuxDataName( const char* nm ) const
{
    return auxDataIndex( nm ) >= 0;
}


SurfaceAuxData::AuxID SurfaceAuxData::auxDataIndex( const char* nm ) const
{
    AuxID id = auxdatanames_.indexOf( nm );
    if ( id >= 0 && !usable_[id] )
	id = -1;
    return id;
}


SurfaceAuxData::AuxID SurfaceAuxData::addAuxData( const char* name )
{
    usable_ += true;
    auxdatanames_.add( name );
    auxdatashift_ += 0.0;
    auxdatatypes_ += NoType;
    auxdatafilenames_.add( "" );
    units_ += 0;

    auxdata_.setNrVals( nrUsableAuxData() );

    changed_ = true;
    return usable_.size() - 1;
}


int SurfaceAuxData::getColIdx( AuxID auxid ) const
{
    int nrusable = 0;
    for ( int idx=0; idx<usable_.size(); idx++ )
    {
	if ( usable_[idx] )
	{
	    if ( idx == auxid )
		return nrusable;
	    nrusable++;
	}
    }
    return -1;
}


void SurfaceAuxData::removeAuxData( AuxID auxid )
{
    if ( !usable_.validIdx(auxid) )
	return;

    auxdata_.removeVal( getColIdx(auxid) );
    usable_[auxid] = false;
    auxdatanames_.get(auxid).setEmpty();

    changed_ = true;
}


float SurfaceAuxData::getAuxDataVal( AuxID auxid, const PosID& posid ) const
{
    if ( !isUsable(auxid) )
	return mUdf(float);

    const BinID geomrc( posid.getRowCol() );
    const BinnedValueSet::SPos spos = auxdata_.find( geomrc );
    if ( !spos.isValid() )
	return mUdf(float);

    return auxdata_.getVals( spos )[ getColIdx(auxid) ];
}


float SurfaceAuxData::getAuxDataVal( AuxID auxid, const TrcKey& tk ) const
{
    if ( !isUsable(auxid) )
	return mUdf(float);

    const BinnedValueSet::SPos spos = auxdata_.find( tk.binID() );
    if ( !spos.isValid() )
	return mUdf(float);

    return auxdata_.getVals( spos )[ getColIdx(auxid) ];
}


void SurfaceAuxData::setAuxDataVal( AuxID auxid, const PosID& posid, float val,
				    bool onlynewpos )
{
    const TrcKey tk = TrcKey( posid.getBinID() );
    if ( !isUsable(auxid) || tk.isUdf() || horizon_.isNodeLocked(tk) )
	return;
    const BinID geomrc( posid.getRowCol() );
    if ( geomrc.isUdf() )
	return;

    const BinnedValueSet::SPos pos = auxdata_.find( geomrc );
    const int colidx = getColIdx( auxid );
    if ( !pos.isValid() )
    {
	mAllocVarLenArr( float, vals, auxdata_.nrVals() );
	for ( int idx=0; idx<auxdata_.nrVals(); idx++ )
	    vals[idx] = mUdf(float);

	vals[colidx] = val;
	auxdata_.add( geomrc, vals );
	changed_ = true;
    }
    else if ( !onlynewpos )
    {
	auxdata_.getVals( pos )[ colidx ] = val;
	changed_ = true;
    }

}


void SurfaceAuxData::setAuxDataVal( AuxID auxid, const TrcKey& tk, float val )
{
    if ( !isUsable(auxid) || tk.isUdf() || horizon_.isNodeLocked(tk) )
	return;

    const BinnedValueSet::SPos pos = auxdata_.find( tk.binID() );
    if ( pos.isValid() )
    {
	auxdata_.getVals( pos )[ getColIdx(auxid) ] = val;
	changed_ = true;
    }

}


bool SurfaceAuxData::isChanged( AuxID ) const
{
    return changed_;
}


void SurfaceAuxData::resetChangedFlag()
{
    changed_ = false;
}


static EMSurfaceTranslator* getTranslator( Horizon& horizon )
{
    PtrMan<IOObj> ioobj = horizon.dbKey().getIOObj();
    if ( !ioobj )
	{ horizon.setErrMsg( uiStrings::phrCannotFindObjInDB() ); return 0; }

    EMSurfaceTranslator* transl =
			(EMSurfaceTranslator*)ioobj->createTranslator();
    if ( !transl || !transl->startRead(*ioobj) )
    {
	horizon.setErrMsg( transl ? transl->errMsg()
				  : toUiString("Cannot find Translator") );
	delete transl;
	return 0;
    }

    return transl;
}


Executor* SurfaceAuxData::auxDataLoader( int selidx )
{
    PtrMan<EMSurfaceTranslator> transl = getTranslator( horizon_ );
    if ( !transl )
	return 0;

    SurfaceIODataSelection& sel = transl->selections();
    const int nrauxdata = sel.sd.valnames.size();
    if ( nrauxdata==0 || selidx >= nrauxdata )
	return 0;

    return transl->getAuxdataReader( horizon_, selidx );
}


Executor* SurfaceAuxData::auxDataLoader( const char* nm )
{
    PtrMan<EMSurfaceTranslator> transl = getTranslator( horizon_ );
    if ( !transl )
	return 0;

    SurfaceIODataSelection& sel = transl->selections();
    const int selidx = sel.sd.valnames.indexOf( nm );
    if ( !sel.sd.valnames.validIdx(selidx) )
	return 0;

    return transl->getAuxdataReader( horizon_, selidx );
}


BufferString SurfaceAuxData::getFreeFileName( const IOObj& ioobj )
{
    PtrMan<StreamConn> conn =
	dynamic_cast<StreamConn*>(ioobj.getConn(Conn::Read));
    if ( !conn ) return 0;

    const int maxnrfiles = 100000; // just to be sure
    for ( int idx=0; idx<maxnrfiles; idx++ )
    {
	BufferString fnm =
	    dgbSurfDataWriter::createHovName( conn->fileName(), idx );
	if ( !File::exists(fnm.buf()) )
	    return fnm;
    }

    return 0;
}


Executor* SurfaceAuxData::auxDataSaver( AuxID auxid, bool overwrite )
{
    if ( !isUsable(auxid) )
    {
	const char* msg = "Attempt to use removed attribute";
	pErrMsg( msg );
	horizon_.setErrMsg( mINTERNAL(msg) );
	return 0;
    }

    PtrMan<IOObj> ioobj = horizon_.dbKey().getIOObj();
    if ( !ioobj )
	{ horizon_.setErrMsg( uiStrings::phrCannotFindObjInDB() ); return 0; }

    PtrMan<EMSurfaceTranslator> transl =
			(EMSurfaceTranslator*)ioobj->createTranslator();
    if ( transl && transl->startWrite(horizon_) )
    {
	PtrMan<Executor> exec = transl->writer( *ioobj, false );
	if ( exec )
	    return transl->getAuxdataWriter( horizon_, auxid, overwrite );
    }

    horizon_.setErrMsg(
	transl ? transl->errMsg()
	: uiStrings::phrCannotFind(uiStrings::sTranslator()));
    return 0;
}


bool SurfaceAuxData::hasAttribute( const IOObj& ioobj, const char* attrnm )
{ return !getFileName(ioobj,attrnm).isEmpty(); }

BufferString
    SurfaceAuxData::getFileName( const IOObj& ioobj, const char* attrnm )
{ return getFileName( ioobj.mainFileName(), attrnm ); }


BufferString
    SurfaceAuxData::getFileName( const char* fulluserexp, const char* attrnmptr)
{
    FixedString attrnm( attrnmptr );
    const BufferString basefnm( fulluserexp );
    BufferString fnm; int gap = 0;
    for ( int idx=0; ; idx++ )
    {
	if ( gap > 20 )
	    return "";

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
    PtrMan<IOObj> ioobj = horizon_.dbKey().getIOObj();
    return ioobj ? SurfaceAuxData::getFileName( *ioobj, attrnm ) : "";
}


bool SurfaceAuxData::removeFile( const char* attrnm ) const
{
    PtrMan<IOObj> ioobj = horizon_.dbKey().getIOObj();
    return ioobj ? SurfaceAuxData::removeFile( *ioobj, attrnm ) : false;
}


Array2D<float>* SurfaceAuxData::createArray2D( AuxID auxid ) const
{
    const auto& horgeom = horizon_.geometry();
    if ( horgeom.geometryElement()->isEmpty() )
	return 0;

    const StepInterval<int> rowrg = horgeom.rowRange();
    const StepInterval<int> colrg = horgeom.colRange( -1 );
    Array2DImpl<float>* arr =
	new Array2DImpl<float>( rowrg.nrSteps()+1, colrg.nrSteps()+1 );

    for ( int irow=rowrg.start; irow<=rowrg.stop; irow+=rowrg.step )
    {
	for ( int icol=colrg.start; icol<=colrg.stop; icol+=colrg.step )
	{
	    const PosID posid = PosID::getFromRowCol( irow, icol );
	    const float val = getAuxDataVal( auxid, posid );
	    arr->set( rowrg.getIndex(irow), colrg.getIndex(icol), val );
	}
    }

    return arr;
}


Interval<float> SurfaceAuxData::valRange( AuxID auxid, bool tosi ) const
{
    Interval<float> valrg( mUdf(float), mUdf(float) );
    const auto& horgeom = horizon_.geometry();
    if ( horgeom.geometryElement()->isEmpty() )
	return valrg;

    const StepInterval<int> rowrg = horgeom.rowRange();
    const StepInterval<int> colrg = horgeom.colRange( -1 );
    for ( int irow=rowrg.start; irow<=rowrg.stop; irow+=rowrg.step )
    {
	for ( int icol=colrg.start; icol<=colrg.stop; icol+=colrg.step )
	{
	    const PosID posid = PosID::getFromRowCol( irow, icol );
	    const float val = getAuxDataVal( auxid, posid );
	    if ( mIsUdf(val) )
		continue;
	    if ( mIsUdf(valrg.start) )
		valrg.start = valrg.stop = val;
	    else
		valrg.include( val, false );
	}
    }

    if ( tosi )
    {
	const auto* uom = unit( auxid );
	if ( uom )
	{
	    valrg.start = uom->getSIValue( valrg.start );
	    valrg.stop = uom->getSIValue( valrg.stop );
	}
    }

    return valrg;
}


void SurfaceAuxData::init( AuxID auxid, bool onlynewpos, float val )
{
    const Geometry::RowColSurface* rcgeom =
	horizon_.geometry().geometryElement();
    if ( !rcgeom || rcgeom->isEmpty() )
	return;

    const StepInterval<int> rowrg = rcgeom->rowRange();
    const StepInterval<int> colrg = rcgeom->colRange();
    PosID posid;
    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	{
	    posid = PosID::getFromRowCol( row, col );
	    if ( auxid<0 )
	    {
		for ( AuxID aidx=0; aidx<nrAuxData(); aidx++ )
		    setAuxDataVal( aidx, posid, val, onlynewpos );
	    }
	    else
		setAuxDataVal( auxid, posid, val, onlynewpos );
	}
    }
}


void SurfaceAuxData::setArray2D( AuxID auxid, const Array2D<float>& arr2d )
{
    const Geometry::RowColSurface* rcgeom =
	horizon_.geometry().geometryElement();
    if ( !rcgeom || rcgeom->isEmpty() )
	return;

    const StepInterval<int> rowrg = rcgeom->rowRange();
    const StepInterval<int> colrg = rcgeom->colRange();
    PosID posid;
    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	{
	    posid = PosID::getFromRowCol( row, col );
	    const float val = arr2d.get( rowrg.getIndex(row),
					 colrg.getIndex(col) );
	    setAuxDataVal( auxid, posid, val );
	}
    }
}


bool SurfaceAuxData::usePar( const IOPar& par )
{
    return true;
}

void SurfaceAuxData::fillPar( IOPar& par ) const
{
}

}; //namespace
