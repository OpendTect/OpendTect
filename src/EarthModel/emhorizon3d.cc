/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/

#include "emhorizon3d.h"
#include "emhorizonascio.h"
#include "emioobjinfo.h"

#include "arrayndalgo.h"
#include "array2dinterpol.h"
#include "arrayndimpl.h"
#include "ascstream.h"
#include "atomic.h"
#include "binidsurface.h"
#include "binnedvalueset.h"
#include "datapointset.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "emsurfaceauxdata.h"
#include "emsurfacetr.h"
#include "emundo.h"
#include "executor.h"
#include "horsubsel.h"
#include "ioobj.h"
#include "pickset.h"
#include "posprovider.h"
#include "ptrman.h"
#include "scaler.h"
#include "survgeom3d.h"
#include "survinfo.h"
#include "tabledef.h"
#include "threadwork.h"
#include "trigonometry.h"
#include "uistrings.h"
#include "unitofmeasure.h"
#include "zaxistransform.h"

#include <math.h>

namespace EM {


class AuxDataImporter : public Executor
{ mODTextTranslationClass(AuxDataImporter)
public:

AuxDataImporter( Horizon3D& hor, const ObjectSet<BinnedValueSet>& sects,
		 const BufferStringSet& attribnames, const int start,
		 TrcKeySampling hs )
    : Executor("Data Import")
    , horizon_(hor)
    , bvss_(sects)
    , startidx_(start)
    , totalnr_(0)
    , nrdone_(0)
    , hs_(hs)
    , inl_(0)
    , nrattribs_(-1)
{
    if ( bvss_.isEmpty() || attribnames.isEmpty() )
	{ msg_ = mINTERNAL("empty aux data input"); return; }
    nrattribs_ = attribnames.size();
    for ( int idx=0; idx<bvss_.size(); idx++ )
	totalnr_ += bvss_[idx]->nrInls();
    if ( totalnr_ < 1 )
	{ msg_ = uiStrings::sNoValidData(); return; }

    for ( int iattr=0; iattr<nrattribs_; iattr++ )
    {
	BufferString nm = attribnames.get( iattr );
	if ( nm.isEmpty() )
	    { nm = "Imported attribute "; nm += iattr + 1; }
	attrindexes_ += horizon_.auxdata.addAuxData( nm.buf() );
    }

    if ( nrattribs_ == 1 )
	{ msg_ = tr("Getting %1").arg(attribnames.get( 0 )); }
    else
	msg_ = tr("Getting horizon attribute values");
}


int nextStep()
{
    if ( nrattribs_ < 0 ) return ErrorOccurred();

    if ( !nrdone_ )
    {
	const Geometry::BinIDSurface* rcgeom =
			horizon_.geometry().geometryElement();
	if ( !rcgeom ) return ErrorOccurred();

	inlrg_ = rcgeom->rowRange();
	crlrg_ = rcgeom->colRange();
	inl_ = inlrg_.start;
    }

    const BinnedValueSet& bvs = *bvss_[0];
    for ( int crl=crlrg_.start; crl<=crlrg_.stop; crl+=crlrg_.step )
    {
	const BinID bid( inl_, crl );
	if ( !hs_.includes(bid) ) continue;

	BinnedValueSet::SPos pos = bvs.find( bid );
	if ( !pos.isValid() ) continue;

	const float* vals = bvs.getVals( pos );
	if ( !vals ) continue;
	const PosID posid = PosID::getFromRowCol( bid );
	for ( int iattr=0; iattr<nrattribs_; iattr++ )
	{
	    const float val = vals[iattr+startidx_];
	    if ( !mIsUdf(val) )
		horizon_.auxdata.setAuxDataVal( attrindexes_[iattr],
						posid, val );
	}
    }

    inl_ += inlrg_.step;
    nrdone_++;
    return MoreToDo();
}


uiString	message() const	{ return msg_; }
od_int64	totalNr() const		{ return totalnr_; }
od_int64	nrDone() const		{ return nrdone_; }
uiString	nrDoneText() const	{ return tr("Positions handled"); }

protected:

    const ObjectSet<BinnedValueSet>& bvss_;
    Horizon3D&			horizon_;
    const TrcKeySampling	hs_;
    uiString			msg_;
    int				nrattribs_;
    int				startidx_;
    TypeSet<int>		attrindexes_;

    int				inl_;
    StepInterval<int>		inlrg_;
    StepInterval<int>		crlrg_;

