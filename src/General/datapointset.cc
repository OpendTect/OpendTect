/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Jan 2005
-*/


#include "datapointset.h"

#include "arrayndimpl.h"
#include "bufstringset.h"
#include "datacoldef.h"
#include "executor.h"
#include "idxable.h"
#include "iopar.h"
#include "keystrs.h"
#include "od_ostream.h"
#include "posprovider.h"
#include "posvecdataset.h"
#include "samplfunc.h"
#include "statrand.h"
#include "survinfo.h"
#include "trckeyzsampling.h"

mUseType( Pos, GeomID );
static const char* sKeyDPS = "Data Point Set";
const int DataPointSet::groupcol_ = 3;
#define mAddMembs(is2d,mini) \
	  is2d_(is2d) \
	, nrfixedcols_(is2d?(mini?3:6):(mini?1:4)) \
	, minimal_(mini)

#define mSetIs2D(is2d) \
    data_.data().setIs2D( is2d );


static int getCompacted( int selgrp, int grp )
{
    return (selgrp<<16) + (grp & 0xFFFF);
}


static void getUnCompacted( int compactedgrp, int& selgrp, int& grp )
{
    selgrp = ( compactedgrp >> 16 );
    if ( compactedgrp < 0 )
	compactedgrp = -compactedgrp;
    grp = ( compactedgrp & 0xFFFF );
}


DataPointSet::Pos::Pos( const Bin2D& b2d, float _z )
    : z_(_z)
{
    set( b2d );
}


DataPointSet::Pos::Pos( const Coord& c, float _z )
    : binid_(SI().transform(c))
    , z_(_z)
{
    setOffs( c );
}


DataPointSet::Pos::Pos( const Coord3& c )
    : binid_(SI().transform(c.getXY()))
    , z_(( float ) c.z_)
{
    setOffs( c.getXY() );
}


void DataPointSet::Pos::setOffs( const Coord& c )
{
    const Coord sc( SI().transform(binid_) );
    offsx_ = ( float ) (c.x_ - sc.x_);
    offsy_ = ( float ) (c.y_ - sc.y_);
}


void DataPointSet::Pos::set( const Coord& c )
{
    binid_ = SI().transform( c );
    setOffs( c );
}


void DataPointSet::Pos::set( const Coord3& c )
{
    z_ = ( float ) c.z_;
    set( c.getXY() );
}


void DataPointSet::Pos::set( const BinID& bid, const Coord& crd )
{
    binid_ = bid;
    setOffs( crd );
}


void DataPointSet::Pos::set( const Bin2D& b2d, const Coord& crd )
{
    bin2d_ = b2d;
    binid_ = SI().transform( crd );
    setOffs( crd );
}


Coord DataPointSet::Pos::coord() const
{
    Coord sc = SI().transform( binid_ );
    sc.x_ += offsx_; sc.y_ += offsy_;
    return sc;
}


void DataPointSet::DataRow::getBVSValues( TypeSet<float>& vals,
					  bool is2d, bool ismini ) const
{
    vals += pos_.z();
    if ( !ismini )
    {
	vals += pos_.offsx_;
	vals += pos_.offsy_;
	vals += grp_;
    }
    if ( is2d )
    {
	vals += (float)pos_.bin2D().lineNr();
	vals += (float)pos_.bin2D().trcNr();
    }
    for ( int idx=0; idx<data_.size(); idx++ )
	vals += data_[idx];
}


od_uint16 DataPointSet::DataRow::group() const
{
    int selgrp,grp;
    getUnCompacted( grp_, selgrp, grp );
    return (od_uint16)((grp < -0.5 ? -grp : grp)+.5);
}


void DataPointSet::DataRow::setGroup( od_uint16 newgrp )
{
    grp_ = grp_ >= 0 ? newgrp : -newgrp;
    grp_ = mCast( short, getCompacted( -1, newgrp ) );
}


DataPointSet::DataPointSet( bool is2d, bool mini )
	: PointDataPack(sKeyDPS)
	, data_(*new PosVecDataSet)
	, mAddMembs(is2d,mini)
{
    initPVDS();
    mSetIs2D(is2d)
}


