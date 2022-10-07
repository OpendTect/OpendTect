/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "datapointset.h"

#include "arrayndimpl.h"
#include "bufstringset.h"
#include "datacoldef.h"
#include "executor.h"
#include "idxable.h"
#include "iopar.h"
#include "keystrs.h"
#include "posprovider.h"
#include "posvecdataset.h"
#include "samplfunc.h"
#include "statrand.h"
#include "survinfo.h"

static const char* sKeyDPS = "Data Point Set";
const int DataPointSet::groupcol_ = 3;
#define mAddMembs(is2d,mini) \
	  is2d_(is2d) \
	, nrfixedcols_(mini?1:4) \
	, minimal_(mini)


static int getCompacted( int selgrp, int grp )
{
    return (selgrp<<16) + (grp & 0xFFFF);
}


static void getUnCompacted( int compactedgrp, int& selgrp, int& grp )
{
    selgrp = ( compactedgrp >> 16 );
    grp = ( compactedgrp & 0xFFFF );
}


mStartAllowDeprecatedSection

DataPointSet::Pos::Pos()
    : offsx_(0), offsy_(0), z_(0)
    , binid_(const_cast<BinID&>(trckey_.position()))
    , nr_(binid_.trcNr())
{}

mStopAllowDeprecatedSection


DataPointSet::Pos::Pos( const BinID& bid, float z )
    : Pos()
{
    trckey_.setPosition( bid );
    z_ = z;
}


DataPointSet::Pos::Pos( ::Pos::GeomID geomid, int trcnr, float z )
    : Pos()
{
    trckey_.setPosition( geomid, trcnr );
    z_ = z;
}


DataPointSet::Pos::Pos( const Coord& c, float z )
    : Pos()
{
    z_ = z;
    setOffs( c );
}


DataPointSet::Pos::Pos( const Coord3& c )
    : Pos()
{
    z_ = float( c.z );
    setOffs( c );
}


DataPointSet::Pos::Pos( const DataPointSet::Pos& oth )
    : Pos()
{
    trckey_ = oth.trckey_;
    z_ = oth.z_;
}


DataPointSet::Pos::~Pos()
{}


DataPointSet::Pos& DataPointSet::Pos::operator =( const DataPointSet::Pos& oth )
{
    if ( this != &oth )
    {
	trckey_ = oth.trckey_;
	z_ = oth.z_;
    }

    return *this;
}


void DataPointSet::Pos::setOffs( const Coord& c )
{
    const Coord sc = trckey_.getCoord();
    offsx_ = ( float ) (c.x - sc.x);
    offsy_ = ( float ) (c.y - sc.y);
}


void DataPointSet::Pos::set( ::Pos::GeomID geomid, int trcnr )
{
    trckey_.setPosition( geomid, trcnr );
    offsx_ = offsy_ = 0;
}


void DataPointSet::Pos::set( const Coord& c )
{
    trckey_.setFrom( c );
    setOffs( c );
}


void DataPointSet::Pos::set( const Coord3& c )
{
    z_ = ( float ) c.z;
    set( ((const Coord&)c) );
}


Coord DataPointSet::Pos::coord() const
{
    Coord sc = trckey_.getCoord();
    sc.x += offsx_; sc.y += offsy_;
    return sc;
}


DataPointSet::DataRow::DataRow()
    : grp_(1)
{
    setSel( false );
}


DataPointSet::DataRow::DataRow( const Pos& p, unsigned short grp, bool issel )
    : pos_(p), grp_((short)grp)
{
    setSel( issel );
}


DataPointSet::DataRow::~DataRow()
{}


void DataPointSet::DataRow::getBVSValues( TypeSet<float>& vals,
					  bool is2d, bool ismini ) const
{
    vals += pos_.z_;
    if ( !ismini )
    {
	vals += pos_.offsx_;
	vals += pos_.offsy_;
	vals += grp_;
    }

    for ( int idx=0; idx<data_.size(); idx++ )
	vals += data_[idx];
}


unsigned short DataPointSet::DataRow::group() const
{
    int selgrp,grp;
    getUnCompacted( grp_, selgrp, grp );
    return (unsigned short)((grp < -0.5 ? -grp : grp)+.5);
}