    int				totalnr_;
    int				nrdone_;
};


class HorizonImporter : public Executor
{ mODTextTranslationClass(HorizonImporter)
public:

HorizonImporter( Horizon3D& hor, const ObjectSet<BinnedValueSet>& sects,
		 const TrcKeySampling& hs )
    : Executor("Horizon Import")
    , horizon_(hor)
    , bvss_(sects)
    , totalnr_(0)
    , nrdone_(0)
    , hs_(hs)
    , sectionidx_(0)
    , nrvals_(-1)
    , msg_(tr("Adding nodes"))
{
    if ( bvss_.isEmpty() ) return;
    nrvals_ = bvss_[0]->nrVals();
    const RowCol step( hs_.step_.inl(), hs_.step_.crl() );
    horizon_.geometry().setStep( step, step );

    for ( int idx=0; idx<bvss_.size(); idx++ )
    {
	const BinnedValueSet& bvs = *bvss_[idx];
	if ( bvs.nrVals() != nrvals_ )
	    { msg_ = tr("Incompatible sections"); return; }

	totalnr_ += mCast( int, bvs.totalSize() );

	TrcKeySampling sectrg;
	sectrg.set( bvs.inlRange(), bvs.crlRange() );
	sectrg.step_ = BinID( step );
	sectrg.limitTo( hs_ );
	mDeclareAndTryAlloc( Array2D<float>*, arr,
		Array2DImpl<float>( sectrg.nrInl(), sectrg.nrCrl() ) );
	if ( arr && !arr->isEmpty() )
	{
	    arr->setAll( mUdf(float) );
	    horarrays_ += arr;
	}
	else
	    msg_ = tr("No valid positions");
    }

    horizon_.enableGeometryChecks( false );
}

~HorizonImporter()
{
    deepErase( horarrays_ );
}

uiString	message() const	{ return msg_; }
od_int64	totalNr() const		{ return totalnr_; }
od_int64	nrDone() const		{ return nrdone_; }
uiString	nrDoneText() const	{ return tr("Positions handled"); }

int nextStep()
{
    if ( nrvals_ == -1 || horarrays_.isEmpty() )
	return ErrorOccurred();

    if ( sectionidx_ >= bvss_.size() )
    {
	fillHorizonArray();
	horizon_.enableGeometryChecks( true );
	return Finished();
    }

    const BinnedValueSet& bvs = *bvss_[sectionidx_];
    BinID bid;
    for ( int idx=0; idx<10000; idx++ )
    {
	nrdone_++;
	if ( !bvs.next(pos_) )
	{
	    sectionidx_++;
	    pos_.reset();
	    return MoreToDo();
	}

	bvs.get( pos_, bid );
	if ( !hs_.includes(bid,true) || !horarrays_.validIdx(sectionidx_) )
	    continue;

	const int inlidx = hs_.inlIdx( bid.inl() );
	const int crlidx = hs_.crlIdx( bid.crl() );

	Array2D<float>* horarr = horarrays_[sectionidx_];
	if ( !horarr->info().validPos(inlidx,crlidx) )
	    continue;

	const float z = bvs.getVals(pos_)[ 0 ];
	horarr->set( inlidx, crlidx, z );
    }

    return MoreToDo();
}

void fillHorizonArray()
{
    Geometry::BinIDSurface* geom = horizon_.geometry().geometryElement();
    geom->setArray( hs_.start_, hs_.step_, horarrays_[0], true );
    horarrays_.erase();
}

protected:

    const ObjectSet<BinnedValueSet>&	bvss_;
    Horizon3D&				horizon_;
    BinnedValueSet::SPos		pos_;
    TrcKeySampling			hs_;
    uiString				msg_;
    int					nrvals_;

    ObjectSet<Array2D<float> > horarrays_;