DataPointSet::DataPointSet( const TypeSet<DataPointSet::DataRow>& pts,
			    const ObjectSet<DataColDef>& dcds, bool is2d,
			    bool mini )
	: PointDataPack(sKeyDPS)
	, data_(*new PosVecDataSet)
	, mAddMembs(is2d,mini)
{
    initPVDS();
    mSetIs2D(is2d)
    init( pts, dcds );
}


DataPointSet::DataPointSet( const TypeSet<DataPointSet::DataRow>& pts,
			    const BufferStringSet& nms, bool is2d, bool mini )
	: PointDataPack(sKeyDPS)
	, data_(*new PosVecDataSet)
	, mAddMembs(is2d,mini)
{
    initPVDS();
    mSetIs2D(is2d)
    ObjectSet<DataColDef> dcds;
    for ( int idx=0; idx<nms.size(); idx++ )
	dcds += new DataColDef( nms.get(idx) );
    init( pts, dcds );
}


class DPSPointExtractor : public Executor
{ mODTextTranslationClass(DPSPointExtractor);
public:

DPSPointExtractor( DataPointSet& dps, ::Pos::Provider& prov,
		   const Pos::Filter* filt, int nrcols, bool filterAccept )
    : Executor( "Extracting positions" )
    , dps_(dps)
    , prov_(prov)
    , filt_(filt)
    , nrcols_(nrcols)
    , p3d_(0)
    , p2d_(0)
    , f3d_(0)
    , f2d_(0)
    , filteraccept_(filterAccept)
{
    mDynamicCast(const ::Pos::Provider3D*,p3d_,&prov)
    mDynamicCast(const ::Pos::Provider2D*,p2d_,&prov)
    mDynamicCast(const ::Pos::Filter3D*,f3d_,filt)
    mDynamicCast(const ::Pos::Filter2D*,f2d_,filt)
    nrdone_ = 0;
    totalnr_ = prov_.estNrPos()*prov_.estNrZPerPos();
    dr_.data_.setSize( nrcols_, mUdf(float) );
}


od_int64 totalNr() const	{ return totalnr_; }

od_int64 nrDone() const		{ return nrdone_; }

uiString message() const	{ return tr("Extracting positions"); }

uiString nrDoneText() const	{ return uiStrings::sPositionsDone(); }


protected:

int nextStep()
{
    nrdone_++;
    if ( !prov_.toNextZ() )
	return Finished();

    const float curz = prov_.curZ();
    if ( mIsUdf(curz) )
	return MoreToDo();

    const Coord crd( prov_.curCoord() );
    if ( p3d_ )
	dr_.pos_.set( crd );
    else
	dr_.pos_.set( p2d_->curBin2D(), crd );
    dr_.pos_.setZ( curz );
    if ( filt_ )
    {
	if ( f3d_ )
	{
	    if ( filteraccept_ ? !f3d_->includes(dr_.pos_.binID(),dr_.pos_.z()):
				f3d_->includes(dr_.pos_.binID(),dr_.pos_.z()) )
		return MoreToDo();
	}
	else if ( f2d_ )
	{
	    if ( filteraccept_ ? !f2d_->includes(dr_.pos_.bin2D(),dr_.pos_.z()):
				f2d_->includes(dr_.pos_.bin2D(),dr_.pos_.z()) )
		return MoreToDo();
	}
    }

    dps_.addRow( dr_ );
    return MoreToDo();
}

    DataPointSet&		dps_;
    ::Pos::Provider&		prov_;
    const ::Pos::Provider3D*	p3d_;
    const ::Pos::Provider2D*	p2d_;
    const ::Pos::Filter*	filt_;
    const ::Pos::Filter3D*	f3d_;
    const ::Pos::Filter2D*	f2d_;
    DataPointSet::DataRow	dr_;
    int				nrcols_;
    od_int64			nrdone_;
    od_int64			totalnr_;
    bool			filteraccept_;
};


bool DataPointSet::extractPositions( ::Pos::Provider& prov,
			    const ObjectSet<DataColDef>& dcds,
			    const TaskRunnerProvider& trprov,
			    const ::Pos::Filter* filt, bool filterAccept )
{
    for ( int idx=0; idx<dcds.size(); idx++ )
	data_.add( new DataColDef(*dcds[idx]) );

    const int nrcols = dcds.size();

    DPSPointExtractor extracttor( *this, prov, filt, nrcols, filterAccept );
    if ( !trprov.execute(extracttor) )
	return false;

    calcIdxs();
    return true;
}


