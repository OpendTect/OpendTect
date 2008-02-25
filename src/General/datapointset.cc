/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2005
-*/

static const char* rcsID = "$Id: datapointset.cc,v 1.12 2008-02-25 15:05:31 cvsbert Exp $";

#include "datapointset.h"
#include "datacoldef.h"
#include "posvecdataset.h"
#include "posprovider.h"
#include "bufstringset.h"
#include "survinfo.h"
#include "iopar.h"
#include "keystrs.h"

static const char* sKeyDPS = "Data Point Set";
const int DataPointSet::groupcol_ = 3;
#define mAdd2DMembs(is2d) \
    	is2d_(is2d), nrfixedcols_(is2d?5:4)


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


void DataPointSet::Pos::set( const BinID& bid, const Coord& c )
{
    binid_ = bid;
    setOffs( c );
}


void DataPointSet::Pos::set( const BinID& bid, const Coord3& c )
{
    binid_ = bid;
    setOffs( c );
    z_ = c.z;
}


Coord DataPointSet::Pos::coord() const
{
    Coord sc( SI().transform(binid_) );
    sc.x += offsx_; sc.y += offsy_;
    return sc;
}


DataPointSet::DataPointSet( const TypeSet<DataPointSet::DataRow>& pts,
			    const ObjectSet<DataColDef>& dcds, bool is2d )
	: PointDataPack(sKeyDPS)
	, data_(*new PosVecDataSet)
    	, mAdd2DMembs(is2d)
{
    initPVDS();
    init( pts, dcds );
}


DataPointSet::DataPointSet( const TypeSet<DataPointSet::DataRow>& pts,
			    const BufferStringSet& nms, bool is2d )
	: PointDataPack(sKeyDPS)
	, data_(*new PosVecDataSet)
    	, mAdd2DMembs(is2d)
{
    initPVDS();
    ObjectSet<DataColDef> dcds;
    for ( int idx=0; idx<nms.size(); idx++ )
	dcds += new DataColDef( nms.get(idx) );
    init( pts, dcds );
}


DataPointSet::DataPointSet( ::Pos::Provider3D& prov,
			    const ObjectSet<DataColDef>& dcds,
       			    const ::Pos::Filter3D* filt )
	: PointDataPack(sKeyDPS)
	, data_(*new PosVecDataSet)
    	, mAdd2DMembs(false)
{
    initPVDS();
    for ( int idx=0; idx<dcds.size(); idx++ )
	data_.add( new DataColDef(*dcds[idx]) );

    const int nrcols = dcds.size();
    DataPointSet::DataRow dr;
    while ( prov.toNextZ() )
    {
	dr.pos_.set( prov.curBinID(), prov.curCoord() );
	dr.pos_.z_ = prov.curZ();
	if ( filt )
	{
	    if ( !filt->includes(dr.pos_.binid_,dr.pos_.z_) )
		continue;
	    dr.pos_.z_ = filt->adjustedZ( dr.pos_.coord(), dr.pos_.z_ );
	}
	dr.data_.setSize( nrcols, mUdf(float) );
	addRow( dr );
    }

    calcIdxs();
}


DataPointSet::DataPointSet( ::Pos::Provider2D& prov,
			    const ObjectSet<DataColDef>& dcds,
       			    const ::Pos::Filter2D* filt )
	: PointDataPack(sKeyDPS)
	, data_(*new PosVecDataSet)
    	, mAdd2DMembs(true)
{
    initPVDS();
    for ( int idx=0; idx<dcds.size(); idx++ )
	data_.add( new DataColDef(*dcds[idx]) );

    const int nrcols = dcds.size();
    DataPointSet::DataRow dr;
    while ( prov.toNextZ() )
    {
	const Coord crd( prov.curCoord() );
	dr.pos_.set( SI().transform(crd), crd );
	dr.pos_.z_ = prov.curZ();
	dr.pos_.nr_ = prov.curNr();
	if ( filt )
	{
	    if ( !filt->includes(dr.pos_.nr_,dr.pos_.z_) )
		continue;
	    filt->adjustedZ( dr.pos_.coord(), dr.pos_.z_ );
	}
	dr.data_.setSize( nrcols, mUdf(float) );
	addRow( dr );
    }

    calcIdxs();
}


