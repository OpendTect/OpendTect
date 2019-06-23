/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : October 2011
-*/

#include "uibodyregiondlg.h"

#include "arrayndimpl.h"
#include "embodytr.h"
#include "emfault3d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "emregion.h"
#include "emsurfacetr.h"
#include "explfaultsticksurface.h"
#include "explplaneintersection.h"
#include "executor.h"
#include "marchingcubes.h"
#include "polygon.h"
#include "polyposprovider.h"
#include "positionlist.h"
#include "sorting.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiioobjseldlg.h"
#include "uimsg.h"
#include "uispinbox.h"
#include "uipossubsel.h"
#include "uistepoutsel.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "varlenarray.h"
#include "od_helpids.h"


#define mBelow 0
#define mAbove 1
#define mToMinInline 0
#define mToMaxInline 1
#define mToMinCrossline 2
#define mToMaxCrossline 3

#define cTypeCol 0
#define cNameCol 1
#define cSideCol 2
#define cRelLayerCol 3
#define cHorShiftUpCol 3
#define cHorShiftDownCol 4

class BodyExtractorFromHorizons : public ParallelTask
{ mODTextTranslationClass(BodyExtractorFromHorizons)
public:
BodyExtractorFromHorizons( const DBKeySet& hlist,
	const TypeSet<char>& sides, const TypeSet<float>& horshift,
	const TrcKeyZSampling& cs, Array3D<float>& res,
	const ODPolygon<float>& p )
    : res_(res)
    , tkzs_(cs)
    , plg_(p)
{
    res_.setAll( 1 );
    SilentTaskRunnerProvider tprov;
    for ( int idx=0; idx<hlist.size(); idx++ )
    {
	EM::Object* emobj =
		EM::Hor3DMan().loadIfNotFullyLoaded( hlist[idx], tprov );
	mDynamicCastGet(EM::Horizon3D*,hor,emobj);
	if ( !hor ) continue;

	hor->ref();
	hors_ += hor;
	hsides_ += sides[idx];
	horshift_ += horshift[idx];
    }
}


~BodyExtractorFromHorizons()	{ deepUnRef( hors_ ); }
od_int64 nrIterations() const   { return tkzs_.nrInl()*tkzs_.nrCrl(); }
uiString message() const	{ return tr("Extracting body from horizons"); }

bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    const int crlsz = tkzs_.nrCrl();
    if ( !crlsz ) return true;

    const int zsz = tkzs_.nrZ();
    const int horsz = hors_.size();
    const bool usepolygon = !plg_.isEmpty();

    for ( int idx=mCast(int,start); idx<=stop && shouldContinue();
						    idx++, addToNrDone(1) )
    {
	const int inlidx = idx/crlsz;
	const int crlidx = idx%crlsz;
	const BinID bid = tkzs_.hsamp_.atIndex(inlidx,crlidx);
	if ( bid.inl()==tkzs_.hsamp_.start_.inl() ||
	     bid.inl()==tkzs_.hsamp_.stop_.inl()  ||
	     bid.crl()==tkzs_.hsamp_.start_.crl() ||
	     bid.crl()==tkzs_.hsamp_.stop_.crl() )
	    continue;/*Extended one layer*/

	if ( usepolygon )
	{
	    Geom::Point2D<float> pt( mCast(float,bid.inl()),
				     mCast(float,bid.crl()) );
	    if ( !plg_.isInside(pt,true,0.01) )
		continue;
	}

	for ( int idz=1; idz<zsz-1; idz++ ) /*Extended one layer*/
	{
	    const float curz = tkzs_.zsamp_.atIndex( idz );
	    bool curzinrange = true;
	    float mindist = -1;
	    for ( int idy=0; idy<horsz; idy++ )
	    {
		const float hz = hors_[idy]->getZ(bid) + horshift_[idy];
		if ( mIsUdf(hz) ) continue;

		const float dist = hsides_[idy]==mBelow ? curz-hz : hz-curz;
		if ( dist<0 )
		{
		    curzinrange = false;
		    break;
		}

		if ( mindist<0 || mindist>dist )
		    mindist = dist;
	    }

	    if ( curzinrange )
		res_.set( inlidx, crlidx, idz, -mindist );
	}
    }

    return true;
}

Array3D<float>&					res_;
const TrcKeyZSampling&				tkzs_;
ObjectSet<EM::Horizon3D>			hors_;
TypeSet<char>					hsides_;
TypeSet<float>					horshift_;
const ODPolygon<float>&				plg_;
};


