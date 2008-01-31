/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2005
-*/

static const char* rcsID = "$Id: datapointset.cc,v 1.2 2008-01-31 06:33:28 cvsnanne Exp $";

#include "datapointset.h"
#include "datacoldef.h"
#include "posvecdataset.h"
#include "bufstringset.h"
#include "survinfo.h"

const int DataPointSet::nrfixedcols_ = 4;
const int DataPointSet::groupcol_ = 3;
static const char* sKeyDPS = "Data Point Set";


DataPointSet::Pos::Pos( const Coord& c, float _z )
    : binid_(SI().transform(c))
    , z_(_z)
{
    setOffs( c );
}


DataPointSet::Pos::Pos( const Coord3& c )
    : binid_(SI().transform(c))
    , z_(c.z)
{
    setOffs( c );
}


void DataPointSet::Pos::setOffs( const Coord& c )
{
    const Coord sc( SI().transform(binid_) );
    offsx_ = c.x - sc.x;
    offsy_ = c.y - sc.y;
}


Coord DataPointSet::Pos::coord() const
{
    Coord sc( SI().transform(binid_) );
    sc.x += offsx_; sc.y += offsy_;
    return sc;
}


DataPointSet::DataPointSet( const TypeSet<DataPointSet::DataRow>& pts,
			    const ObjectSet<DataColDef>& dcds )
	: PointDataPack(sKeyDPS)
	, data_(*new PosVecDataSet)
{
    initPVDS();
    init( pts, dcds );
}


DataPointSet::DataPointSet( const TypeSet<DataPointSet::DataRow>& pts,
			    const BufferStringSet& nms )
	: PointDataPack(sKeyDPS)
	, data_(*new PosVecDataSet)
{
    initPVDS();
    ObjectSet<DataColDef> dcds;
    for ( int idx=0; idx<nms.size(); idx++ )
	dcds += new DataColDef( nms.get(idx) );
    init( pts, dcds );
}


DataPointSet::DataPointSet( const PosVecDataSet& pdvs, bool hasoffs,
       			    bool hasgrp	)
	: PointDataPack(sKeyDPS)
	, data_(*new PosVecDataSet)
{
    initPVDS();

    const BinIDValueSet& bvs = pdvs.data();
    const int bvssz = bvs.nrVals();
    int startidx = hasoffs ? 3 : 1; if ( hasgrp ) startidx++;
    if ( bvssz < startidx ) return;

    for ( int idx=startidx; idx<bvssz; idx++ )
	data_.add( new DataColDef(pdvs.colDef(idx)) );

    float* vals = new float [ bvssz ];
    BinID bid; DataPointSet::DataRow dr;
    dr.data_.setSize( bvssz - startidx );
    BinIDValueSet::Pos bvspos;
    while ( bvs.next(bvspos) )
    {
	bvs.get( bvspos, dr.pos_.binid_, vals );
	dr.pos_.z_ = vals[0];
	if ( hasoffs )
	    { dr.pos_.offsx_ = vals[1]; dr.pos_.offsy_ = vals[2]; }
	for ( int idx=startidx; idx<bvssz; idx++ )
	    dr.data_[idx-startidx] = vals[idx];
	addRow( dr );
    }

    calcIdxs();
}


DataPointSet::DataPointSet( const DataPointSet& dps )
	: PointDataPack(sKeyDPS)
	, data_(*new PosVecDataSet)
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
    }
    return *this;
}


void DataPointSet::initPVDS()
{
    data_.add( new DataColDef("X Offset") );
    data_.add( new DataColDef("Y Offset") );
    data_.add( new DataColDef("Selection status") );
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
    BinIDValueSet::Pos bvspos;
    while ( bivSet().next(bvspos) )
	bvsidxs_ += bvspos;
}