DataPointSet::DataPointSet( const PosVecDataSet& pdvs, bool is2d, bool mini )
	: PointDataPack(sKeyDPS)
	, data_(*new PosVecDataSet)
	, mAddMembs(is2d,mini)
{
    initPVDS();
    mSetIs2D(is2d)
    data_.pars() = pdvs.pars();

    const BinnedValueSet& bvs = pdvs.data();
    const int bvssz = bvs.nrVals();
    const bool isdps = bvssz >= nrfixedcols_
		    && pdvs.colDef(1) == data_.colDef(1)
		    && pdvs.colDef(2) == data_.colDef(2)
		    && pdvs.colDef(3) == data_.colDef(3);
    const int startidx = isdps ? nrfixedcols_ : 1;
    if ( bvssz < startidx )
	return;

    for ( int idx=startidx; idx<bvssz; idx++ )
	data_.add( new DataColDef(pdvs.colDef(idx)) );

    float* vals = new float [ bvssz ];
    DataPointSet::DataRow dr;
    dr.data_.setSize( bvssz - startidx );
    SPos bvspos; BinID bid;
    while ( bvs.next(bvspos) )
    {
	bvs.get( bvspos, bid, vals );
	dr.pos_.set( bid );
	dr.pos_.setZ( vals[0] );
	if ( isdps )
	{
	    dr.grp_ = (short)vals[3];
	    if ( is2d_ )
		dr.pos_.set( Bin2D(GeomID(mNINT32(vals[4])),mNINT32(vals[5])) );
	    dr.pos_.setBinIDOffsets( vals[1], vals[2] );
	}
	for ( int idx=startidx; idx<bvssz; idx++ )
	    dr.data_[idx-startidx] = vals[idx];
	addRow( dr );
    }

    calcIdxs();
}


DataPointSet::DataPointSet( const DataPointSet& dps, const ::Pos::Filter& filt )
	: PointDataPack(sKeyDPS)
	, data_(*new PosVecDataSet)
	, mAddMembs(dps.is2d_,dps.minimal_)
{
    mSetIs2D(dps.is2d_)
    data_.copyStructureFrom( dps.data_ );
    const int typ = filt.is2D() != dps.is2d_ ? -1 : (dps.is2d_ ? 1 : 0);

    mDynamicCastGet(const ::Pos::Filter3D*,f3d,&filt)
    mDynamicCastGet(const ::Pos::Filter2D*,f2d,&filt)

    for ( RowID irow=0; irow<dps.size(); irow++ )
    {
	DataRow dr( dps.dataRow(irow) );
	bool inc = true;
	if ( typ == -1 )
	    inc = filt.includes( dr.pos_.coord(), dr.pos_.z() );
	else if ( f3d )
	    inc = f3d->includes( dr.pos_.binID(), dr.pos_.z() );
	else if ( f2d )
	    inc = f2d->includes( dr.pos_.bin2D(), dr.pos_.z() );

	if ( inc )
	    addRow( dr );
    }

    calcIdxs();
}


DataPointSet::DataPointSet( const DataPointSet& dps )
	: PointDataPack(sKeyDPS)
	, data_(*new PosVecDataSet)
	, mAddMembs(dps.is2d_,dps.minimal_)
{
    data_ = dps.data_;
    mSetIs2D(dps.is2d_)
    sposs_ = dps.sposs_;
}


DataPointSet::~DataPointSet()
{
    delete &data_;
}


mImplMonitorableAssignment( DataPointSet, PointDataPack )
mImplAlwaysDifferentMonitorableCompareClassData( DataPointSet )


void DataPointSet::copyClassData( const DataPointSet& dps )
{
    data_ = dps.data_;
    sposs_ = dps.sposs_;
    is2d_ = dps.is2d_;
    minimal_ = dps.minimal_;
    const_cast<int&>(nrfixedcols_) = dps.nrfixedcols_;
}