class ImplicitBodyRegionExtractor : public ParallelTask
{ mODTextTranslationClass(ImplicitBodyRegionExtractor)
public:
ImplicitBodyRegionExtractor( const DBKeySet& surflist,
	const TypeSet<char>& sides, const TypeSet<float>& horshift,
	const TrcKeyZSampling& cs, Array3D<float>& res,
	const ODPolygon<float>& p )
    : res_(res)
    , tkzs_(cs)
    , plg_(p)
    , bidinplg_(0)
{
    res_.setAll( 1 );

    c_[0] = Geom::Point2D<float>( mCast(float,tkzs_.hsamp_.start_.inl()),
				  mCast(float,tkzs_.hsamp_.start_.crl()) );
    c_[1] = Geom::Point2D<float>( mCast(float,tkzs_.hsamp_.stop_.inl()),
				  mCast(float,tkzs_.hsamp_.start_.crl()) );
    c_[2] = Geom::Point2D<float>( mCast(float,tkzs_.hsamp_.stop_.inl()),
				  mCast(float,tkzs_.hsamp_.stop_.crl()) );
    c_[3] = Geom::Point2D<float>( mCast(float,tkzs_.hsamp_.start_.inl()),
				  mCast(float,tkzs_.hsamp_.stop_.crl()) );

    SilentTaskRunnerProvider tprov;
    for ( int idx=0; idx<surflist.size(); idx++ )
    {
	EM::Object* emobj =
		EM::Hor3DMan().loadIfNotFullyLoaded( surflist[idx], tprov );
	mDynamicCastGet( EM::Horizon3D*, hor, emobj );
	if ( hor )
	{
	    hor->ref();
	    hors_ += hor;
	    hsides_ += sides[idx];
	    horshift_ += horshift[idx];
	}
	else
	{
	    mDynamicCastGet( EM::Fault3D*, emflt, emobj );
	    Geometry::FaultStickSurface* flt =
		emflt ? emflt->geometry().geometryElement() : 0;
	    if ( !flt ) continue;

	    emflt->ref();
	    Geometry::ExplFaultStickSurface* efs =
		new Geometry::ExplFaultStickSurface(0,SI().zScale());
	    efs->setCoordList( new Coord3ListImpl, new Coord3ListImpl );
	    efs->setSurface( flt );
	    efs->update( true, 0 );
	    expflts_ += efs;
	    fsides_ += sides[idx];
	    flts_ += emflt;

	    computeFltOuterRange( *flt, sides[idx] );
	}
    }

    computeHorOuterRange();
    if ( !plg_.isEmpty() )
    {
	bidinplg_ = new Array2DImpl<unsigned char>(tkzs_.nrInl(),tkzs_.nrCrl());

	const TrcKeySampling& hrg = tkzs_.hsamp_;
	TrcKeySamplingIterator iter( hrg );
	do
	{
	    const TrcKey trk( iter.curTrcKey() );
	    const int inl = trk.lineNr();
	    const int crl = trk.trcNr();
	    const int inlidx = hrg.lineIdx( inl );
	    const int crlidx = hrg.trcIdx( crl );
	    bidinplg_->set( inlidx, crlidx, plg_.isInside(
		    Geom::Point2D<float>( mCast(float,inl),
					 mCast(float,crl) ),true,0.01 ) );
	} while ( iter.next() );
    }
}


~ImplicitBodyRegionExtractor()
{
    delete bidinplg_;
    deepUnRef( hors_ );
    deepErase( expflts_ );
    deepUnRef( flts_ );
}

od_int64 nrIterations() const	{ return tkzs_.nrZ(); }
uiString message() const	{ return tr("Extracting implicit body"); }

bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    const int lastinlidx = tkzs_.nrInl()-1;
    const int lastcrlidx = tkzs_.nrCrl()-1;
    const int lastzidx = tkzs_.nrZ()-1;
    const int horsz = hors_.size();
    const int fltsz = fsides_.size();
    const bool usepolygon = !plg_.isEmpty();

    ObjectSet<Geometry::ExplPlaneIntersection> intersects;
    for ( int idx=0; idx<fltsz; idx++ )
    {
	intersects += new Geometry::ExplPlaneIntersection();
	intersects[idx]->setCoordList( new Coord3ListImpl, new Coord3ListImpl );
	intersects[idx]->setShape( *expflts_[idx] );
    }