int DataPointSet::nrCols() const
{
    return data_.nrCols() - nrfixedcols_;
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


const DataColDef& DataPointSet::colDef( DataPointSet::ColID cid ) const
{
    return data_.colDef( nrfixedcols_ + cid );
}


DataPointSet::ColID DataPointSet::indexOf( const char* nm ) const
{
    if ( !nm || !*nm ) return -1;
    const int nrcols = nrCols();
    for ( int idx=0; idx<nrcols; idx++ )
    {
	if ( !strcmp(nm,colName(idx)) )
	    return idx;
    }
    return -1;
}


const BinIDValueSet& DataPointSet::bivSet() const
{
    return data_.data();
}


int DataPointSet::size() const
{
    return bvsidxs_.size();
}


DataPointSet::Pos DataPointSet::pos( DataPointSet::RowID rid ) const
{
    mChkRowID(rid,Pos());
    const float* vals = bivSet().getVals( bvsidxs_[rid] );
    return Pos( binID(rid), vals[0], vals[1], vals[2] );
}


DataPointSet::DataRow DataPointSet::dataRow( DataPointSet::RowID rid ) const
{
    mChkRowID(rid,DataRow());

    const float* vals = bivSet().getVals( bvsidxs_[rid] );
    const int nrvals = bivSet().nrVals();

    DataRow dr( Pos(binID(rid),vals[0],vals[1],vals[2]) );
    dr.grp_ = (short)mNINT(vals[3]);
    for ( int idx=nrfixedcols_; idx<nrvals; idx++ )
	dr.data_ += vals[idx];

    return dr;
}


BinID DataPointSet::binID( DataPointSet::RowID rid ) const
{
    mChkRowID(rid,BinID(0,0));
    return bivSet().getBinID( bvsidxs_[rid] );
}


Coord DataPointSet::coord( DataPointSet::RowID rid ) const
{
    mChkRowID(rid,Coord::udf());
    Pos p( pos(rid) );
    return p.coord();
}


float DataPointSet::z( DataPointSet::RowID rid ) const
{
    mChkRowID(rid,mUdf(float));
    Pos p( pos(rid) );
    return p.z();
}


float DataPointSet::value( DataPointSet::ColID cid,
			   DataPointSet::RowID rid ) const
{
    mChkColID(cid,mUdf(float));
    mChkRowID(rid,mUdf(float));
    return bivSet().getVal( bvsidxs_[rid], cid + nrfixedcols_ );
}


unsigned short DataPointSet::group( DataPointSet::RowID rid ) const
{
    mChkRowID(rid,0);
    const float v = bivSet().getVal( bvsidxs_[rid], groupcol_ );
    return (unsigned short)((v < -0.5 ? -v : v)+.5);
}


bool DataPointSet::selected( DataPointSet::RowID rid ) const
{
    mChkRowID(rid,0);
    return bivSet().getVal( bvsidxs_[rid], groupcol_ ) > 0.5;
}


void DataPointSet::setGroup( DataPointSet::RowID rid, unsigned short newgrp )
{
    mChkRowID(rid,);
    short grp = selected(rid) ? newgrp : -newgrp;
    bivSet().getVals( bvsidxs_[rid] )[ groupcol_ ] = grp;
}


void DataPointSet::setSelected( DataPointSet::RowID rid, bool sel )
{
    mChkRowID(rid,);
    short grp = (short)group( rid );
    if ( (!sel && grp > 0) || (sel && grp < 0) ) grp = -grp;
    bivSet().getVals( bvsidxs_[rid] )[ groupcol_ ] = grp;
}


void DataPointSet::setInactive( DataPointSet::RowID rid, bool sel )
{
    mChkRowID(rid,);
    bivSet().getVals( bvsidxs_[rid] )[ groupcol_ ] = 0;
}


void DataPointSet::addRow( const DataPointSet::DataRow& dr )
{
    const int nrvals = nrCols();
    BinIDValues bivs( dr.pos_.binid_, nrvals + nrfixedcols_ );
    bivs.value(0) = dr.pos_.z_;
    bivs.value(1) = dr.pos_.offsx_;
    bivs.value(2) = dr.pos_.offsy_;
    bivs.value(3) = dr.grp_;
    for ( int idx=0; idx<nrvals; idx++ )
	bivs.value(nrfixedcols_+idx) = dr.data_[idx];
    bivSet().add( bivs );
}


void DataPointSet::getAuxInfo( DataPointSet::RowID rid, IOPar& iop ) const
{
    //TODO
}


float DataPointSet::nrKBytes() const
{
    const int twointsz = 2 * sizeof(int);
    const float rowsz = kbfac_ * (twointsz + bivSet().nrVals()*sizeof(float));
    const int nrrows = bivSet().totalSize();
    return bivSet().totalSize() * (rowsz + twointsz);
}


void DataPointSet::dumpInfo( IOPar& iop ) const
{
    //TODO
}