    int			sectionidx_;
    int			totalnr_;
    int			nrdone_;
};


// EM::Horizon3D
Horizon3D::Horizon3D( const char* nm )
    : Horizon(nm)
    , geometry_(*this)
    , auxdata(*new SurfaceAuxData(*this))
    , lockednodes_(0)
    , parents_(0)
    , children_(0)
    , parentcolor_(Color::Yellow())
    , nodesource_( 0 )
    , arrayinited_( false )
{
}


Horizon3D::Horizon3D( const Horizon3D& oth )
    : Horizon(oth.name())
    , geometry_(*this)
    , auxdata(*new SurfaceAuxData(*this))
    , lockednodes_(0)
    , parents_(0)
    , children_(0)
    , parentcolor_(Color::Yellow())
    , nodesource_( 0 )
    , arrayinited_( false )
{
    *this = oth;
}


Horizon3D::~Horizon3D()
{
    delete &auxdata;
}


void Horizon3D::removeAll()
{
    Surface::removeAll();
    geometry_.removeAll();
    auxdata.removeAll();
}


void Horizon3D::fillPar( IOPar& par ) const
{
    Surface::fillPar( par );
    Horizon::fillPar( par );
    auxdata.fillPar( par );
}


bool Horizon3D::usePar( const IOPar& par )
{
    return Surface::usePar( par ) &&
	auxdata.usePar( par ) &&
	Horizon::usePar( par );
}


Horizon3DGeometry& Horizon3D::geometry()
{ return geometry_; }


const Horizon3DGeometry& Horizon3D::geometry() const
{ return geometry_; }


mImplementEMObjFuncs( Horizon3D,
				EMHorizon3DTranslatorGroup::sGroupName() )

mImplMonitorableAssignment(Horizon3D,Surface);


void Horizon3D::copyClassData( const Horizon3D& oth )
{
}

Monitorable::ChangeType Horizon3D::compareClassData(
					const Horizon3D& oth ) const
{ return cNoChange(); }

Horizon3D* Horizon3D::createWithConstZ( float z, const TrcKeySampling& hrg )
{
    Object* emobj = Hor3DMan().createTempObject( typeStr() );
    mDynamicCastGet(Horizon3D*,hor3d,emobj)
    if ( !hor3d ) return 0;

    Array2D<float>* array = new Array2DImpl<float>( hrg.nrInl(), hrg.nrCrl() );
    array->setAll( z );
    if ( !hor3d->setArray2D(array,hrg.start_,hrg.step_) )
    {
	delete array;
	hor3d->ref(); hor3d->unRef();
	return 0;
    }

    hor3d->setFullyLoaded( true );
    return hor3d;
}


void Horizon3D::setNodeSourceType( const TrcKey& tk, NodeSourceType type )
{
    if ( !nodesource_ || !trackingsamp_.includes(tk) ) return;

    nodesource_->getData()[trackingsamp_.globalIdx(tk)] = (char)type;
}


void Horizon3D::setNodeSourceType( const PosID& posid, NodeSourceType type )
{
    const TrcKey tk = geometry_.getTrcKey( posid );
    setNodeSourceType( tk, type );
}


bool Horizon3D::isNodeSourceType( const PosID& posid,
    NodeSourceType type ) const
{
    const TrcKey tk = geometry_.getTrcKey( posid );
    return !tk.isUdf() ? isNodeSourceType( tk, type ) : false;
}


bool Horizon3D::hasNodeSourceType( const PosID& posid ) const
{
    const TrcKey tk = geometry_.getTrcKey( posid );
    return nodesource_ ? nodesource_->getData()[tk.trcNr()] !=
	(char)NodeSourceType::None : false;
}



bool  Horizon3D::isNodeSourceType( const TrcKey& tk,
    NodeSourceType type ) const
{
    return nodesource_ ?
	nodesource_->getData()[trackingsamp_.globalIdx(tk)]
	== (char)type : false;
}


bool Horizon3D::setZ( const TrcKey& tk, float z, bool addtohist,
    NodeSourceType type )
{
    return setPos( PosID::getFromRowCol(tk.binID()),
		   Coord3(0,0,z), addtohist, type);
}


float Horizon3D::getZ( const TrcKey& tk ) const
{ return (float) getPos( PosID::getFromRowCol(tk.binID()) ).z_; }

bool Horizon3D::setZ( const BinID& bid, float z, bool addtohist )
{
    return setPos( PosID::getFromRowCol(bid), Coord3(0,0,z), addtohist );
}

float Horizon3D::getZ( const BinID& bid ) const
{ return (float) getPos( PosID::getFromRowCol(bid) ).z_; }

bool Horizon3D::hasZ( const TrcKey& tk ) const
{ return isDefined( PosID::getFromRowCol(tk.binID()) ); }

Coord3 Horizon3D::getCoord( const TrcKey& tk ) const
{ return getPos( PosID::getFromRowCol(tk.binID()) ); }


void Horizon3D::setAttrib( const TrcKey& tk, int attr, bool yn, bool addtohist )
{
    const PosID pid = PosID::getFromRowCol( tk.binID() );
    setPosAttrib( pid, attr, yn, addtohist );
}


bool Horizon3D::isAttrib( const TrcKey& tk, int attr ) const
{
    const PosID pid = PosID::getFromRowCol( tk.binID() );
    return isPosAttrib( pid, attr );
}


float Horizon3D::getZValue( const Coord& c, bool allow_udf, int nr ) const
{
    //TODO support the parameters
    return getZ( SI().transform(c) );
}


TrcKeySampling Horizon3D::range() const
{
    TrcKeySampling hs;
    hs.set( geometry().rowRange(), geometry().colRange(-1) );
    return hs;
}


Interval<float> Horizon3D::getZRange() const
{
    if ( isFullyLoaded() )
    {
	IOObjInfo ioobjinfo( dbKey() );
	return ioobjinfo.getZRange();
    }
    else
	return Interval<float>::udf();
}


Array2D<float>* Horizon3D::createArray2D(
				const ZAxisTransform* zaxistransform ) const
{
    const Geometry::BinIDSurface* geom = geometry_.geometryElement();
    if ( !geom || geom->isEmpty() )
	return 0;

    Array2DImpl<float>* arr = 0;
    if ( zaxistransform || !geom->getArray() )
    {
	const StepInterval<int> rowrg = geom->rowRange();
	const StepInterval<int> colrg = geom->colRange();

	arr = new Array2DImpl<float>( rowrg.nrSteps()+1, colrg.nrSteps()+1 );
	if ( arr && arr->isOK() )
	{
	    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
	    {
		for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
		{
		    const Coord3 pos = geom->getKnot( RowCol(row,col), false );
		    const float zval = zaxistransform->transform( pos );
		    arr->set( rowrg.getIndex(row), colrg.getIndex(col), zval );
		}
	    }
	}
    }
    else
	arr = new Array2DImpl<float>( *geom->getArray() );

    if ( arr && !arr->isOK() )
    {
	delete arr;
	arr = 0;
    }

    return arr;
}


bool Horizon3D::setArray2D( Array2D<float>* array, const BinID& start,
			    const BinID& step, bool takeover )
{
    removeAll();

    Geometry::BinIDSurface* geom = geometry().geometryElement();
    if ( !geom ) return false;

    geom->setArray( start, step, array, takeover );
    return true;
}


bool Horizon3D::setArray2D( const Array2D<float>& arr,
			    bool onlyfillundefs, const char* undodesc,
			    bool trimundefs )
{
    const Geometry::BinIDSurface* geom = geometry_.geometryElement();
    if ( !geom || geom->isEmpty() )
	return 0;

    const StepInterval<int> rowrg = geom->rowRange();
    const StepInterval<int> colrg = geom->colRange();

    const RowCol startrc( rowrg.start, colrg.start );
    const RowCol stoprc( rowrg.stop, colrg.stop );
    geometry().geometryElement()->expandWithUdf( BinID(startrc),
						 BinID(stoprc) );

    int poscount = 0;
    geometry().geometryElement()->blockCallBacks( true, false );
    const bool didcheck = geometry().enableChecks( false );
    setBurstAlert( true );

    Array2D<float>* oldarr = undodesc ? createArray2D( 0 ) : 0;

    const int arrnrrows = arr.getSize( 0 );
    const int arrnrcols = arr.getSize( 1 );

    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	{
	    const RowCol rc( row, col );
	    Coord3 pos = getPos( PosID::getFromRowCol(rc) );
	    if ( pos.isDefined() && onlyfillundefs )
		continue;

	    const int rowidx = rowrg.nearestIndex(row);
	    const int colidx = colrg.nearestIndex(col);

	    const double val = rowidx>=0 && rowidx<arrnrrows &&
			       colidx>=0 && colidx<arrnrcols
		   ? arr.get(rowrg.getIndex(row),colrg.getIndex(col))
		   : mUdf(float);

	    if ( pos.z_ == val || (mIsUdf(pos.z_) && mIsUdf(val)) )
		continue;

	    pos.z_ = val;
	    setPos( PosID::getFromRowCol(rc), pos, false );

	    if ( ++poscount >= 10000 )
	    {
		geometry().geometryElement()->blockCallBacks( true, true );
		poscount = 0;
	    }
	}
    }

    setBurstAlert(false);
    geometry().geometryElement()->blockCallBacks( false, true );
    geometry().enableChecks( didcheck );
    if ( trimundefs )
	geometry().geometryElement()->trimUndefParts();

    if ( oldarr )
    {
	UndoEvent* undo = new  SetAllHor3DPosUndoEvent( this, oldarr );
	Hor3DMan().undo(id()).addEvent( undo, undodesc );
    }

    return true;
}