    TypeSet<Coord3> corners;
    if ( fltsz )
    {
	corners += Coord3( SI().transform(tkzs_.hsamp_.start_), 0 );
	const BinID cbid0( tkzs_.hsamp_.start_.inl(),tkzs_.hsamp_.stop_.crl() );
	corners += Coord3( SI().transform(cbid0), 0 );
	corners += Coord3( SI().transform(tkzs_.hsamp_.stop_), 0 );
	const BinID cbid1( tkzs_.hsamp_.stop_.inl(),tkzs_.hsamp_.start_.crl() );
	corners += Coord3( SI().transform(cbid1), 0 );
    }
    const int cornersz = corners.size();

    for ( int idz=mCast(int,start); idz<=stop && shouldContinue();
	    idz++, addToNrDone(1) )
    {
	if ( !idz || idz==lastzidx )
	    continue;

	const double curz = tkzs_.zsamp_.atIndex( idz );
	bool outsidehorrg = false;
	for ( int hidx=0; hidx<horoutrgs_.size(); hidx++ )
	{
	    if ( horoutrgs_[hidx].includes(curz,false) )
	    {
		outsidehorrg = true;
		break;
	    }
	}

	if ( outsidehorrg )
	    continue;

	if ( fltsz )
	{
	    for ( int cidx=0; cidx<cornersz; cidx++ )
		corners[cidx].z_ = curz;

	    for ( int fidx=0; fidx<fltsz; fidx++ )
	    {
		if ( intersects[fidx]->nrPlanes() )
		    intersects[fidx]->setPlane( 0, Coord3(0,0,1), corners );
		else
		    intersects[fidx]->addPlane( Coord3(0,0,1), corners );

		intersects[fidx]->update( false, 0 );
	    }
	}

	ObjectSet<ODPolygon<float> > polygons;
	for ( int idx=0; idx<fltsz; idx++ )
	{
	    polygons += new ODPolygon<float>;
	    getPolygon( idx, intersects[idx], *polygons[idx] );
	}

	const TrcKeySampling& hrg = tkzs_.hsamp_;
	TrcKeySamplingIterator iter( hrg );
	do
	{
	    const TrcKey trk( iter.curTrcKey() );
	    const int inl = trk.lineNr();
	    const int crl = trk.trcNr();
	    const int inlidx = hrg.lineIdx( inl );
	    const int crlidx = hrg.trcIdx( crl );
	    if (!inlidx || !crlidx || inlidx==lastinlidx || crlidx==lastcrlidx)
		continue;

	    if ( usepolygon && !bidinplg_->get(inlidx,crlidx) )
		continue;

	    bool infltrg = true;
	    const BinID& bid = trk.position();
	    for ( int idy=0; idy<fltsz; idy++ )
	    {
		infltrg = inFaultRange(bid,idy,*polygons[idy]);
		if ( !infltrg ) break;
	    }
	    if ( !infltrg )
		continue;

	    if ( !horsz )
	    {
		res_.set( inlidx, crlidx, idz, -1 );
		continue;
	    }

	    float minz = mUdf(float);
	    float maxz = mUdf(float);
	    for ( int idy=0; idy<horsz; idy++ )
	    {
		const float hz = hors_[idy]->getZ(bid) + horshift_[idy];
		if ( mIsUdf(hz) ) continue;

		if ( hsides_[idy]==mBelow )
		{
		    if ( mIsUdf(minz) || minz<hz )
			minz = hz;
		}
		else if ( mIsUdf(maxz) || maxz>hz )
		    maxz = hz;
	    }

	    if ( mIsUdf(minz) && mIsUdf(maxz) )
		continue;

	    if ( mIsUdf(minz) ) minz = tkzs_.zsamp_.start;
	    if ( mIsUdf(maxz) ) maxz = tkzs_.zsamp_.stop;
	    if ( minz>=maxz )
		continue;

	    double val = curz < minz ? minz - curz :
		( curz > maxz ? curz - maxz : -mMIN(curz-minz, maxz-curz) );
	    res_.set( inlidx, crlidx, idz, (float) val );
	} while ( iter.next() );

	deepErase( polygons );
    }

    deepErase( intersects );

    return true;
}