void DataPointSet::initPVDS()
{
    if ( !minimal_ )
    {
	data_.add( new DataColDef(sKey::XOffset()) );
	data_.add( new DataColDef(sKey::YOffset()) );
	data_.add( new DataColDef(sKey::SelectionStatus()) );
    }
    if ( is2d_ )
    {
	data_.add( new DataColDef(sKey::GeomID()) );
	data_.add( new DataColDef(sKey::TraceNr()) );
    }
}


void DataPointSet::init( const TypeSet<DataPointSet::DataRow>& pts,
			 const ObjectSet<DataColDef>& dcds )
{
    for ( int idx=0; idx<dcds.size(); idx++ )
	data_.add( new DataColDef(*dcds[idx]) );
    for ( int idx=0; idx<pts.size(); idx++ )
	addRow( pts[idx] );

    calcIdxs();
}


OD::GeomSystem DataPointSet::geomSystem() const
{
    if ( !is2d_ )
	return OD::VolBasedGeom;
    return data_.geomSystem();
}


void DataPointSet::calcIdxs()
{
    sposs_.erase();
    SPos bvspos;
    while ( bivSet().next(bvspos) )
	sposs_ += bvspos;
}


int DataPointSet::nrCols() const
{
    return data_.nrCols() - nrfixedcols_;
}


void DataPointSet::setEmpty()
{
    data_.setEmpty();
    sposs_.erase();
    initPVDS();
}


void DataPointSet::clearData()
{
    bivSet().setEmpty();
}


void DataPointSet::addCol( const char* nm, const char* reference,
			   const UnitOfMeasure* un )
{
    const int idxof = dataSet().findColDef( nm, PosVecDataSet::NameExact );
    if ( idxof < 0 )
	dataSet().add( new DataColDef(nm,reference,un) );
    else
    {
	dataSet().colDef( idxof ).ref_ = reference;
	dataSet().colDef( idxof ).unit_ = un;
    }

}


#define mChkColID( cid, ret ) if ( cid >= nrCols() ) return ret
#define mChkRowID( rid, ret ) if ( rid >= size() ) return ret


const char* DataPointSet::colName( DataPointSet::ColID cid ) const
{
    mChkColID( cid, 0 );
    return colDef( cid ).name_.buf();
}


const UnitOfMeasure* DataPointSet::unit( DataPointSet::ColID cid ) const
{
    mChkColID( cid, 0 );
    return colDef( cid ).unit_;
}


DataColDef& DataPointSet::gtColDef( DataPointSet::ColID cid ) const
{
    return const_cast<DataPointSet*>(this)->data_.colDef( nrfixedcols_ + cid );
}


DataPointSet::ColID DataPointSet::indexOf( const char* nmstr ) const
{
    FixedString nm( nmstr );
    if ( nm.isEmpty() )
	return -1;

    const int nrcols = nrCols();
    for ( int idx=0; idx<nrcols; idx++ )
    {
	if ( nm==colName(idx) )
	    return idx;
    }
    return -1;
}


bool DataPointSet::validColID( ColID cid ) const
{
    return cid >= 0 && cid < nrCols();
}



BinnedValueSet& DataPointSet::bivSet()
{
    return data_.data();
}


DataPointSet::Pos DataPointSet::pos( DataPointSet::RowID rid ) const
{
    mChkRowID( rid, Pos() );
    const float* vals = bivSet().getVals( sposs_[rid] );
    Pos p( binID(rid), vals[0] );
    if ( !minimal_ )
	p.setBinIDOffsets( vals[1], vals[2] );
    if ( is2d_ )
	p.set( Bin2D( GeomID(mNINT32(vals[nrfixedcols_-2])),
				 mNINT32(vals[nrfixedcols_-1]) ) );
    return p;
}


DataPointSet::DataRow DataPointSet::dataRow( DataPointSet::RowID rid ) const
{
    mChkRowID( rid, DataRow() );

    const float* vals = bivSet().getVals( sposs_[rid] );
    const int nrvals = bivSet().nrVals();

    DataRow dr( pos(rid) );
    if ( !minimal_ )
	dr.grp_ = (short)mNINT32(vals[groupcol_]);
    for ( int idx=nrfixedcols_; idx<nrvals; idx++ )
	dr.data_ += vals[idx];

    return dr;
}