void DataPointSet::DataRow::setGroup( unsigned short newgrp )
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
}


DataPointSet::DataPointSet( const TypeSet<DataPointSet::DataRow>& pts,
			    const ObjectSet<DataColDef>& dcds, bool is2d,
			    bool mini )
	: PointDataPack(sKeyDPS)
	, data_(*new PosVecDataSet)
	, mAddMembs(is2d,mini)
{
    initPVDS();
    init( pts, dcds );
}


DataPointSet::DataPointSet( const TypeSet<DataPointSet::DataRow>& pts,
			    const BufferStringSet& nms, bool is2d, bool mini )
	: PointDataPack(sKeyDPS)
	, data_(*new PosVecDataSet)
	, mAddMembs(is2d,mini)
{
    initPVDS();
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


od_int64 totalNr() const override	{ return totalnr_; }

od_int64 nrDone() const override	{ return nrdone_; }

uiString uiMessage() const override	{ return tr("Extracting positions"); }

uiString uiNrDoneText() const override	{ return tr("Positions done"); }


protected:

int nextStep() override
{
    nrdone_++;
    if ( !prov_.toNextZ() )
	return Finished();

    const float curz = prov_.curZ();
    if ( mIsUdf(curz) )
	return MoreToDo();

    DataPointSet::Pos& pos = dr_.pos_;
    if ( p3d_ )
	pos.set( p3d_->curBinID() );
    else if ( p2d_ )
	pos.set( p2d_->curGeomID(), p2d_->curNr() );

    pos.z_ = curz;
    if ( filt_ )
    {
	if ( f3d_ )
	{
	    if ( filteraccept_ != f3d_->includes(pos.binID(),pos.z_) )
		return MoreToDo();
	}
	else if ( f2d_ )
	{
	    if ( filteraccept_ !=
		    f2d_->includes(pos.geomID(),pos.trcNr(),pos.z_) )
		return MoreToDo();
	}
	if ( filt_->hasZAdjustment() )
	{
	    const Coord crd( prov_.curCoord() );
	    pos.z_ = filt_->adjustedZ( crd, (float) pos.z_ );
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
			    const ::Pos::Filter* filt,
			    TaskRunner* tr, bool filterAccept )
{
    for ( int idx=0; idx<dcds.size(); idx++ )
	data_.add( new DataColDef(*dcds[idx]) );

    const int nrcols = dcds.size();

    DPSPointExtractor extracttor( *this, prov, filt, nrcols, filterAccept );
    if ( !TaskRunner::execute(tr,extracttor) )
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
    data_.pars() = pdvs.pars();

    const BinIDValueSet& bvs = pdvs.data();
    const int bvssz = bvs.nrVals();
    const bool isdps = bvssz >= nrfixedcols_
		    && pdvs.colDef(1) == data_.colDef(1)
		    && pdvs.colDef(2) == data_.colDef(2)
		    && pdvs.colDef(3) == data_.colDef(3);
    const int startidx = isdps ? nrfixedcols_ : 1;
    if ( bvssz < startidx ) return;

    for ( int idx=startidx; idx<bvssz; idx++ )
	data_.add( new DataColDef(pdvs.colDef(idx)) );

    float* vals = new float [ bvssz ];
    BinID bid;
    DataPointSet::DataRow dr;
    dr.data_.setSize( bvssz - startidx );
    BinIDValueSet::SPos bvspos;
    while ( bvs.next(bvspos) )
    {
	bvs.get( bvspos, bid, vals );
	dr.pos_.trckey_.setPosition( bid );
	dr.pos_.z_ = vals[0];
	if ( isdps )
	{
	    dr.pos_.setBinIDOffsets( vals[1], vals[2] );
	    dr.grp_ = (short)vals[3];
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
    data_.copyStructureFrom( dps.data_ );
    const int typ = filt.is2D() != dps.is2d_ ? -1 : (dps.is2d_ ? 1 : 0);

    mDynamicCastGet(const ::Pos::Filter3D*,f3d,&filt)
    mDynamicCastGet(const ::Pos::Filter2D*,f2d,&filt)

    for ( RowID irow=0; irow<dps.size(); irow++ )
    {
	const DataRow dr( dps.dataRow(irow) );
	const DataPointSet::Pos& pos = dr.pos_;
	bool inc = true;
	if ( typ == -1 )
	    inc = filt.includes( pos.coord(), pos.z_ );
	else if ( f3d )
	    inc = f3d->includes( pos.binID(), pos.z_ );
	else if ( f2d )
	    inc = f2d->includes( pos.geomID(), pos.trcNr(), pos.z_ );

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
    bvsidxs_ = dps.bvsidxs_;
}


DataPointSet::~DataPointSet()
{
    delete &data_;
}


DataPointSet& DataPointSet::operator =( const DataPointSet& dps )
{
    if ( this != &dps )
    {
	data_ = dps.data_;
	bvsidxs_ = dps.bvsidxs_;
	is2d_ = dps.is2d_;
	minimal_ = dps.minimal_;
	const_cast<int&>(nrfixedcols_) = dps.nrfixedcols_;
    }
    return *this;
}


void DataPointSet::initPVDS()
{
    bivSet().setIs2D( is2D() );
    if ( minimal_ )
	return;

    data_.add( new DataColDef(sKey::XOffset()) );
    data_.add( new DataColDef(sKey::YOffset()) );
    data_.add( new DataColDef(sKey::SelectionStatus()) );
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


void DataPointSet::calcIdxs()
{
    bvsidxs_.erase();
    BinIDValueSet::SPos bvspos;
    while ( bivSet().next(bvspos) )
	bvsidxs_ += bvspos;
}


int DataPointSet::nrCols() const
{
    return data_.nrCols() - nrfixedcols_;
}


void DataPointSet::setEmpty()
{
    data_.setEmpty();
    bvsidxs_.erase();
    initPVDS();
}


void DataPointSet::clearData()
{
    bivSet().setEmpty();
}


void DataPointSet::addCol( const char* nm, const char* ref,
				const UnitOfMeasure* un )
{
    const int idxof = dataSet().findColDef( nm, PosVecDataSet::NameExact );
    if ( idxof < 0 )
	dataSet().add( new DataColDef(nm,ref,un) );
    else
    {
	dataSet().colDef( idxof ).ref_ = ref;
	dataSet().colDef( idxof ).unit_ = un;
    }

}


#define mChkColID(cid,ret) if ( cid >= nrCols() ) return ret
#define mChkRowID(rid,ret) if ( rid >= size() ) return ret


const char* DataPointSet::colName( DataPointSet::ColID cid ) const
{
    mChkColID(cid,0);
    return colDef( cid ).name_.buf();
}


const UnitOfMeasure* DataPointSet::unit( DataPointSet::ColID cid ) const
{
    mChkColID(cid,0);
    return colDef( cid ).unit_;
}


DataColDef& DataPointSet::gtColDef( DataPointSet::ColID cid ) const
{
    return const_cast<DataPointSet*>(this)->data_.colDef( nrfixedcols_ + cid );
}


DataPointSet::ColID DataPointSet::indexOf( const char* nmstr ) const
{
    StringView nm( nmstr );
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



BinIDValueSet& DataPointSet::bivSet()
{
    return data_.data();
}


DataPointSet::Pos DataPointSet::pos( DataPointSet::RowID rid ) const
{
    mChkRowID(rid,Pos());
    const BinID bid = binID( rid );
    Pos pos;
    if ( is2D() )
	pos.set( ::Pos::GeomID(bid.lineNr()), bid.trcNr() );
    else
	pos.set( bid );

    const float* vals = bivSet().getVals( bvsidxs_[rid] );
    pos.z_ = vals[0];
    if ( !minimal_ )
	pos.setBinIDOffsets( vals[1], vals[2] );

    return pos;
}


DataPointSet::DataRow DataPointSet::dataRow( DataPointSet::RowID rid ) const
{
    mChkRowID(rid,DataRow());

    const float* vals = bivSet().getVals( bvsidxs_[rid] );
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
    mChkRowID(rid,BinID(0,0));
    return bivSet().getBinID( bvsidxs_[rid] );
}


TrcKey DataPointSet::trcKey( DataPointSet::RowID rid ) const
{
    mChkRowID(rid,TrcKey::udf());
    return pos(rid).trckey_;
}


Coord DataPointSet::coord( DataPointSet::RowID rid ) const
{
    mChkRowID(rid,Coord::udf());
    return pos(rid).coord();
}


float DataPointSet::z( DataPointSet::RowID rid ) const
{
    mChkRowID(rid,mUdf(float));
    return bivSet().getVal( bvsidxs_[rid], 0 );
}


::Pos::GeomID DataPointSet::geomID( DataPointSet::RowID rid ) const
{
    mChkRowID(rid,::Pos::GeomID::udf());
    return pos(rid).geomID();
}


int DataPointSet::trcNr( DataPointSet::RowID rid ) const
{
    mChkRowID(rid,0);
    return pos(rid).trcNr();
}


float DataPointSet::value( DataPointSet::ColID cid,
			   DataPointSet::RowID rid ) const
{
    mChkColID(cid,mUdf(float));
    mChkRowID(rid,mUdf(float));
    return bivSet().getVal( bvsidxs_[rid], cid + nrfixedcols_ );
}


bool DataPointSet::setValue( DataPointSet::ColID cid, DataPointSet::RowID rid,
			     float val )
{
    mChkColID(cid,false);
    mChkRowID(rid,false);
    float* vals = bivSet().getVals( bvsidxs_[rid] );
    vals[cid + nrfixedcols_] = val;
    return true;
}


float* DataPointSet::getValues( DataPointSet::RowID rid )
{
    mChkRowID(rid,0);
    return (bivSet().getVals( bvsidxs_[rid] )) + nrfixedcols_ ;
}


const float* DataPointSet::getValues( DataPointSet::RowID rid ) const
{
    return const_cast<DataPointSet*>(this)->getValues( rid );
}


unsigned short DataPointSet::group( DataPointSet::RowID rid ) const
{
    if ( minimal_ ) return 0;
    mChkRowID(rid,0);
    int selgrp, grp;
    getUnCompacted( mNINT32(bivSet().getVal(bvsidxs_[rid],groupcol_)),
		    selgrp, grp );
    return (unsigned short)((grp < -0.5 ? -grp : grp)+.5);
}


int DataPointSet::selGroup( DataPointSet::RowID rid ) const
{
    int grp,selgrp;
    getUnCompacted( mNINT32(bivSet().getVal(bvsidxs_[rid],groupcol_)),
		    selgrp, grp );
    return selgrp;
}


bool DataPointSet::isSelected( DataPointSet::RowID rid ) const
{
    if ( minimal_ ) return true;
    mChkRowID(rid,0);
    return selGroup(rid) >= 0;
}


void DataPointSet::setGroup( DataPointSet::RowID rid, unsigned short newgrp )
{
    if ( minimal_ ) return;
    mChkRowID(rid,);
    int grp = getCompacted( -1, newgrp ) ;
    bivSet().getVals( bvsidxs_[rid] )[ groupcol_ ] = mCast( float, grp );
}


void DataPointSet::setSelected( DataPointSet::RowID rid, int selgrp )
{
    if ( minimal_ ) return;
    mChkRowID(rid,);
    short grp = (short)group( rid );
    bivSet().getVals( bvsidxs_[rid] )[ groupcol_ ] =
				  mCast( float, getCompacted( selgrp, grp) );
}


void DataPointSet::setInactive( DataPointSet::RowID rid, bool sel )
{
    if ( minimal_ ) return;
    mChkRowID(rid,);
    bivSet().getVals( bvsidxs_[rid] )[ groupcol_ ] = 0;
}


void DataPointSet::addRow( const DataPointSet::DataRow& dr )
{
    TypeSet<float> vals; dr.getBVSValues( vals, is2d_, minimal_ );
    bivSet().add( dr.binID(), vals );
}


bool DataPointSet::setRow( const DataPointSet::DataRow& dr )
{
    bool alreadyin = false;
    BinIDValueSet::SPos bvspos = bivSet().find( dr.pos_.binID() );
    while ( bvspos.isValid() && bivSet().getBinID(bvspos) == dr.pos_.binID() )
    {
	const float zval = *bivSet().getVals( bvspos );
	if ( mIsZero(zval-dr.pos_.z_,mDefEps) )
	    { alreadyin = true; break; }
	bivSet().next( bvspos );
    }

    if ( !alreadyin )
	{ addRow( dr ); return true; }

    TypeSet<float> vals; dr.getBVSValues( vals, is2d_, minimal_ );
    bivSet().set( bvspos, vals );
    return false;
}


float DataPointSet::nrKBytes() const
{
    const int twointsz = 2 * sizeof(int);
    const float rowsz =
	sKb2MbFac() * (twointsz + bivSet().nrVals()*sizeof(float));
    const int nrrows = mCast( int, bivSet().totalSize() );
    return nrrows * (rowsz + twointsz);
}


bool DataPointSet::getRange( TrcKeyZSampling& tkzs ) const
{
    if ( bivSet().isEmpty() || !bivSet().nrVals() )
	return false;

    tkzs.hsamp_.set( bivSet().firstRange(), bivSet().secondRange() );
    tkzs.zsamp_.setFrom( bivSet().valRange(0) );
    return true;
}


void DataPointSet::dumpInfo( StringPairSet& infoset ) const
{
    BufferString typstr( "PointSet (" );
    typstr += is2d_ ? "2D" : "3D";
    typstr += minimal_ ? ",minimal)" : ")";
    infoset.add( sKey::Type(), typstr );
    infoset.add( "Number of rows", bivSet().totalSize() );
    const int nrcols = nrCols();
    infoset.add( "Number of cols", nrcols );
    for ( int idx=0; idx<nrcols; idx++ )
	infoset.add( IOPar::compKey("Col",idx), colName(idx) );
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
    TypeSet<BinIDValueSet::SPos> torem;
    for ( RowID irow=0; irow<size(); irow++ )
    {
	if ( isInactive(irow) )
	    torem += bvsidxs_[irow];
    }
    if ( !torem.isEmpty() )
    {
	bivSet().remove( torem );
	calcIdxs();
    }
}


void DataPointSet::purgeSelected( bool sel )
{
    TypeSet<BinIDValueSet::SPos> torem;
    for ( RowID irow=0; irow<size(); irow++ )
    {
	if ( sel != isSelected(irow) )
	    torem += bvsidxs_[irow];
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
    {
	static Stats::RandGen gen;
	gen.subselect( idxs, activesz, maxsz );
    }
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


DataPointSet::RowID DataPointSet::getRowID( BinIDValueSet::SPos bvspos ) const
{
    int rid = -1;
    return IdxAble::findPos( bvsidxs_.arr(), bvsidxs_.size(), bvspos, -1, rid )
	? rid : -1;
}


DataPointSet::RowID DataPointSet::find( const DataPointSet::Pos& dpos ) const
{
    BinIDValueSet::SPos bpos = bivSet().find( dpos.binID() );
    bivSet().prev( bpos );
    while ( bivSet().next(bpos) )
    {
	if ( bivSet().getBinID(bpos) != dpos.binID() )
	    break;

	const float zval = bivSet().getVals(bpos)[0];
	if ( mIsZero(zval-dpos.z_,1e-6) )
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
    mGetZ( dpos.z_, zinxy );
    Coord3 targetpos( dpos.coord(), zinxy );
    for ( int rowidx=0; rowidx<bvsidxs_.size(); rowidx++ )
    {
	mGetZ( z(rowidx), zinxy );
	Coord3 poscoord( coord(rowidx), zinxy );
	const float dist = (float) poscoord.distTo( targetpos );
	if ( dist < maxdist  && dist < mindist )
	{
	    resrowidx = rowidx;
	    mindist = dist;
	}
    }

    return resrowidx;
}


DataPointSet::RowID DataPointSet::findFirst( const TrcKey& tkey ) const
{
    if ( tkey.is2D() == !this->is2D() )
	return -1;

    BinIDValueSet::SPos bpos = bivSet().find( tkey.position() );
    return getRowID( bpos );
}


DataPointSet::RowID DataPointSet::findFirst( const BinID& bid ) const
{
    BinIDValueSet::SPos bpos = bivSet().find( bid );
    return getRowID( bpos );
}


DataPointSet::RowID DataPointSet::findFirst( const Coord& crd ) const
{
    const BinID bid = SI().transform( crd );
    if ( minimal_ ) return findFirst( bid );
    const DataPointSet::RowID rid = findFirst( bid );
    if ( rid < 0 ) return -1;

    for ( int idx=rid; idx<size(); idx++ )
    {
	if ( binID(idx) != bid ) break;
	Coord c( coord(idx) );
	if ( mIsEqual( c.x, crd.x, 1e-3 )
	  && mIsEqual( c.y, crd.y, 1e-3 ) ) return idx;
    }

    return -1;
}


// DPSFromVolumeFiller
DPSFromVolumeFiller::DPSFromVolumeFiller( DataPointSet& dps, int firstcol,
				const SeisDataPack& sdp, int component )
    : ParallelTask()
    , dps_(dps)
    , sdp_(sdp)
    , component_(component)
    , firstcol_(firstcol)
    , hastrcdata_(false)
    , hasstorage_(false)
    , sampling_(0)
{
    dps_.ref();
    sdp_.ref();

    const int testcomponent = component==-1 ? 0 : component;
    if ( !sdp_.isEmpty() )
    {
	const Array3DImpl<float>& array = sdp_.data( testcomponent );
	hastrcdata_ = array.getData();
	hasstorage_ = array.getStorage();
    }
}


DPSFromVolumeFiller::~DPSFromVolumeFiller()
{
    dps_.unRef();
    sdp_.unRef();
}


uiString DPSFromVolumeFiller::uiMessage() const
{
    return tr("Reading data");
}


uiString DPSFromVolumeFiller::uiNrDoneText() const
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
    const ZSampling zsamp = sdp_.zRange();
    const int nrz = zsamp.nrSteps() + 1;
    const int nrtrcs = sdp_.nrTrcs();
    const SamplingData<float> sd( zsamp.start, zsamp.step );
    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	const DataPointSet::RowID rid = mCast(int,idx);
	BinID bid = dps_.binID( rid );

	if ( sampling_ )
	{
	    if ( !sampling_->hsamp_.lineRange().includes(bid.lineNr(),true) ||
		 !sampling_->hsamp_.trcRange().includes(bid.trcNr(),true) )
		continue;

	    bid = sampling_->hsamp_.getNearest( bid );
	}

	const TrcKey tk = sdp_.is2D()
			? TrcKey( sampling_->hsamp_.getGeomID(), bid.trcNr() )
			: TrcKey( bid );
	const int gidx = sdp_.getGlobalIdx( tk );
	if ( gidx<0 || gidx>nrtrcs ) continue;

	float* vals = dps_.getValues( rid );
	const float zval = dps_.z( rid );
	const float fzidx = sd.getfIndex( zval );
	int outidx = 0;
	for ( int cidx=0; cidx<sdp_.nrComponents(); cidx++ )
	{
	    if ( component_!=-1 && component_!=cidx )
		continue;

	    if ( hastrcdata_ )
	    {
		const float* trcdata = sdp_.getTrcData( cidx, gidx );
		const SampledFunctionImpl<float,const float*>
						sampfunc( trcdata, nrz );
		vals[firstcol_+outidx] = sampfunc.getValue( fzidx );
	    }
	    else if ( hasstorage_ )
	    {
		const OffsetValueSeries<float> ovs =
			sdp_.getTrcStorage( cidx, gidx );
		const SampledFunctionImpl<float,ValueSeries<float> >
						sampfunc( ovs, nrz );
		vals[firstcol_+outidx] = sampfunc.getValue( fzidx );
	    }

	    outidx++;
	}

	addToNrDone( 1 );
    }

    return true;
}