void getPolygon( int curidx, Geometry::ExplPlaneIntersection* epi,
                 ODPolygon<float>& poly )
{
   if (  !epi || !epi->getPlaneIntersections().size() )
	return;

    const char side = fsides_[curidx];
    const TypeSet<Coord3>& crds = epi->getPlaneIntersections()[0].knots_;
    const TypeSet<int>& conns = epi->getPlaneIntersections()[0].conns_;
    const int sz = crds.size();
    if ( sz<2 )
	return;

    TypeSet<int> edgeids;
    for ( int idx=0; idx<conns.size(); idx++ )
    {
	const int index = conns[idx];
	if ( index == -1 )
	    continue;

	int count = 0;
	for ( int cidx=0; cidx<conns.size(); cidx++ )
	{
	    const int cindex = conns[cidx];
	    if ( cindex == -1 )
		continue;

	    if ( index == cindex )
		count++;
	}

	if ( count == 1 )
	    edgeids += index;
    }

    if ( edgeids.isEmpty() )
	return;

    TypeSet<int> crdids;
    crdids += edgeids[0];
    edgeids -= edgeids[0];

    while ( !edgeids.isEmpty() && crdids.size() < sz )
    {
	bool foundmatch = false;
	for ( int idx=0; idx<conns.size()-1; idx++ )
	{
	    const int index = conns[idx];
	    const int nextindex = conns[idx+1];
	    if ( index == -1 || nextindex == -1 )
		continue;

	    const int lastidx = crdids.last();
	    if ( index == lastidx && !crdids.isPresent(nextindex) )
	    {
		crdids += nextindex;
		foundmatch = true;
		break;
	    }
	    else if ( nextindex == lastidx && !crdids.isPresent(index) )
	    {
		crdids += index;
		foundmatch = true;
		break;
	    }
	}

	if ( foundmatch == false  )
	{
	    if ( !crdids.isPresent(edgeids[0]) )
		crdids += edgeids[0];

	    edgeids -= edgeids[0];
	}
    }


    mAllocVarLenArr(int,ids,sz);
    TypeSet< Geom::Point2D<float> > bidpos;
    for ( int idx=0; idx<sz; idx++ )
    {
	ids[idx] = crdids[idx];
	BinID bid = SI().transform( crds[idx].getXY() );
	bidpos += Geom::Point2D<float>( mCast(float,bid.inl()),
					mCast(float,bid.crl()) );
    }

    poly.setClosed( true );
    for ( int idx=0; idx<sz; idx++ )
	poly.add( bidpos[ids[idx]] );

    const bool ascending = poly.data()[sz-1].y_ > poly.data()[0].y_;

    if ( (side==mToMinInline && ascending) ||
	 (side==mToMaxCrossline && ascending) )
    {
	poly.add( c_[2] );
	poly.add( c_[3] );
	poly.add( c_[0] );
    }
    else if ( (side==mToMinInline && !ascending) ||
	      (side==mToMinCrossline && !ascending) )
    {
	poly.add( c_[1] );
	poly.add( c_[0] );
	poly.add( c_[3] );
    }
    else if ( (side==mToMaxInline && ascending) ||
	      (side==mToMinCrossline && ascending) )
    {
	poly.add( c_[2] );
	poly.add( c_[1] );
	poly.add( c_[0] );
    }
    else
    {
	poly.add( c_[1] );
	poly.add( c_[2] );
	poly.add( c_[3] );
    }
}


bool inFaultRange( const BinID& pos, int curidx, ODPolygon<float>& poly )
{
    const char side = fsides_[curidx];
    const int ic = side==mToMinInline || side==mToMaxInline
	? pos.inl() : pos.crl();
    if ( outsidergs_[curidx].includes(ic,false) )
	return false;

    if ( insidergs_[curidx].includes(ic,false) )
	return true;

    if ( poly.isEmpty() )
	return true;

    return poly.isInside(
	Geom::Point2D<float>( mCast(float,pos.inl()),
			      mCast(float,pos.crl())), true, 0 );
}


void computeHorOuterRange()
{
    Interval<double> hortopoutrg(mUdf(float),mUdf(float));
    Interval<double> horbotoutrg(mUdf(float),mUdf(float));
    for ( int idx=0; idx<hors_.size(); idx++ )
    {
	const Geometry::BinIDSurface* surf =
	    hors_[idx]->geometry().geometryElement();
	const Array2D<float>* depth = surf ? surf->getArray() : 0;
	const int sz = depth ? mCast(int,depth->totalSize()) : 0;
	if ( !sz ) continue;

	const float* data = depth->getData();
	Interval<float> zrg;
	bool defined = false;
	for ( int idz=0; idz<sz; idz++ )
	{
	    if ( mIsUdf(data[idz]) )
		continue;

	    if ( !defined )
	    {
		zrg.start = zrg.stop = data[idz];
		defined = true;
	    }
	    else
		zrg.include( data[idz], false );
	}

	if ( !defined )
	    continue;

	if ( hsides_[idx]==mBelow )
	{
	    if ( hortopoutrg.isUdf() )
		hortopoutrg.set(tkzs_.zsamp_.start,zrg.start);
	    else if ( hortopoutrg.stop>zrg.start )
		hortopoutrg.stop = zrg.start;
	}
	else
	{
	    if ( horbotoutrg.isUdf() )
		horbotoutrg.set(zrg.stop,tkzs_.zsamp_.stop);
	    else if ( horbotoutrg.start<zrg.stop )
		hortopoutrg.start = zrg.stop;
	}
    }

    if ( !hortopoutrg.isUdf() && hortopoutrg.start<hortopoutrg.stop )
	horoutrgs_ += hortopoutrg;
    if ( !horbotoutrg.isUdf() && horbotoutrg.start<horbotoutrg.stop )
	horoutrgs_ += horbotoutrg;
}