BinID DataPointSet::binID( DataPointSet::RowID rid ) const
{
    mChkRowID( rid, BinID(0,0) );
    return bivSet().getBinID( sposs_[rid] );
}


Bin2D DataPointSet::bin2D( DataPointSet::RowID rid ) const
{
    mChkRowID( rid, Bin2D::udf() );
    return bivSet().getBin2D( sposs_[rid] );
}


Coord DataPointSet::coord( DataPointSet::RowID rid ) const
{
    mChkRowID( rid, Coord::udf() );
    return pos(rid).coord();
}


float DataPointSet::z( DataPointSet::RowID rid ) const
{
    mChkRowID( rid, mUdf(float) );
    return bivSet().getVal( sposs_[rid], 0 );
}


DataPointSet::linenr_type DataPointSet::lineNr( DataPointSet::RowID rid ) const
{
    mChkRowID( rid, 0 );
    return is2d_ ? bin2D(rid).lineNr() : binID(rid).inl();
}


DataPointSet::trcnr_type DataPointSet::trcNr( DataPointSet::RowID rid ) const
{
    mChkRowID( rid, 0 );
    return is2d_ ? bin2D(rid).trcNr() : binID(rid).crl();
}


float DataPointSet::value( DataPointSet::ColID cid,
			   DataPointSet::RowID rid ) const
{
    mChkColID( cid, mUdf(float) );
    mChkRowID( rid, mUdf(float) );
    return bivSet().getVal( sposs_[rid], cid + nrfixedcols_ );
}


bool DataPointSet::setValue( DataPointSet::ColID cid, DataPointSet::RowID rid,
			     float val )
{
    mChkColID( cid, false );
    mChkRowID( rid, false );
    float* vals = bivSet().getVals( sposs_[rid] );
    vals[cid + nrfixedcols_] = val;
    return true;
}


float* DataPointSet::getValues( DataPointSet::RowID rid )
{
    mChkRowID( rid, 0 );
    return (bivSet().getVals( sposs_[rid] )) + nrfixedcols_ ;
}


const float* DataPointSet::getValues( DataPointSet::RowID rid ) const
{
    return const_cast<DataPointSet*>(this)->getValues( rid );
}


od_uint16 DataPointSet::group( DataPointSet::RowID rid ) const
{
    if ( minimal_ ) return 0;
    mChkRowID( rid, 0 );
    int selgrp, grp;
    getUnCompacted( mNINT32(bivSet().getVal(sposs_[rid],groupcol_)),
		    selgrp, grp );
    return (od_uint16)((grp < -0.5 ? -grp : grp)+.5);
}


int DataPointSet::selGroup( DataPointSet::RowID rid ) const
{
    int grp,selgrp;
    getUnCompacted( mNINT32(bivSet().getVal(sposs_[rid],groupcol_)),
		    selgrp, grp );
    return selgrp;
}


bool DataPointSet::isSelected( DataPointSet::RowID rid ) const
{
    if ( minimal_ ) return true;
    mChkRowID( rid, 0 );
    return selGroup(rid) >= 0;
}


void DataPointSet::setGroup( DataPointSet::RowID rid, od_uint16 newgrp )
{
    if ( minimal_ )
	return;
    mChkRowID( rid, );
    int grp = getCompacted( -1, newgrp ) ;
    bivSet().getVals( sposs_[rid] )[ groupcol_ ] = mCast( float, grp );
}


void DataPointSet::setSelected( DataPointSet::RowID rid, int selgrp )
{
    if ( minimal_ ) return;
    mChkRowID( rid, );
    short grp = (short)group( rid );
    bivSet().getVals( sposs_[rid] )[ groupcol_ ] =
				  mCast( float, getCompacted( selgrp, grp) );
}


void DataPointSet::setInactive( DataPointSet::RowID rid, bool sel )
{
    if ( minimal_ ) return;
    mChkRowID( rid, );
    bivSet().getVals( sposs_[rid] )[ groupcol_ ] = 0;
}


void DataPointSet::addRow( const DataPointSet::DataRow& dr )
{
    TypeSet<float> vals; dr.getBVSValues( vals, is2d_, minimal_ );
    bivSet().add( dr.binID(), vals );
}