DataPointSet::DataPointSet( const PosVecDataSet& pdvs, bool is2d )
	: PointDataPack(sKeyDPS)
	, data_(*new PosVecDataSet)
    	, mAdd2DMembs(is2d)
{
    initPVDS();

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
    BinID bid; DataPointSet::DataRow dr;
    dr.data_.setSize( bvssz - startidx );
    BinIDValueSet::Pos bvspos;
    while ( bvs.next(bvspos) )
    {
	bvs.get( bvspos, dr.pos_.binid_, vals );
	dr.pos_.z_ = vals[0];
	if ( isdps )
	{
	    dr.pos_.offsx_ = vals[1]; dr.pos_.offsy_ = vals[2];
	    dr.grp_ = (short)vals[3];
	    if ( is2d_ )
		dr.pos_.nr_ = mNINT(vals[4]);
	}
	for ( int idx=startidx; idx<bvssz; idx++ )
	    dr.data_[idx-startidx] = vals[idx];
	addRow( dr );
    }

    calcIdxs();
}


DataPointSet::DataPointSet( const DataPointSet& dps )
	: PointDataPack(sKeyDPS)
	, data_(*new PosVecDataSet)
    	, mAdd2DMembs(dps.is2d_)
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
	const_cast<int&>(nrfixedcols_) = dps.nrfixedcols_;
    }
    return *this;
}


void DataPointSet::initPVDS()
{
    data_.add( new DataColDef("X Offset") );
    data_.add( new DataColDef("Y Offset") );
    data_.add( new DataColDef("Selection status") );
    if ( is2d_ )
	data_.add( new DataColDef("Trace number") );
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


BinIDValueSet& DataPointSet::bivSet()
{
    return data_.data();
}


DataPointSet::Pos DataPointSet::pos( DataPointSet::RowID rid ) const
{
    mChkRowID(rid,Pos());
    const float* vals = bivSet().getVals( bvsidxs_[rid] );
    Pos p( binID(rid), vals[0], vals[1], vals[2] );
    if ( is2d_ )
	p.nr_ = mNINT(vals[4]);
    return p;
}


DataPointSet::DataRow DataPointSet::dataRow( DataPointSet::RowID rid ) const
{
    mChkRowID(rid,DataRow());

    const float* vals = bivSet().getVals( bvsidxs_[rid] );
    const int nrvals = bivSet().nrVals();

    DataRow dr( pos(rid) );
    dr.grp_ = (short)mNINT(vals[groupcol_]);
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
    return bivSet().getVal( bvsidxs_[rid], 0 );
}


int DataPointSet::trcNr( DataPointSet::RowID rid ) const
{
    mChkRowID(rid,0); if ( !is2d_ ) return 0;
    const float fnr = bivSet().getVal( bvsidxs_[rid], groupcol_+1 );
    return mNINT(fnr);
}


float DataPointSet::value( DataPointSet::ColID cid,
			   DataPointSet::RowID rid ) const
{
    mChkColID(cid,mUdf(float));
    mChkRowID(rid,mUdf(float));
    return bivSet().getVal( bvsidxs_[rid], cid + nrfixedcols_ );
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
    if ( is2d_ )
	bivs.value(4) = dr.pos_.nr_;
    for ( int idx=0; idx<nrvals; idx++ )
	bivs.value(nrfixedcols_+idx) = dr.data_[idx];
    bivSet().add( bivs );
}


float DataPointSet::nrKBytes() const
{
    const int twointsz = 2 * sizeof(int);
    const float rowsz = kbfac_ * (twointsz + bivSet().nrVals()*sizeof(float));
    const int nrrows = bivSet().totalSize();
    return nrrows * (rowsz + twointsz);
}


void DataPointSet::dumpInfo( IOPar& iop ) const
{
    iop.set( sKey::Type, "PointSet" );
    iop.set( "Number of rows", bivSet().totalSize() );
    const int nrcols = nrCols();
    iop.set( "Number of cols", nrcols );
    for ( int idx=0; idx<nrcols; idx++ )
	iop.set( IOPar::compKey("Col",idx), colName(idx) );
}


void DataPointSet::purgeInactive()
{
    TypeSet<BinIDValueSet::Pos> torem;
    for ( RowID irow=0; irow<size(); irow++ )
    {
	if ( inactive(irow) )
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
    TypeSet<BinIDValueSet::Pos> torem;
    for ( RowID irow=0; irow<size(); irow++ )
    {
	if ( sel != selected(irow) )
	    torem += bvsidxs_[irow];
    }
    if ( !torem.isEmpty() )
    {
	bivSet().remove( torem );
	calcIdxs();
    }
}


DataPointSet::RowID DataPointSet::findFirstCoord( const Coord& crd ) const
{
    const BinID bid = SI().transform( crd );
    BinIDValueSet::Pos firstpos = bivSet().findFirst( bid );
    int rowid = getRowID( firstpos );
    if ( rowid<0 ) return -1;

    for ( int idx=rowid; idx<size(); idx++ )
    {
	if ( binID( idx ) != bid ) break;
	if ( mIsEqual( coord(idx).x, crd.x, 1e-3 )
	   &&mIsEqual( coord(idx).y, crd.y, 1e-3 ) ) return idx;
    }

    return -1;
}