void computeFltOuterRange( const Geometry::FaultStickSurface& flt, char side )
{
    TrcKeySampling hrg(false);
    for ( int idx=0; idx<flt.nrSticks(); idx++ )
    {
	const TypeSet<Coord3>* stick = flt.getStick(idx);
	if ( !stick ) continue;

	for ( int idy=0; idy<stick->size(); idy++ )
	    hrg.include( SI().transform((*stick)[idy].getXY() ) );
    }

    Interval<int> insiderg;
    Interval<int> outsiderg;

    if ( side==mToMinInline )
    {
	insiderg.set( tkzs_.hsamp_.start_.inl(), hrg.start_.inl() );
	outsiderg.set( hrg.stop_.inl(), tkzs_.hsamp_.stop_.inl() );
    }
    else if ( side==mToMaxInline )
    {
	insiderg.set( hrg.stop_.inl(), tkzs_.hsamp_.stop_.inl() );
	outsiderg.set( tkzs_.hsamp_.start_.inl(), hrg.start_.inl() );
    }
    else if ( side==mToMinCrossline )
    {
	insiderg.set( tkzs_.hsamp_.start_.crl(), hrg.start_.crl() );
	outsiderg.set( hrg.stop_.crl(), tkzs_.hsamp_.stop_.crl() );
    }
    else
    {
	insiderg.set( hrg.stop_.crl(), tkzs_.hsamp_.stop_.crl() );
	outsiderg.set( tkzs_.hsamp_.start_.crl(), hrg.start_.crl() );
    }

    insidergs_ += insiderg;
    outsidergs_ += outsiderg;
}

Array3D<float>&					res_;
const TrcKeyZSampling&				tkzs_;
Geom::Point2D<float>				c_[4];

ObjectSet<EM::Horizon3D>			hors_;
TypeSet<char>					hsides_;
TypeSet<float>					horshift_;

ObjectSet<EM::Fault3D>				flts_;
TypeSet<char>					fsides_;

ObjectSet<Geometry::ExplFaultStickSurface>	expflts_;
TypeSet< Interval<int> >			insidergs_;
TypeSet< Interval<int> >			outsidergs_;
TypeSet< Interval<double> >			horoutrgs_;
const ODPolygon<float>&				plg_;
Array2D<unsigned char>*				bidinplg_;
};


