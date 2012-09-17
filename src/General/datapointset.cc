/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2005
-*/

static const char* rcsID = "$Id: datapointset.cc,v 1.47 2012/07/10 13:06:02 cvskris Exp $";

#include "datapointset.h"
#include "datacoldef.h"
#include "posvecdataset.h"
#include "posprovider.h"
#include "bufstringset.h"
#include "survinfo.h"
#include "statrand.h"
#include "idxable.h"
#include "iopar.h"
#include "keystrs.h"
#include <iostream>

static const char* sKeyDPS = "Data Point Set";
const int DataPointSet::groupcol_ = 3;
#define mAddMembs(is2d,mini) \
    	  is2d_(is2d) \
    	, nrfixedcols_(is2d?(mini?2:5):(mini?1:4)) \
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


void DataPointSet::Pos::set( const Coord& c )
{
    binid_ = SI().transform( c );
    setOffs( c );
}


void DataPointSet::Pos::set( const Coord3& c )
{
    z_ = c.z;
    set( ((const Coord&)c) );
}


Coord DataPointSet::Pos::coord() const
{
    Coord sc( SI().transform(binid_) );
    sc.x += offsx_; sc.y += offsy_;
    return sc;
}


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
    if ( is2d )
	vals += pos_.nr_;
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
    grp_ = getCompacted( -1, newgrp ) ;
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