const IOObjContext& Horizon3D::getIOObjContext() const
{ return EMHorizon3DTranslatorGroup::ioContext(); }


Executor* Horizon3D::importer( const ObjectSet<BinnedValueSet>& sections,
			   const TrcKeySampling& hs )
{
    removeAll();
    return new HorizonImporter( *this, sections, hs );
}


Executor* Horizon3D::auxDataImporter( const ObjectSet<BinnedValueSet>& sections,
				      const BufferStringSet& attribnms,
				      const int start, const TrcKeySampling& hs)
{
    return new AuxDataImporter( *this, sections, attribnms, start, hs );
}


void Horizon3D::initAllAuxData( float val )
{ auxdata.init( -1, false, val ); }


void Horizon3D::initTrackingAuxData( float val )
{
    BufferStringSet auxdatanames;
    auxdatanames.add( "Amplitude" ).add( "Correlation" )
		.add( "Seed Index" ).add("Tracking Order" );
    for ( int idx=0; idx<auxdatanames.size(); idx++ )
    {
	const char* nm = auxdatanames.get(idx).buf();
	int auxidx = -1;
	if ( !auxdata.hasAuxDataName(nm) )
	    auxidx = auxdata.addAuxData( nm );
	else
	    auxidx = auxdata.auxDataIndex( nm );

	auxdata.init( auxidx, true, val );
	auxdata.setAuxDataType( auxidx, SurfaceAuxData::Tracking );
    }
}


void Horizon3D::initTrackingArrays()
{
    const bool haspcd = readNodeArrays();
    if ( !haspcd )
    {
	updateTrackingSampling();
	return;
    }

    if ( !arrayinited_ )
	initNodeArraysSize(trackingsamp_.inlRange(),trackingsamp_.trcRange());

}


void Horizon3D::updateTrackingSampling()
{
    const TrcKeySampling curhs = getTrackingSampling();
    const TrcKeySampling hs = getTrcKeySampling();

    if ( hs == curhs || hs.isEmpty() )
	return;

    initTrackingAuxData();
    trackingsamp_ = hs;
    initNodeArraysSize( hs.inlRange(), hs.crlRange() );
}


TrcKeySampling Horizon3D::getTrcKeySampling() const
{
    TrcKeySampling hs;
    const Geometry::BinIDSurface* geom = geometry_.geometryElement();
    if ( !geom || geom->isEmpty() )
	return hs;

    const TrcKeySampling curhs = getTrackingSampling();
    hs.setLineRange( geom->rowRange() );
    hs.setTrcRange( geom->colRange() );
    hs.include( curhs, true );
    return hs;
}


bool Horizon3D::saveNodeArrays()
{
    if ( !parents_ || !nodesource_ || !lockednodes_ || !children_ )
	return true;

    const od_int64 totalsz = parents_->totalSize();
    if ( totalsz != nodesource_->totalSize()
      || totalsz != lockednodes_->totalSize()
      || totalsz != children_->totalSize() )
	return true;

    const od_int64* ptdata = parents_->getData();
    const char*	itpnodesdata = nodesource_->getData();
    const char* lckdata = lockednodes_->getData();
    const char* chddata = children_->getData();

    if ( totalsz<1 || !ptdata || !itpnodesdata || !lckdata || !children_ )
	return false;

    IOObjInfo ioobjinfo( dbKey() );
    if ( !ioobjinfo.ioObj() ) return false;

    od_ostream strm( getParentChildFileName(*ioobjinfo.ioObj()) );
    if ( !strm.isOK() ) return false;

    ascostream astream( strm );
    astream.putHeader( "Node Array Data" );

    IOPar par;
    trackingsamp_.fillPar( par );
    par.putTo( astream );

    for ( od_int64 idx=0; idx<totalsz; idx++ )
    {
	strm.addBin( ptdata[idx] );
	strm.addBin( itpnodesdata[idx] );
	strm.addBin( lckdata[idx] );
	strm.addBin( chddata[idx] );
    }

    strm.close();

    return true;
}


bool Horizon3D::readNodeArrays()
{
    IOObjInfo ioobjinfo( dbKey() );
    if ( !ioobjinfo.ioObj() ) return false;

    od_istream strm( getParentChildFileName(*ioobjinfo.ioObj()) );
    if ( !strm.isOK() ) return false;

    ascistream astream( strm );
    const char* oldheader = "Parent-Child Data";

    if ( astream.fileType() == oldheader )
	return readParentArray(); // old version

    const IOPar par( astream );
    trackingsamp_.usePar( par );

    delete parents_; parents_= 0;
    delete nodesource_; nodesource_= 0;
    delete lockednodes_; lockednodes_ = 0;
    delete children_; children_= 0;

    initNodeArraysSize( trackingsamp_.inlRange(), trackingsamp_.crlRange() );

    if ( !parents_ || !children_ || !lockednodes_ || !nodesource_ )
	return false;

    char* itpdata = nodesource_->getData();
    char* lckdata = lockednodes_->getData();
    char* chddata = children_->getData();
    od_int64* ptdata = parents_->getData();

    const od_int64 totalsz = parents_->totalSize();
    for ( od_int64 idx=0; idx<totalsz; idx++ )
    {
	strm.getBin( ptdata[idx] );
	strm.getBin( itpdata[idx] );
	strm.getBin( lckdata[idx] );
	strm.getBin( chddata[idx] );
	if ( lckdata[idx]=='1' )
	    haslockednodes_ = true;
    }

    return true;
}


bool Horizon3D::saveParentArray()
{
    if ( !parents_ ) return true;

    const od_int64 totalsz = parents_->totalSize();
    const od_int64* data = parents_->getData();
    if ( totalsz<1 || !data ) return false;

    IOObjInfo ioobjinfo( dbKey() );
    if ( !ioobjinfo.ioObj() ) return false;

    od_ostream strm( getParentChildFileName(*ioobjinfo.ioObj()) );
    if ( !strm.isOK() ) return false;

    ascostream astream( strm );
    astream.putHeader( "Parent-Child Data" );

    IOPar par;
    trackingsamp_.fillPar( par );
    par.putTo( astream );

    for ( od_int64 idx=0; idx<totalsz; idx++ )
	strm.addBin( data[idx] );

    strm.close();

    return true;
}