uiBodyRegionGrp::uiBodyRegionGrp( uiParent* p, const Setup& mysetup )
    : uiGroup(p,"BodyRegion Group")
    , subvolfld_(0)
    , singlehorfld_(0)
    , region3d_(*new EM::Region3D)
{
    addinlbutton_ = addcrlbutton_ = addzbutton_
		  = addhorbutton_ = addfltbutton_ = 0;

    uiPosSubSel::Setup setup( mysetup.is2d_, true );
    setup.choicetype(uiPosSubSel::Setup::RangewithPolygon)
	 .seltxt(tr("Geometry boundary")).withstep(false);

    uiGroup* lastgrp = 0;
    if ( mysetup.withareasel_ )
	lastgrp = subvolfld_ = new uiPosSubSel( this, setup );

    if ( mysetup.withsinglehor_ )
    {
	singlehorfld_ = new uiGenInput( this, uiStrings::sApply(),
		BoolInpSpec(false,tr("Single horizon wrapping"),
				  tr("Multiple horizon layers")) );
	if ( lastgrp )
	    singlehorfld_->attach( alignedBelow, lastgrp );
	singlehorfld_->valuechanged.notify(
		mCB(this,uiBodyRegionGrp,horModChg) );
	lastgrp = singlehorfld_;
    }

    uiTable::Setup tsu( 0, 3 );
    uiGroup* tblgrp = new uiGroup( this );
    if ( lastgrp )
	tblgrp->attach( alignedBelow, lastgrp );
    table_ = new uiTable( tblgrp, tsu.rowdesc(uiStrings::sBoundary())
		    .defrowlbl(true), "Sf");
    uiStringSet lbls; lbls.add(uiStrings::sType()).add(uiStrings::sName())
			  .add(tr("Region location"));
    table_->setColumnLabels( lbls );
    table_->setPrefWidth( 300 );
    table_->setColumnResizeMode( uiTable::ResizeToContents );
    table_->resizeColumnsToContents();
    table_->setTableReadOnly( true );

    uiButtonGroup* butgrp =
		new uiButtonGroup( tblgrp, "Add Buttons", OD::Vertical );
    butgrp->attach( rightOf, table_ );
    if ( mysetup.withinlcrlz_ )
    {
	addinlbutton_ = new uiPushButton( butgrp, tr("Add In-line"),
		mCB(this,uiBodyRegionGrp,addInlCrlZCB), false );
	addcrlbutton_ = new uiPushButton( butgrp, tr("Add Cross-line"),
		mCB(this,uiBodyRegionGrp,addInlCrlZCB), false );
	addzbutton_ = new uiPushButton( butgrp, tr("Add Z-slice"),
		mCB(this,uiBodyRegionGrp,addInlCrlZCB), false );
    }

    addhorbutton_ = new uiPushButton( butgrp, tr("Add Horizon"),
	    mCB(this,uiBodyRegionGrp,addSurfaceCB), false );

    if ( mysetup.withfault_ )
    {
	addfltbutton_ = new uiPushButton( butgrp, tr("Add Fault"),
		mCB(this,uiBodyRegionGrp,addSurfaceCB), false );
    }

    removebutton_ = new uiPushButton( butgrp, uiStrings::sRemove(),
	    mCB(this,uiBodyRegionGrp,removeSurfaceCB), false );
    removebutton_->setSensitive( false );

    setHAlignObj( table_ );
}


uiBodyRegionGrp::~uiBodyRegionGrp()
{}


void uiBodyRegionGrp::setRegion( const EM::Region3D& region3d )
{
    IOPar pars;
    region3d.fillPar( pars );
    region3d_.usePar( pars );
    updateTable();
}


const EM::Region3D& uiBodyRegionGrp::region() const
{ return region3d_; }


void uiBodyRegionGrp::horModChg( CallBacker* )
{
    table_->clearTable();
    table_->selectRow( 0 );
    region3d_.setEmpty();

    const bool singlehormode =
	singlehorfld_ ? singlehorfld_->getBoolValue() : false;
    uiStringSet lbls;
    lbls.add(uiStrings::sType()).add(uiStrings::sLocation());
    if ( singlehormode )
	lbls.add(tr("Relative shift up")).add(tr("Relative shift down"));

    table_->setColumnLabels( lbls );
    table_->resizeColumnsToContents();
    addhorbutton_->setSensitive( singlehormode );
    removebutton_->setSensitive( false );
}


void uiBodyRegionGrp::addInlCrlZCB( CallBacker* cb )
{
    EM::RegionBoundary* bd = 0;
    if ( cb==addinlbutton_ ) bd = new EM::RegionInlBoundary();
    if ( cb==addcrlbutton_ ) bd = new EM::RegionCrlBoundary();
    if ( cb==addzbutton_ ) bd = new EM::RegionZBoundary();

    region3d_.addBoundary( bd );
    updateTable();
}


void uiBodyRegionGrp::addSurfaceCB( CallBacker* cb )
{
    const bool isflt = addfltbutton_==cb;
    if ( !isflt && addhorbutton_!=cb )
	return;

    const bool singlehormode =
	singlehorfld_ ? singlehorfld_->getBoolValue() : false;

    PtrMan<CtxtIOObj> objio =  isflt ? mMkCtxtIOObj(EMFault3D)
				     : mMkCtxtIOObj(EMHorizon3D);
    uiIOObjSelDlg::Setup sdsu; sdsu.multisel( !singlehormode );
    PtrMan<uiIOObjSelDlg> dlg = new uiIOObjSelDlg( this, sdsu, *objio );
    if ( !dlg->go() )
	return;

    const int nrsel = dlg->nrChosen();
    for ( int idx=0; idx<nrsel; idx++ )
    {
	const DBKey& mid = dlg->chosenID( idx );
	if ( region3d_.hasBoundary(mid) )
	    continue;

	EM::RegionBoundary* bd = 0;
	if ( isflt )
	    bd = new EM::RegionFaultBoundary( mid );
	else
	    bd = new EM::RegionHor3DBoundary( mid );

	region3d_.addBoundary( bd );
    }

    updateTable();
}