DataPointSet::DataPointSet( ::Pos::Provider& prov,
			    const ObjectSet<DataColDef>& dcds,
       			    const ::Pos::Filter* filt, bool mini )
	: PointDataPack(sKeyDPS)
	, data_(*new PosVecDataSet)
    	, mAddMembs(prov.is2D(),mini)
{
    initPVDS();
    for ( int idx=0; idx<dcds.size(); idx++ )
	data_.add( new DataColDef(*dcds[idx]) );

    mDynamicCastGet(const ::Pos::Provider3D*,p3d,&prov)
    mDynamicCastGet(const ::Pos::Provider2D*,p2d,&prov)
    mDynamicCastGet(const ::Pos::Filter3D*,f3d,filt)
    mDynamicCastGet(const ::Pos::Filter2D*,f2d,filt)

    const int nrcols = dcds.size();
    DataPointSet::DataRow dr;
    while ( prov.toNextZ() )
    {
	if ( mIsUdf(prov.curZ()) )
	    continue;
	const Coord crd( prov.curCoord() );
	dr.pos_.set( crd );
	if ( !p3d )
	    dr.pos_.nr_ = p2d->curNr();
	dr.pos_.z_ = prov.curZ();
	if ( filt )
	{
	    if ( f3d )
	    {
		if ( !f3d->includes(dr.pos_.binid_,dr.pos_.z_) )
		    continue;
	    }
	    else if ( f2d )
	    {
		if ( !f2d->includes(dr.pos_.nr_,dr.pos_.z_) )
		    continue;
	    }
	    if ( filt->hasZAdjustment() )
		dr.pos_.z_ = filt->adjustedZ( crd, dr.pos_.z_ );
	}
	dr.data_.setSize( nrcols, mUdf(float) );
	addRow( dr );
    }

    calcIdxs();
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
    BinID bid; DataPointSet::DataRow dr;
    dr.data_.setSize( bvssz - startidx );
    BinIDValueSet::Pos bvspos;
    while ( bvs.next(bvspos) )
    {
	bvs.get( bvspos, dr.pos_.binid_, vals );
	dr.pos_.z_ = vals[0];
	if ( isdps )
	{
	    dr.pos_.setBinIDOffsets( vals[1], vals[2] );
	    dr.grp_ = (short)vals[3];
	    if ( is2d_ )
		dr.pos_.nr_ = mNINT32(vals[4]);
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
	DataRow dr( dps.dataRow(irow) );
	bool inc = true;
	if ( typ == -1 )
	    inc = filt.includes( dr.pos_.coord(), dr.pos_.z_ );
	else if ( f3d )
	    inc = f3d->includes( dr.pos_.binID(), dr.pos_.z_ );
	else if ( f2d )
	    inc = f2d->includes( dr.pos_.nr_, dr.pos_.z_ );

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
    if ( !minimal_ )
    {
	data_.add( new DataColDef("X Offset") );
	data_.add( new DataColDef("Y Offset") );
	data_.add( new DataColDef("Selection status") );
    }
    if ( is2d_ )
	data_.add( new DataColDef(sKey::TraceNr) );
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


void DataPointSet::setEmpty()
{
    data_.setEmpty();
    bvsidxs_.erase();
    initPVDS();
}


void DataPointSet::clearData()
{
    bivSet().empty();
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
    Pos p( binID(rid), vals[0] );
    if ( !minimal_ )
	p.setBinIDOffsets( vals[1], vals[2] );
    if ( is2d_ )
	p.nr_ = mNINT32(vals[nrfixedcols_-1]);
    return p;
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


int DataPointSet::trcNr( DataPointSet::RowID rid ) const
{
    mChkRowID(rid,0); if ( !is2d_ ) return 0;
    const float fnr = bivSet().getVal( bvsidxs_[rid], nrfixedcols_-1 );
    return mNINT32(fnr);
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
    vals[cid + nrfixedcols_] = mUdf(float);
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
    bivSet().getVals( bvsidxs_[rid] )[ groupcol_ ] = grp;
}


void DataPointSet::setSelected( DataPointSet::RowID rid, int selgrp )
{
    if ( minimal_ ) return;
    mChkRowID(rid,);
    short grp = (short)group( rid );
    bivSet().getVals( bvsidxs_[rid] )[ groupcol_ ] = getCompacted( selgrp, grp);
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
    BinIDValueSet::Pos bvspos = bivSet().findFirst( dr.pos_.binid_ );
    while ( bvspos.valid() && bivSet().getBinID(bvspos) == dr.pos_.binid_ )
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
    const float rowsz = sKb2MbFac() * (twointsz + bivSet().nrVals()*sizeof(float));
    const int nrrows = bivSet().totalSize();
    return nrrows * (rowsz + twointsz);
}


void DataPointSet::dumpInfo( IOPar& iop ) const
{
    BufferString typstr( "PointSet (" );
    typstr += is2d_ ? "2D" : "3D";
    typstr += minimal_ ? ",minimal)" : ")";
    iop.set( sKey::Type, typstr );
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
    TypeSet<BinIDValueSet::Pos> torem;
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
    TypeSet<BinIDValueSet::Pos> torem;
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

    mGetIdxArr(RowID,idxs,mysz);
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
	Stats::RandGen::subselect( idxs, activesz, maxsz );
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


DataPointSet::RowID DataPointSet::getRowID( BinIDValueSet::Pos bvspos ) const
{
    int rid = -1;
    return IdxAble::findPos( bvsidxs_.arr(), bvsidxs_.size(), bvspos, -1, rid )
	? rid : -1;
}


DataPointSet::RowID DataPointSet::find( const DataPointSet::Pos& dpos ) const
{
    BinIDValueSet::Pos bpos = bivSet().findFirst( dpos.binid_ );
    bivSet().prev( bpos );
    while ( bivSet().next(bpos) )
    {
	if ( bivSet().getBinID(bpos) != dpos.binid_ )
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
	res = SI().xyInFeet()&&SI().zInMeter() ? dz*mToFeetFactor \
					       : dz*mFromFeetFactor; \
} \
else \
{ \
    res = dz * SI().zFactor(); \
    if ( SI().xyInFeet() ) \
	res *= mToFeetFactor; \
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
	const float dist = poscoord.distTo( targetpos );
	if ( dist < maxdist  && dist < mindist )
	{
	    resrowidx=rowidx;
	    mindist = dist;
	}
    }

    return resrowidx;
}


DataPointSet::RowID DataPointSet::findFirst( const BinID& bid ) const
{
    BinIDValueSet::Pos bpos = bivSet().findFirst( bid );
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