bool Horizon3D::readParentArray()
{
    IOObjInfo ioobjinfo( dbKey() );
    if ( !ioobjinfo.ioObj() ) return false;

    od_istream strm( getParentChildFileName(*ioobjinfo.ioObj()) );
    if ( !strm.isOK() ) return false;

    ascistream astream( strm );
    const IOPar par( astream );
    trackingsamp_.usePar( par );

    delete parents_;
    parents_ = new Array2DImpl<od_int64>( trackingsamp_.nrInl(),
					  trackingsamp_.nrCrl() );
    od_int64* data = parents_->getData();
    if ( !data )
    { delete parents_; parents_ = 0; return false; }

    const od_int64 totalsz = parents_->totalSize();
    for ( od_int64 idx=0; idx<totalsz; idx++ )
	strm.getBin( data[idx] );

    return true;
}


TrcKeySampling Horizon3D::getTrackingSampling() const
{ return trackingsamp_; }


void Horizon3D::setParent( const TrcKey& node, const TrcKey& parent )
{
    if ( !parents_ ) return;

    const od_int64 gidx = trackingsamp_.globalIdx( node );
    if ( gidx >= 0 && gidx < parents_->totalSize() )
	parents_->getData()[gidx] = trackingsamp_.globalIdx( parent );
}


TrcKey Horizon3D::getParent( const TrcKey& node ) const
{
    const od_int64 gidx = trackingsamp_.globalIdx( node );
    const od_int64 parentidx = parents_->getData()[gidx];
    if ( parentidx==-1 )
	return TrcKey::udf();

    return TrcKey( trackingsamp_.atIndex(parentidx) );
}


void Horizon3D::getParents( const TrcKey& node, TypeSet<TrcKey>& parents ) const
{
    if ( !parents_ || node.isUdf() ) return;

    od_int64 gidx = trackingsamp_.globalIdx( node );
    if ( gidx<0 || gidx>=parents_->totalSize() )
	return;

    while ( true )
    {
	gidx = parents_->getData()[gidx];
	if ( gidx==-1 || gidx>=parents_->totalSize() )
	    break;

	const TrcKey tk = TrcKey( trackingsamp_.atIndex(gidx) );
	if ( parents.isPresent(tk) )
	    break;

	parents.add( tk );
    }
}


void Horizon3D::initNodeArraysSize( const StepInterval<int>& inlrg,
    const StepInterval<int>& crlrg)
{
   setNodeArraySize( inlrg, crlrg, Parents );
   setNodeArraySize( inlrg, crlrg, LockNode );
   setNodeArraySize( inlrg, crlrg, NodeSource );
   setNodeArraySize( inlrg, crlrg, Children );
   arrayinited_ = true;
}


void Horizon3D::setNodeArraySize( const StepInterval<int>& inlrg,
    const StepInterval<int>& crlrg, ArrayType arrtype )
{
    const TrcKeySampling curhs = getTrackingSampling();
    TrcKeySampling hs;
    hs.setLineRange( inlrg );
    hs.setTrcRange( crlrg );
    hs.include( curhs, true );

    if ( arrtype == Parents )
    {
	if ( !parents_ )
	{
	    parents_ =
		new Array2DImpl<od_int64>(inlrg.nrSteps()+1,crlrg.nrSteps()+1);
	    parents_->setAll(-1);
	    return;
	}
	else if ( hs !=curhs )
	{
	    updateNodeSourceArray( hs, arrtype );
	}
	return;
    }

    const Array2D<char>* arr = getNodeSourceArray( arrtype );
    if ( !arr )
	createNodeSourceArray( inlrg, crlrg, arrtype );
    else if ( hs!=curhs )
	updateNodeSourceArray( hs, arrtype );
}


void Horizon3D::updateNodeSourceArray( const TrcKeySampling hs,
    ArrayType arrtype )
{
    const TrcKeySampling curhs = getTrackingSampling();
    if ( arrtype == Parents )
    {
	Array2DImpl<od_int64>* newparent =
	    new Array2DImpl<od_int64>( hs.nrLines(),hs.nrTrcs() );
	newparent->setAll( -1 );

	for ( od_int64 idx=0; idx<curhs.totalNr(); idx++ )
	{
	    const TrcKey curtk = curhs.trcKeyAt( idx );
	    od_int64 parentidx = parents_->getData()[idx];
	    if ( parentidx==-1 ) continue;

	    const TrcKey parenttk = curhs.trcKeyAt( parentidx );
	    parentidx = hs.globalIdx( parenttk );
	    const od_int64 newidx = hs.globalIdx( curtk );
	    newparent->getData()[newidx] = parentidx;
	}

	delete parents_;
	parents_ = newparent;
	return;
    }

    Array2DImpl<char>* newnodes =
	new Array2DImpl<char>( hs.nrLines(),hs.nrTrcs() );
    newnodes->setAll( (char)None );

    Array2D<char>*& arr = getNodeSourceArray( arrtype );
    CubeHorArrayCopier<char> nodescopier( *arr, *newnodes,
				     CubeHorSubSel(curhs),
				     CubeHorSubSel(hs) );
    if ( nodescopier.execute() )
    {
	delete arr;
	arr = newnodes;
    }
}


Array2D<char>*& Horizon3D::getNodeSourceArray( ArrayType arrtype )
{
    if ( arrtype== Children )
	return children_;
    else if ( arrtype== NodeSource )
	return nodesource_;

    if ( arrtype != LockNode )
	pErrMsg("Wrong ArrayType requested");

    return lockednodes_;
}