void uiBodyRegionGrp::removeSurfaceCB( CallBacker* )
{
    const int currow = table_->currentRow();
    if ( currow==-1 ) return;

    if ( currow<region3d_.size() )
	region3d_.removeBoundary( currow );

    updateTable();
}


void uiBodyRegionGrp::updateTable()
{
    accept();

    const int nrboundaries = region3d_.size();
    table_->setNrRows( nrboundaries );

    const bool singlehormode =
	singlehorfld_ ? singlehorfld_->getBoolValue() : false;

    for ( int row=0; row<nrboundaries; row++ )
    {
	const EM::RegionBoundary* bd = region3d_.getBoundary( row );
	if ( !bd ) continue;

	table_->setText( RowCol(row,cTypeCol), bd->type() );
	if ( bd->hasName() )
	    table_->setText( RowCol(row,cNameCol), bd->name() );
	table_->setCellReadOnly( RowCol(row,cTypeCol), true );
	table_->setCellReadOnly( RowCol(row,cNameCol), true );

	mDynamicCastGet(const EM::RegionHor3DBoundary*,horbd,bd)
	if ( singlehormode && horbd )
	{
	    const uiString unt = SI().zDomain().unitStr();

	    uiSpinBox* shiftupfld = new uiSpinBox( 0, 0, "Shift up" );
	    shiftupfld->setSuffix(unt);
	    table_->setCellObject( RowCol(row,cHorShiftUpCol), shiftupfld );

	    uiSpinBox* shiftdownfld = new uiSpinBox( 0, 0, "Shift down" );
	    shiftdownfld->setSuffix(unt);
	    table_->setCellObject( RowCol(row,cHorShiftDownCol), shiftdownfld );

	    continue;
	}

	mDynamicCastGet(const EM::RegionInlBoundary*,inlbd,bd)
	mDynamicCastGet(const EM::RegionCrlBoundary*,crlbd,bd)
	mDynamicCastGet(const EM::RegionZBoundary*,zbd,bd)
	if ( inlbd || crlbd || zbd )
	{
	    uiSpinBox* iczbox = new uiSpinBox( 0, 0, "Inl/Crl/Z" );
	    if ( zbd )
		iczbox->setSuffix( SI().zDomain().unitStr() );
	    if ( inlbd )
	    {
		iczbox->setInterval( SI().inlRange() );
		iczbox->setValue( inlbd->inl_ );
	    }
	    if ( crlbd )
	    {
		iczbox->setInterval( SI().crlRange() );
		iczbox->setValue( crlbd->crl_ );
	    }
	    if ( zbd )
	    {
		StepInterval<float> zrg = SI().zRange();
		zrg.scale( (float)SI().zDomain().userFactor() ); zrg.step = 1.f;
		iczbox->setInterval( zrg );
		iczbox->setValue( zbd->z_ );
	    }

	    table_->setCellObject( RowCol(row,cNameCol), iczbox );
	}

	uiStringSet sidenms;
	bd->getSideStrs( sidenms );
	uiComboBox* sidesel = new uiComboBox( 0, sidenms, 0 );
	const int side = (int)bd->getSide();
	sidesel->setCurrentItem( side );
	table_->setCellObject( RowCol(row,cSideCol), sidesel );
    }

    removebutton_->setSensitive( !region3d_.isEmpty() );
}


#define mRetErr(msg)  { uiMSG().error( msg ); return false; }
bool uiBodyRegionGrp::accept()
{
    if ( region3d_.isEmpty() )
	mRetErr(uiStrings::phrPlsSelectAtLeastOne(
		    uiStrings::sBoundary().toLower()));

    for ( int idx=0; idx<region3d_.size(); idx++ )
    {
	EM::RegionBoundary* bd = region3d_.getBoundary( idx );
	mDynamicCastGet(uiComboBox*,selbox,
		table_->getCellObject(RowCol(idx,cSideCol)));
	if ( !bd || !selbox ) continue;

	const int selside = selbox->currentItem();
	bd->setSide( selside );

	mDynamicCastGet(EM::RegionInlBoundary*,inlbd,bd)
	mDynamicCastGet(EM::RegionCrlBoundary*,crlbd,bd)
	mDynamicCastGet(EM::RegionZBoundary*,zbd,bd)
	if ( inlbd || crlbd || zbd )
	{
	    mDynamicCastGet(uiSpinBox*,sb,
		table_->getCellObject(RowCol(idx,cNameCol)))
	    if ( !sb ) break;
	    if ( inlbd ) inlbd->inl_ = sb->getIntValue();
	    if ( crlbd ) crlbd->crl_ = sb->getIntValue();
	    if ( zbd ) zbd->z_ = sb->getFValue() / SI().zDomain().userFactor();
	}
    }

    if ( subvolfld_ )
    {
	TrcKeyZSampling tkzs = subvolfld_->envelope();
	tkzs.expand( 1, 1, 1 );
    }

    return true;
}