bool DataPointSet::setRow( const DataPointSet::DataRow& dr )
{
    bool alreadyin = false;
    SPos bvspos = bivSet().find( dr.pos_.binID() );
    while ( bvspos.isValid() && bivSet().getBinID(bvspos) == dr.pos_.binID() )
    {
	const float zval = *bivSet().getVals( bvspos );
	if ( mIsZero(zval-dr.pos_.z(),mDefEps) )
	    { alreadyin = true; break; }
	bivSet().next( bvspos );
    }

    if ( !alreadyin )
	{ addRow( dr ); return true; }

    TypeSet<float> vals; dr.getBVSValues( vals, is2d_, minimal_ );
    bivSet().set( bvspos, vals );
    return false;
}


float DataPointSet::gtNrKBytes() const
{
    const int twointsz = 2 * sizeof(int);
    const float rowsz =
	sKb2MbFac() * (twointsz + bivSet().nrVals()*sizeof(float));
    const int nrrows = mCast( int, bivSet().totalSize() );
    return nrrows * (rowsz + twointsz);
}


void DataPointSet::doDumpInfo( IOPar& iop ) const
{
    BufferString typstr( "PointSet (" );
    typstr += is2d_ ? "2D" : "3D";
    typstr += minimal_ ? ",minimal)" : ")";
    iop.set( sKey::Type(), typstr );
    iop.set( "Number of rows", bivSet().totalSize() );
    const int nrcols = nrCols();
    iop.set( "Number of cols", nrcols );
    for ( int idx=0; idx<nrcols; idx++ )
	iop.set( IOPar::compKey("Col",idx), colName(idx) );
}


int DataPointSet::nrActive() const
{
    int ret = 0;
    for ( RowID irow=0; irow<size(); irow++ )
    {
	if ( !isInactive(irow) )
	    ret++;
    }
    return ret;
}


void DataPointSet::purgeInactive()
{
    TypeSet<SPos> torem;
    for ( RowID irow=0; irow<size(); irow++ )
    {
	if ( isInactive(irow) )
	    torem += sposs_[irow];
    }
    if ( !torem.isEmpty() )
    {
	bivSet().remove( torem );
	calcIdxs();
    }
}


void DataPointSet::purgeSelected( bool sel )
{
    TypeSet<SPos> torem;
    for ( RowID irow=0; irow<size(); irow++ )
    {
	if ( sel != isSelected(irow) )
	    torem += sposs_[irow];
    }
    if ( !torem.isEmpty() )
    {
	bivSet().remove( torem );
	calcIdxs();
    }
}


void DataPointSet::randomSubselect( int maxsz )
{
    bivSet().randomSubselect( maxsz );
    dataChanged();
}


DataPointSet* DataPointSet::getSubselected( int maxsz,
			const TypeSet<int>* outcols, bool allowudf,
			const ObjectSet<Interval<float> >* rgs ) const
{
    const int mysz = size();
    const int mynrcols = nrCols();
    TypeSet<int> colsbuf;
    if ( !outcols )
    {
	for ( ColID icol=0; icol<mynrcols; icol++ )
	    colsbuf += icol;
	outcols = &colsbuf;
    }
    if ( maxsz <= mysz && allowudf && outcols->size() == mynrcols )
	return new DataPointSet( *this );

    ObjectSet<DataColDef> cds;
    for ( ColID icol=0; icol<outcols->size(); icol++ )
    {
	const int colnr = (*outcols)[icol];
	if ( colnr < mynrcols )
	    cds += const_cast<DataColDef*>(
			    dataSet().coldefs_[ nrfixedcols_ + colnr ] );
    }
    DataPointSet* ret = new DataPointSet( TypeSet<DataRow>(), cds, is2D(),
					  isMinimal() );

    mGetIdxArr( RowID, idxs, mysz );
    if ( !idxs ) { delete ret; return 0; }

    const int nrcols = outcols->size();
    int activesz = mysz;

    if ( !allowudf || rgs )
    {
	for ( RowID idx=0; idx<activesz; idx++ )
	{
	    RowID irow = idxs[ idx ];
	    const float* vals = getValues( irow );
	    bool canuse = true;
	    for ( ColID icol=0; icol<nrcols; icol++ )
	    {
		const float val = vals[ (*outcols)[icol] ];
		if ( !allowudf && mIsUdf(val) )
		    { canuse = false; break; }
		if ( rgs )
		{
		    const Interval<float>* rg = (*rgs)[icol];
		    if ( rg )
		    {
			if ( (!mIsUdf(rg->start) && val < rg->start)
			   || (!mIsUdf(rg->stop) && val > rg->stop) )
			    { canuse = false; break; }
		    }
		}
	    }
	    if ( !canuse )
		{ activesz--; idxs[idx] = activesz; idx--; }
	}
    }

    if ( activesz > maxsz )
	Stats::randGen().subselect( idxs, activesz, maxsz );
    else
	maxsz = activesz;

    for ( int idx=0; idx<maxsz; idx++ )
    {
	const RowID irow = idxs[idx];
	DataRow dr( dataRow(irow) );
	const float* vals = getValues( irow );
	dr.data_.erase();
	for ( ColID icol=0; icol<nrcols; icol++ )
	    dr.data_ += vals[ (*outcols)[icol] ];

	ret->addRow( dr );
    }
    delete [] idxs;

    ret->dataChanged();
    return ret;
}