void Horizon3D::createNodeSourceArray( const StepInterval<int>& inlrg,
    const StepInterval<int>& crlrg, ArrayType arrtype )
{
    Array2D<char>* arr =
	new Array2DImpl<char>(inlrg.nrSteps()+1,crlrg.nrSteps()+1);
    arr->setAll((char)None);

    if ( arrtype== Children )
	children_ =  arr;
    else if ( arrtype== NodeSource )
	nodesource_ = arr;
    else if ( arrtype==LockNode )
	lockednodes_ = arr;
}


class FindTask : public Task
{
public:
FindTask( ChildFinder& finder, od_int64 pidx )
    : finder_(finder)
    , pidx_(pidx)
{
}


bool execute()
{
    TypeSet<od_int64> nbs;
    finder_.hs_.neighbors( pidx_, nbs );
    for ( int idx=0; idx<nbs.size(); idx++ )
    {
	const od_int64 childidx = nbs[idx];
	const od_int64 parent = finder_.parents_.getData()[childidx];
	if ( parent == pidx_ )
	{
	    finder_.children_.getData()[childidx] = '1';
	    finder_.addTask( childidx );
	}
    }

    return true;
}

    od_int64		pidx_;
    ChildFinder&	finder_;
};


ChildFinder::ChildFinder( const TrcKeySampling& hs,
			  const Array2D<od_int64>& parents,
			  Array2D<char>& children )
    : SequentialTask()
    , hs_(hs)
    , parents_(parents)
    , children_(children)
    , twm_(Threads::WorkManager::twm())
{
    queueid_ =
	twm_.addQueue( Threads::WorkManager::MultiThread, "Child Finder" );
    nrdone_ = 0;
    nrtodo_ = 0;
}


ChildFinder::~ChildFinder()
{
    twm_.removeQueue( queueid_, true );
}


void ChildFinder::addTask( od_int64 pidx )
{
    Threads::Locker locker( addlock_ );
    nrtodo_++;
    CallBack cb( mCB(this,ChildFinder,taskFinished) );
    Task* task = new FindTask( *this, pidx );
    twm_.addWork( Threads::Work(*task,true), &cb, queueid_,
		  false, false, true );
}


void ChildFinder::taskFinished( CallBacker* )
{
    Threads::Locker locker( finishlock_ );
    nrtodo_--;
    nrdone_++;
}


int ChildFinder::nextStep()
{
    return nrtodo_>0 ? MoreToDo() : Finished();
}


bool Horizon3D::selectChildren( const TrcKey& node )
{
    if ( !children_ ) return false;

    mLock4Write();
    children_->setAll( '0' );
    od_int64 gidx = trackingsamp_.globalIdx( node );
    ChildFinder cf( trackingsamp_, *parents_, *children_ );
    cf.addTask( gidx );
    const bool res = cf.execute();
    if ( res )
	mSendEMCBNotif( Object::cSelectionChange() );

    return res;
}


Array2D<char>* Horizon3D::getChildren() const
{ return children_; }

void Horizon3D::resetChildren()
{ if ( children_ ) children_->setAll( '0' ); }


void Horizon3D::deleteChildren()
{
    if ( !children_ ) return;

    const int prevevid = EM::Hor3DMan().undo(id()).currentEventID();

    Geometry::Element* ge = geometryElement();
    setBurstAlert( true );
    if ( ge ) ge->blockCallBacks( true, false );
    const od_int64 totalnr = trackingsamp_.totalNr();
    for ( od_int64 idx=0; idx<totalnr; idx++ )
    {
	if ( children_->getData()[idx] == '0' )
	    continue;

	const TrcKey& tk = TrcKey( trackingsamp_.atIndex(idx) );
	setZ( tk, mUdf(float), true );
    }
    if ( ge ) ge->blockCallBacks( false, true );
    setBurstAlert( false );

    resetChildren();

    const int evid = EM::Hor3DMan().undo(id()).currentEventID();
    if ( prevevid != evid )
	EM::Hor3DMan().undo(id()).setUserInteractionEnd( evid );
}


void Horizon3D::setNodeLocked( const TrcKey& node, bool locked )
{
    if ( !lockednodes_ || !trackingsamp_.includes(node) )
	return;

    const od_int64 pos = trackingsamp_.globalIdx( node );
#ifdef __debug__
    if ( !lockednodes_->getData() ||
	 pos < 0 || pos >= trackingsamp_.totalNr() ||
		    pos >= lockednodes_->totalSize() )
	pErrMsg("Invalid access");
#endif

    lockednodes_->getData()[pos] = locked ? '1' : '0';
}


bool Horizon3D::isNodeLocked( const PosID& posid )const
{
    const TrcKey tk = geometry_.getTrcKey( posid );
    return !tk.isUdf() ? isNodeLocked( tk ) : false;
}


bool Horizon3D::isNodeLocked( const TrcKey& node ) const
{
    if ( !lockednodes_ || !trackingsamp_.includes(node) )
	return false;

    const od_int64 pos = trackingsamp_.globalIdx( node );
#ifdef __debug__
    if ( !lockednodes_->getData() ||
	 pos < 0 || pos >= trackingsamp_.totalNr() ||
		    pos >= lockednodes_->totalSize() )
	pErrMsg("Invalid access");
#endif

    return lockednodes_->getData()[pos] == '1';
}


void Horizon3D::lockAll()
{
    if ( !lockednodes_ ) return;

    PtrMan<ObjectIterator> it = createIterator();
    if ( !it ) return;

    mLock4Write();
    while ( true )
    {
	const PosID pid = it->next();
	if ( pid.isInvalid() )
	    break;

	const Coord3 crd = getPos( pid );
	if ( !crd.isDefined() ) continue;

	const TrcKey tk = TrcKey( pid.getBinID() );
	setNodeLocked( tk, true );
    }

    haslockednodes_ = true;

    mSendEMCBNotif( cLockChange() );
}


void Horizon3D::unlockAll()
{
    if ( !lockednodes_ ) return;

    mLock4Write();
    haslockednodes_ = false;
    lockednodes_->setAll( '0' );

    mSendEMCBNotif( cLockChange() );
}