// uiBodyRegionDlg
uiBodyRegionDlg::uiBodyRegionDlg( uiParent* p, bool is2d )
    : uiDialog( p, Setup(tr("Region constructor"),tr("Boundary settings"),
			 mODHelpKey(mBodyRegionDlgHelpID) ) )
{
    uiBodyRegionGrp::Setup grpsetup(is2d);
    grpsetup.withareasel(true).withinlcrlz(false).withsinglehor(true);
    grp_ = new uiBodyRegionGrp( this, grpsetup );

    outputfld_ = new uiIOObjSel( this, mWriteIOObjContext(EMBody) );
    outputfld_->attach( alignedBelow, grp_ );
}


uiBodyRegionDlg::~uiBodyRegionDlg()
{}


DBKey uiBodyRegionDlg::getBodyMid() const
{
    return outputfld_->key();
}


bool uiBodyRegionDlg::acceptOK()
{
    if ( !outputfld_->ioobj() )
	return false;

    if ( createImplicitBody() )
    {
	uiString msg = tr("The body %1 created successfully")
	             .arg(outputfld_->getInput());
	uiMSG().message( msg );
    }

    return false; //Make the dialog stay.
}



bool uiBodyRegionDlg::createImplicitBody()
{
/*
    mDeclareAndTryAlloc( Array3DImpl<float>*, arr,
	    Array3DImpl<float> (cs.nrInl(),cs.nrCrl(),cs.nrZ()) );
    if ( !arr )
	mRetErrDelHoridx(tr("Can not allocate disk space to create region."))

    uiTaskRunner taskrunner( this );
    ODPolygon<float> dummy;
    mDynamicCastGet(Pos::PolyProvider3D*,plgp,subvolfld_->curProvider());

    if ( hasfaults )
    {
	ImplicitBodyRegionExtractor ext( surfacelist_, sides, horshift, cs,
		*arr, plgp ? plgp->polygon() : dummy );

	if ( !TaskRunner::execute( &taskrunner, ext ) )
	    mRetErrDelHoridx(tr("Extracting body region failed."))
    }
    else
    {
	BodyExtractorFromHorizons ext( surfacelist_, sides, horshift, cs, *arr,
		plgp ? plgp->polygon() : dummy );
	if ( !TaskRunner::execute( &taskrunner, ext ) )
	    mRetErrDelHoridx(tr("Extracting body from horizons failed."))
    }

    RefMan<EM::MarchingCubesSurface> emcs =
	new EM::MarchingCubesSurface(EM::BodyMan());

    emcs->surface().setVolumeData( 0, 0, 0, *arr, 0, &taskrunner);
    emcs->setInlSampling(
	    SamplingData<int>(cs.hsamp_.start_.inl(),cs.hsamp_.step_.inl()));
    emcs->setCrlSampling(
	    SamplingData<int>(cs.hsamp_.start_.crl(),cs.hsamp_.step_.crl()));
    emcs->setZSampling(SamplingData<float>(cs.zsamp_.start,cs.zsamp_.step));

    emcs->setDBKey( outputfld_->key() );
    emcs->setName( outputfld_->getInput() );
    emcs->setFullyLoaded( true );
    emcs->setChangedFlag();

    EM::BodyMan().addObject( emcs );
    PtrMan<Executor> exec = emcs->saver();
    if ( !exec )
	mRetErrDelHoridx( uiStrings::phrSaveBodyFail() )

    DBKey key = emcs->dbKey();
    PtrMan<IOObj> ioobj = key.getIOObj();
    if ( !ioobj->pars().find( sKey::Type() ) )
    {
	ioobj->pars().set( sKey::Type(), emcs->getTypeStr() );
	const auto uirv = ioobj->commitChanges();
	if ( !uirv.isOK() )
	    mRetErr( uirv ) )
    }

    if ( !TaskRunner::execute(&taskrunner,*exec) )
	mRetErrDelHoridx(uiStrings::phrSaveBodyFail());
*/

    return true;
}