DataPointSet::RowID DataPointSet::getRowID( SPos bvspos ) const
{
    int rid = -1;
    return IdxAble::findPos( sposs_.arr(), sposs_.size(), bvspos, -1, rid )
	? rid : -1;
}


DataPointSet::RowID DataPointSet::find( const DataPointSet::Pos& dpos ) const
{
    SPos bpos = bivSet().find( dpos.binID() );
    bivSet().prev( bpos );
    while ( bivSet().next(bpos) )
    {
	if ( bivSet().getBinID(bpos) != dpos.binID() )
	    break;

	const float zval = bivSet().getVals(bpos)[0];
	if ( mIsZero(zval-dpos.z(),1e-6) )
	    return getRowID( bpos );
    }
    return -1;
}


#define mGetZ( dz, res ) \
if ( !SI().zIsTime() ) \
{ \
    if ( (SI().xyInFeet() && SI().zInFeet()) || \
	 (!SI().xyInFeet() && SI().zInMeter()) ) \
	res = dz; \
    else \
	res = SI().xyInFeet()&&SI().zInMeter() ? dz*mToFeetFactorF \
					       : dz*mFromFeetFactorF; \
} \
else \
{ \
    res = dz * SI().zDomain().userFactor(); \
    if ( SI().xyInFeet() ) \
	res *= mToFeetFactorF; \
}


DataPointSet::RowID DataPointSet::find( const DataPointSet::Pos& dpos,
					float horradius, float deltaz ) const
{
    float zinxy = mUdf(float);
    mGetZ( deltaz, zinxy );
    const float maxdist = Math::Sqrt(2*(horradius*horradius) + deltaz*deltaz);
    float mindist = mUdf(float);
    int resrowidx=-1;
    mGetZ( dpos.z(), zinxy );
    Coord3 targetpos( dpos.coord(), zinxy );
    for ( int rowidx=0; rowidx<sposs_.size(); rowidx++ )
    {
	mGetZ( z(rowidx), zinxy );
	Coord3 poscoord( coord(rowidx), zinxy );
	const float dist = poscoord.distTo<float>( targetpos );
	if ( dist < maxdist  && dist < mindist )
	{
	    resrowidx = rowidx;
	    mindist = dist;
	}
    }

    return resrowidx;
}


DataPointSet::RowID DataPointSet::findFirst( const BinID& bid ) const
{
    SPos bpos = bivSet().find( bid );
    return getRowID( bpos );
}


DataPointSet::RowID DataPointSet::findFirst( const Bin2D& b2d ) const
{
    return findFirst( b2d.coord() );
}


DataPointSet::RowID DataPointSet::findFirst( const Coord& crd ) const
{
    const BinID bid = SI().transform( crd );
    if ( minimal_ )
	return findFirst( bid );
    const DataPointSet::RowID rid = findFirst( bid );
    if ( rid < 0 )
	return -1;

    for ( int idx=rid; idx<size(); idx++ )
    {
	if ( binID(idx) != bid ) break;
	Coord c( coord(idx) );
	if ( mIsEqual( c.x_, crd.x_, 1e-3 )
	  && mIsEqual( c.y_, crd.y_, 1e-3 ) ) return idx;
    }

    return -1;
}