const Array2D<char>* Horizon3D::getLockedNodes() const
{ return lockednodes_; }


void Horizon3D::setParentColor( const Color& col )
{
    mLock4Write();
    parentcolor_ = col;
    mSendEMCBNotif( cParentColorChange() );
}


const Color& Horizon3D::getParentColor() const
{ return parentcolor_; }


bool Horizon3D::setPosition( const PosID& posid,
			const Coord3& crd, bool addtoundo,
			NodeSourceType type )
{
    const BinID bid = posid.getBinID();
    const TrcKey tk = TrcKey( bid );
    if ( !arrayinited_ )
    {
	const auto& geom3d = SurvGeom::get3D();
	StepInterval<int> inlrg = geom3d.inlRange();
	StepInterval<int> trcrg = geom3d.crlRange();
	initNodeArraysSize( inlrg, trcrg );
    }

    if ( isNodeLocked(tk) )
	return false;
    const NodeSourceType tp = crd.isDefined() ? type : None;
    setNodeSourceType( tk, tp );
    return Object::setPosition( posid, crd, addtoundo );
}


// Horizon3DGeometry
Horizon3DGeometry::Horizon3DGeometry( Surface& surf )
    : HorizonGeometry( surf )
    , step_( SI().inlStep(), SI().crlStep() )
    , loadedstep_( SI().inlStep(), SI().crlStep() )
    , checksupport_( true )
{}


const Geometry::BinIDSurface*
    Horizon3DGeometry::geometryElement() const
{
    return (const Geometry::BinIDSurface*)
				    SurfaceGeometry::geometryElement();
}


Geometry::BinIDSurface*
    Horizon3DGeometry::geometryElement()
{
    return (Geometry::BinIDSurface*) SurfaceGeometry::geometryElement();
}


RowCol Horizon3DGeometry::loadedStep() const
{
    return loadedstep_;
}


RowCol Horizon3DGeometry::step() const
{
    return step_;
}


void Horizon3DGeometry::setStep( const RowCol& ns, const RowCol& loadedstep )
{
    if ( ns.row() && ns.col() )
    {
	step_.row() = abs( ns.row() ); step_.col() = abs( ns.col() );
    }

    if ( loadedstep.row() && loadedstep.col() )
    {
	loadedstep_.row() = abs( loadedstep.row() );
	loadedstep_.col() = abs( loadedstep.col() );
    }
}


bool Horizon3DGeometry::isFullResolution() const
{
    return loadedstep_ == step_;
}


bool Horizon3DGeometry::enableChecks( bool yn )
{
    const bool res = checksupport_;
    for ( int idx=0; idx<sections_.size(); idx++ )
	((Geometry::BinIDSurface*)sections_[idx])->checkSupport(yn);

    checksupport_ = yn;

    return res;
}


bool Horizon3DGeometry::isChecksEnabled() const
{
    return checksupport_;
}


PosID Horizon3DGeometry::getPosID( const TrcKey& trckey ) const
{
    mDynamicCastGet(const EM::Horizon*, hor, &surface_ );
    if ( trckey.geomSystem()!=hor->geomSystem() )
	return PosID();

    return PosID::getFromRowCol( trckey.position() );
}


TrcKey Horizon3DGeometry::getTrcKey( const PosID& pid ) const
{
    mDynamicCastGet(const EM::Horizon*, hor, &surface_ );

    const RowCol rc = pid.getRowCol();
    return TrcKey( hor->geomSystem(), BinID(rc) );
}



bool Horizon3DGeometry::isNodeOK( const PosID& pid ) const
{
    const Geometry::BinIDSurface* surf = geometryElement();
    return surf ? surf->hasSupport( pid.getRowCol() ) : false;
}


bool Horizon3DGeometry::isAtEdge( const PosID& pid ) const
{
    if ( !surface_.isDefined( pid ) )
	return false;

    return !surface_.isDefined(getNeighbor(pid,RowCol(0,1))) ||
	   !surface_.isDefined(getNeighbor(pid,RowCol(1,0))) ||
	   !surface_.isDefined(getNeighbor(pid,RowCol(0,-1))) ||
	   !surface_.isDefined(getNeighbor(pid,RowCol(-1,0)));
}


PosID Horizon3DGeometry::getNeighbor( const PosID& posid,
				      const RowCol& dir ) const
{
    RowCol diff(0,0);
    if ( dir.row()>0 ) diff.row() = step_.row();
    else if ( dir.row()<0 ) diff.row() = -step_.row();

    if ( dir.col()>0 ) diff.col() = step_.col();
    else if ( dir.col()<0 ) diff.col() = -step_.col();

    TypeSet<PosID> aliases;
    getLinkedPos( posid, aliases );
    aliases += posid;

    const int nraliases = aliases.size();
    for ( int idx=0; idx<nraliases; idx++ )
    {
	const RowCol ownrc = aliases[idx].getRowCol();
	const RowCol neigborrc = ownrc+diff;
	if ( surface_.isDefined(PosID::getFromRowCol(neigborrc)) )
	    return PosID::getFromRowCol( neigborrc );
    }

    const RowCol ownrc = posid.getRowCol();
    const RowCol neigborrc = ownrc+diff;

    return PosID::getFromRowCol( neigborrc );
}


int Horizon3DGeometry::getConnectedPos( const PosID& posid,
					TypeSet<PosID>* res ) const
{
    int rescount = 0;
    const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();
    for ( int idx=dirs.size()-1; idx>=0; idx-- )
    {
	const PosID neighbor = getNeighbor( posid, dirs[idx] );
	if ( surface_.isDefined( neighbor ) )
	{
	    rescount++;
	    if ( res ) (*res) += neighbor;
	}
    }

    return rescount;
}


Geometry::BinIDSurface* Horizon3DGeometry::createGeometryElement() const
{
    Geometry::BinIDSurface* res = new Geometry::BinIDSurface(
					    BinID(loadedstep_) );
    res->checkSupport( checksupport_ );

    return res;
}


void Horizon3DGeometry::getDataPointSet( DataPointSet& dps, float shift ) const
{
    BinnedValueSet& bidvalset = dps.bivSet();
    bidvalset.setNrVals( 1 );
    const int nrknots = geometryElement()->nrKnots();
    for ( int idx=0; idx<nrknots; idx++ )
    {
	const BinID bid( geometryElement()->getKnotRowCol(idx) );
	Coord3 coord = geometryElement()->getKnot( bid, false );
	bidvalset.add( bid, (float)(coord.z_ + shift) );
    }
    dps.dataChanged();
}


bool Horizon3DGeometry::getBoundingPolygon( Pick::Set& set ) const
{
    set.setEmpty();
    const Geometry::BinIDSurface* surf = geometryElement();
    if ( !surf )
	return false;

    StepInterval<int> rowrg = rowRange();
    StepInterval<int> colrg = colRange( rowrg.start );
    PosID posid;
    bool nodefound = false;
    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	{
	    posid = PosID::getFromRowCol( row, col );
	    if ( isNodeOK(posid) && isAtEdge(posid) )
	    {
		nodefound = true;
		break;
	    }
	}

	if ( nodefound ) break;
    }

    if ( !nodefound )
	return false;

    const PosID firstposid = posid;
    while ( true )
    {
	Coord3 pos = surf->getKnot( posid.getRowCol(), false );
	set.add( Pick::Location(pos) );

	nodefound = false;
	const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();
	if ( dirs.size() != 8 ) return false;

	for ( int idx=0; idx<dirs.size(); idx++ )
	{
	    PosID curposid, leftposid, rightposid;
	    curposid = leftposid = rightposid = posid;
	    RowCol rcol = curposid.getRowCol();
	    RowCol currcol = rcol + dirs[idx];
	    int leftidx = idx ? idx - 1 : 7;
	    int rightidx = idx < 7 ? idx + 1 : 0;
	    RowCol leftrcol = rcol + dirs[leftidx];
	    RowCol rightrcol = rcol + dirs[rightidx];
	    curposid = PosID::getFromRowCol( currcol );
	    leftposid = PosID::getFromRowCol( leftrcol );
	    rightposid = PosID::getFromRowCol( rightrcol );
	    if ( !isAtEdge(curposid) )
		continue;

	    if ( !surface_.isDefined(leftposid) || !isNodeOK(leftposid) )
		continue;

	    if ( !surface_.isDefined(rightposid) || !isNodeOK(rightposid) )
	    {
		posid = curposid;
		nodefound = true;
		break;
	    }
	}

	if ( posid == firstposid || !nodefound || set.size() > 100000 )
	    break;
    }

    set.setConnection( Pick::Set::Disp::Close );
    return set.size() ? true : false;
}


void Horizon3DGeometry::fillBinnedValueSet( BinnedValueSet& bivs,
					   Pos::Provider3D* prov ) const
{
    PtrMan<ObjectIterator> it = createIterator();
    if ( !it ) return;

    while ( true )
    {
	const PosID pid = it->next();
	if ( pid.isInvalid() )
	    break;

	const Coord3 crd = surface_.getPos( pid );
	if ( crd.isDefined() )
	{
	    BinID bid = pid.getBinID();
	    const bool isinside = prov ? prov->includes( bid ) : true;
	    if ( isinside )
		bivs.add( bid, (float)crd.z_ );
	}
    }
}


ObjectIterator* Horizon3DGeometry::createIterator(
					const TrcKeyZSampling* cs) const
{
    if ( !cs )
	return new RowColIterator( surface_, cs );

    const StepInterval<int> rowrg = cs->hsamp_.inlRange();
    const StepInterval<int> colrg = cs->hsamp_.crlRange();
    return new RowColIterator( surface_, rowrg, colrg );
}


const char* Horizon3DAscIO::sKeyFormatStr()
{ return "Horizon3D"; }

const char* Horizon3DAscIO::sKeyAttribFormatStr()
{ return "Horizon3DAttributes"; }

Table::FormatDesc* Horizon3DAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Horizon3D" );
    fd->headerinfos_ += new Table::TargetInfo( uiStrings::sUndefVal(),
			StringInpSpec(sKey::FloatUdf()), Table::Required );
    BufferStringSet attrnms;
    createDescBody( fd, attrnms );
    return fd;
}


void Horizon3DAscIO::createDescBody( Table::FormatDesc* fd,
				     const BufferStringSet& attrnms )
{
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true, true, false );

    for ( int idx=0; idx<attrnms.size(); idx++ )
    {
	const BufferString fldname = attrnms.get( idx );
	Table::TargetInfo* ti;
	if ( fldname == "Z values" )
	    ti = Table::TargetInfo::mkZPosition( true );
	else
	    ti = new Table::TargetInfo( toUiString(fldname), FloatInpSpec(),
					Table::Required );
	fd->bodyinfos_ += ti;
    }
}


void Horizon3DAscIO::updateDesc( Table::FormatDesc& fd,
				 const BufferStringSet& attrnms )
{
    fd.bodyinfos_.erase();
    createDescBody( &fd, attrnms );
}


#define mErrRet(s) { if ( s ) errmsg_ = s; return 0; }

bool Horizon3DAscIO::isXY() const
{
    return formOf( false, 0 ) == 0;
}


int Horizon3DAscIO::getNextLine( Coord& pos, TypeSet<float>& data )
{
    data.erase();
    if ( !finishedreadingheader_ )
    {
	if ( !getHdrVals(strm_) )
	    return -1;

	udfval_ = getFValue( 0 );
	finishedreadingheader_ = true;
    }

    const int ret = getNextBodyVals( strm_ );
    const int nrattribs = fd_.bodyinfos_.size() - 1;
    if ( ret <= 0 || nrattribs < 1 )
	return ret;
    pos = getPos( 0, 1, udfval_ );
    for ( int idx=0; idx<nrattribs; idx++ )
	data += getFValue( idx+2, udfval_ );

    return ret;
}


} // namespace EM