void DataPointSet::dumpLocations( od_ostream* strm ) const
{
    if ( !strm )
	strm = &od_cout();
    for ( RowID irow=0; irow<size(); irow++ )
    {
	const Coord crd( coord( irow ) );
	*strm << crd.x_ << ' ';
	*strm << crd.y_ << ' ';
	*strm << z(irow) << od_endl;
    }
}


// DPSFromVolumeFiller
DPSFromVolumeFiller::DPSFromVolumeFiller( DataPointSet& dps, int firstcol,
				const VolumeDataPack& vdp, int component )
    : ParallelTask()
    , dps_(dps)
    , vdp_(vdp)
    , component_(component)
    , firstcol_(firstcol)
    , hastrcdata_(false)
    , hasstorage_(false)
    , sampling_(0)
{
    dps_.ref();
    vdp_.ref();

    const int testcomponent = component==-1 ? 0 : component;
    if ( !vdp_.isEmpty() )
    {
	const Array3DImpl<float>& array = vdp_.data( testcomponent );
	hastrcdata_ = array.getData();
	hasstorage_ = array.getStorage();
    }
}


DPSFromVolumeFiller::~DPSFromVolumeFiller()
{
    dps_.unRef();
    vdp_.unRef();
}


uiString DPSFromVolumeFiller::message() const
{
    return tr("Reading data");
}


uiString DPSFromVolumeFiller::nrDoneText() const
{
    return tr("Traces done");
}


od_int64 DPSFromVolumeFiller::nrIterations() const
{
    return dps_.size();
}

void DPSFromVolumeFiller::setSampling( const TrcKeyZSampling* tkzs )
{ sampling_ = tkzs; }



bool DPSFromVolumeFiller::doWork( od_int64 start, od_int64 stop, int thridx )
{
    const StepInterval<float>& zsamp = vdp_.zRange();
    const int nrz = zsamp.nrSteps() + 1;
    const SamplingData<float> sd( zsamp.start, zsamp.step );
    Array1DImpl<float>* trcarr = 0;
    if ( !hastrcdata_ && !hasstorage_ )
	trcarr = new Array1DImpl<float>( nrz );
    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	const DataPointSet::RowID rid = mCast(int,idx);
	float* vals = dps_.getValues( rid );
	BinID bid = dps_.binID( rid );

	if ( sampling_ )
	{
	    if ( !sampling_->hsamp_.lineRange().includes(bid.lineNr(),true) ||
		 !sampling_->hsamp_.trcRange().includes(bid.trcNr(),true) )
		continue;

	    bid = sampling_->hsamp_.getNearest( bid );
	}

	const int gidx = vdp_.getGlobalIdx( TrcKey(bid,dps_.is2D()) );
	if ( gidx<0 )
	    continue;

	const float zval = dps_.z( rid );
	const float fzidx = sd.getfIndex( zval );
	int outidx = 0;
	for ( int cidx=0; cidx<vdp_.nrComponents(); cidx++ )
	{
	    if ( component_!=-1 && component_!=cidx )
		continue;

	    if ( hastrcdata_ )
	    {
		const float* trcdata = vdp_.getTrcData( cidx, gidx );
		const SampledFunctionImpl<float,const float*>
						sampfunc( trcdata, nrz );
		vals[firstcol_+outidx] = sampfunc.getValue( fzidx );
	    }
	    else if ( hasstorage_ )
	    {
		const OffsetValueSeries<float> ovs =
			vdp_.getTrcStorage( cidx, gidx );
		const SampledFunctionImpl<float,ValueSeries<float> >
						sampfunc( ovs, nrz );
		vals[firstcol_+outidx] = sampfunc.getValue( fzidx );
	    }
	    else if ( vdp_.getCopiedTrcData(cidx,gidx,*trcarr) )
	    {
		float* dataptr = trcarr->getData();
		const SampledFunctionImpl<float,const float*>
						sampfunc( dataptr, nrz );
		vals[firstcol_+outidx] = sampfunc.getValue( fzidx );
	    }

	    outidx++;
	}

	addToNrDone( 1 );
    }

    return true;
}
