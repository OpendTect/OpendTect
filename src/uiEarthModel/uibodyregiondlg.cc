/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : October 2011
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uibodyregiondlg.h"

#include "arrayndimpl.h"
#include "embodytr.h"
#include "emfault3d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "emsurfacetr.h"
#include "explfaultsticksurface.h"
#include "explplaneintersection.h"
#include "executor.h"
#include "ioman.h"
#include "marchingcubes.h"
#include "polygon.h"
#include "polyposprovider.h"
#include "positionlist.h"
#include "sorting.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "varlenarray.h"



#define mBelow 0
#define mAbove 1
#define mToMinInline 0
#define mToMaxInline 1
#define mToMinCrossline 2
#define mToMaxCrossline 3


static const char* collbls[] = { "Name", "Side", 0 };
#define cNameCol 0
#define cSideCol 1

class BodyExtractorFromHorizons : public ParallelTask
{
public:
BodyExtractorFromHorizons( const TypeSet<MultiID>& hlist, 
	const TypeSet<char>& sides, const CubeSampling& cs, Array3D<float>& res,
	const ODPolygon<float>& plg )
    : res_( res )
    , cs_( cs )
    , plg_( plg )	      
{
    res_.setAll( 1 );
    for ( int idx=0; idx<hlist.size(); idx++ )
    {
	EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded(hlist[idx]);
	mDynamicCastGet(EM::Horizon3D*,hor,emobj);
	if ( !hor ) continue;
	
	hor->ref();
	hors_ += hor;
	hsides_ += sides[idx];
    }
}


~BodyExtractorFromHorizons()	{ deepUnRef( hors_ ); }
od_int64 nrIterations() const   { return cs_.nrInl()*cs_.nrCrl(); }
const char* message() const	{ return "Extracting body from horizons"; }

bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    const int crlsz = cs_.nrCrl();
    if ( !crlsz ) return true;

    const int zsz = cs_.nrZ();
    const int horsz = hors_.size();
    const bool usepolygon = !plg_.isEmpty();
    
    for ( int idx=mCast(int,start); idx<=stop && shouldContinue(); 
						    idx++, addToNrDone(1) )
    {
	const int inlidx = idx/crlsz;
	const int crlidx = idx%crlsz;
	const BinID bid = cs_.hrg.atIndex(inlidx,crlidx);
	if ( bid.inl==cs_.hrg.start.inl || bid.inl==cs_.hrg.stop.inl ||
	     bid.crl==cs_.hrg.start.crl || bid.crl==cs_.hrg.stop.crl )
	    continue;/*Extended one layer*/

	if ( usepolygon && !plg_.isInside(Geom::Point2D<float>(
		     mCast(float,bid.inl), mCast(float,bid.crl)), true, 0.01) )
	    continue;

	for ( int idz=1; idz<zsz-1; idz++ ) /*Extended one layer*/
	{
    	    const double curz = cs_.zrg.atIndex( idz );
	    bool curzinrange = true;
	    float mindist = -1;
	    for ( int idy=0; idy<horsz; idy++ )
	    {
		const float hz = hors_[idy]->getZ(bid);
		if ( mIsUdf(hz) ) continue;
	    
		const float dist = (float)( hsides_[idy]==mBelow ? curz-hz : hz-curz );
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
const CubeSampling&				cs_;
ObjectSet<EM::Horizon3D>			hors_;
TypeSet<char>					hsides_;
const ODPolygon<float>&				plg_;
};


class ImplicitBodyRegionExtractor : public ParallelTask
{
public:
ImplicitBodyRegionExtractor( const TypeSet<MultiID>& surflist, 
	const TypeSet<char>& sides, const CubeSampling& cs, Array3D<float>& res,
	const ODPolygon<float>& plg )
    : res_(res)
    , cs_(cs)
    , plg_(plg)
    , bidinplg_(0)
{
    res_.setAll( 1 );

    c_[0] = Geom::Point2D<float>( mCast(float,cs_.hrg.start.inl), 
	                          mCast(float,cs_.hrg.start.crl) );
    c_[1] = Geom::Point2D<float>( mCast(float,cs_.hrg.stop.inl), 
				  mCast(float,cs_.hrg.start.crl) );
    c_[2] = Geom::Point2D<float>( mCast(float,cs_.hrg.stop.inl), 
				  mCast(float,cs_.hrg.stop.crl) );
    c_[3] = Geom::Point2D<float>( mCast(float,cs_.hrg.start.inl), 
				  mCast(float,cs_.hrg.stop.crl) );

    for ( int idx=0; idx<surflist.size(); idx++ )
    {
	EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( surflist[idx] );
	mDynamicCastGet( EM::Horizon3D*, hor, emobj );
	if ( hor )
	{
	    hor->ref();
	    hors_ += hor;
	    hsides_ += sides[idx];
	}
	else
	{
	    mDynamicCastGet( EM::Fault3D*, emflt, emobj );
	    Geometry::FaultStickSurface* flt = 
		emflt ? emflt->geometry().sectionGeometry(0) : 0;
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
	bidinplg_ = new Array2DImpl<unsigned char>(cs_.nrInl(),cs_.nrCrl());
	
	HorSamplingIterator iter( cs_.hrg );
	BinID bid;
	while( iter.next(bid) )
	{
	    const int inlidx = cs_.hrg.inlIdx(bid.inl);
	    const int crlidx = cs_.hrg.crlIdx(bid.crl);	    
	    bidinplg_->set( inlidx, crlidx, plg_.isInside(
		    Geom::Point2D<float>( mCast(float,bid.inl),
					 mCast(float,bid.crl) ),true,0.01 ) );
	}
    }
}


~ImplicitBodyRegionExtractor()
{
    delete bidinplg_;
    deepUnRef( hors_ );
    deepUnRef( flts_ );
    deepErase( expflts_ );
}

od_int64 nrIterations() const   	{ return cs_.nrZ(); }
const char* message() const		{ return "Extracting implicit body"; }

bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    const int lastinlidx = cs_.nrInl()-1;
    const int lastcrlidx = cs_.nrCrl()-1;
    const int lastzidx = cs_.nrZ()-1;
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
	corners += Coord3( SI().transform(cs_.hrg.start), 0 );
	const BinID cbid0( cs_.hrg.start.inl, cs_.hrg.stop.crl );
	corners += Coord3( SI().transform(cbid0), 0 );
	corners += Coord3( SI().transform(cs_.hrg.stop), 0 );
	const BinID cbid1( cs_.hrg.stop.inl, cs_.hrg.start.crl );
	corners += Coord3( SI().transform(cbid1), 0 );
    }
    const int cornersz = corners.size();

    for ( int idz=mCast(int,start); idz<=stop && shouldContinue(); 
						    idz++, addToNrDone(1) )
    {
	if ( !idz || idz==lastzidx )
	    continue;

	const double curz = cs_.zrg.atIndex( idz );
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
    		corners[cidx].z = curz;

	    for ( int fidx=0; fidx<fltsz; fidx++ )
	    {
		if ( intersects[fidx]->nrPlanes() )
		    intersects[fidx]->setPlane( 0, Coord3(0,0,1), corners );
		else
		    intersects[fidx]->addPlane( Coord3(0,0,1), corners );
		
		intersects[fidx]->update( false, 0 );
	    }
	}

	HorSamplingIterator iter( cs_.hrg );
	BinID bid;
	while( iter.next(bid) )
	{
	    const int inlidx = cs_.hrg.inlIdx(bid.inl);
	    const int crlidx = cs_.hrg.crlIdx(bid.crl);	    
	    if (!inlidx || !crlidx || inlidx==lastinlidx || crlidx==lastcrlidx)
		continue;

	    if ( usepolygon && !bidinplg_->get(inlidx,crlidx) )
		continue;
	    
	    bool infltrg = true;
	    for ( int idy=0; idy<fltsz; idy++ )
	    {
		infltrg = inFaultRange(bid,idy,intersects[idy]);
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
		const float hz = hors_[idy]->getZ(bid);
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
	    
	    if ( mIsUdf(minz) ) minz = cs_.zrg.start;
	    if ( mIsUdf(maxz) ) maxz = cs_.zrg.stop;
	    if ( minz>=maxz )
		continue;
	    
	    double val = curz < minz ? minz - curz : 
		( curz > maxz ? curz - maxz : -mMIN ( curz - minz,maxz - curz ) );
	    res_.set( inlidx, crlidx, idz, (float) val );
	}
    }

    deepErase( intersects );
    return true;
}

bool inFaultRange( const BinID& pos, int curidx, 
	Geometry::ExplPlaneIntersection* epi )
{
    const char side = fsides_[curidx];
    const int ic = side==mToMinInline || side==mToMaxInline ? pos.inl : pos.crl;
    if ( outsidergs_[curidx].includes(ic,false) )
	return false;

    if ( insidergs_[curidx].includes(ic,false) || 
	 !epi || !epi->getPlaneIntersections().size() )
	return true;

    const TypeSet<Coord3>& crds = epi->getPlaneIntersections()[0].knots_;
    const int sz = crds.size();
    if ( sz<2 )
	return true;

    mAllocVarLenArr(int,ids,sz);
    mAllocVarLenArr(int,inls,sz);
    TypeSet< Geom::Point2D<float> > bidpos;
    for ( int idx=0; idx<sz; idx++ )
    {
	ids[idx] = idx;
	BinID bid = SI().transform( crds[idx] );
	inls[idx] = bid.inl;
	bidpos += Geom::Point2D<float>( mCast(float,bid.inl),
					mCast(float,bid.crl) );
    }

    sort_coupled( mVarLenArr(inls), mVarLenArr(ids), sz );

    ODPolygon<float> poly;
    poly.setClosed( true );
    for ( int idx=0; idx<sz; idx++ )
	poly.add( bidpos[ids[idx]] );
	
    const bool ascending = poly.data()[sz-1].y > poly.data()[0].y;

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

    return poly.isInside(Geom::Point2D<float>( mCast(float,pos.inl),
						mCast(float,pos.crl)),true,0 );
}


void computeHorOuterRange()
{
    Interval<double> hortopoutrg(mUdf(float),mUdf(float));
    Interval<double> horbotoutrg(mUdf(float),mUdf(float));
    for ( int idx=0; idx<hors_.size(); idx++ )
    {
	const Geometry::BinIDSurface* surf = 
	    hors_[idx]->geometry().sectionGeometry(hors_[idx]->sectionID(0));
	const Array2D<float>* depth = surf ? surf->getArray() : 0;
	const int sz = depth ? mCast( int,depth->info().getTotalSz() ) : 0;
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
		hortopoutrg.set(cs_.zrg.start,zrg.start);
	    else if ( hortopoutrg.stop>zrg.start )
		hortopoutrg.stop = zrg.start;
	}
	else
	{
	    if ( horbotoutrg.isUdf() )
		horbotoutrg.set(zrg.stop,cs_.zrg.stop);
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
    HorSampling hrg(false);
    for ( int idx=0; idx<flt.nrSticks(); idx++ )
    {
	const TypeSet<Coord3>* stick = flt.getStick(idx);
	if ( !stick ) continue;

	for ( int idy=0; idy<stick->size(); idy++ )
	    hrg.include( SI().transform((*stick)[idy]) );
    }

    Interval<int> insiderg;
    Interval<int> outsiderg;
    
    if ( side==mToMinInline )
    {
	insiderg.set( cs_.hrg.start.inl, hrg.start.inl );
	outsiderg.set( hrg.stop.inl, cs_.hrg.stop.inl );
    }
    else if ( side==mToMaxInline )
    {
	insiderg.set( hrg.stop.inl, cs_.hrg.stop.inl );
	outsiderg.set( cs_.hrg.start.inl, hrg.start.inl );
    }
    else if ( side==mToMinCrossline )
    {
	insiderg.set( cs_.hrg.start.crl, hrg.start.crl );
	outsiderg.set( hrg.stop.crl, cs_.hrg.stop.crl );
    }
    else 
    {
	insiderg.set( hrg.stop.crl, cs_.hrg.stop.crl );
	outsiderg.set( cs_.hrg.start.crl, hrg.start.crl );
    }

    insidergs_ += insiderg;
    outsidergs_ += outsiderg;
}

Array3D<float>&					res_;
const CubeSampling&				cs_;
Geom::Point2D<float>				c_[4];

ObjectSet<EM::Horizon3D>			hors_;
TypeSet<char>					hsides_;

ObjectSet<EM::Fault3D>				flts_;
TypeSet<char>					fsides_;

ObjectSet<Geometry::ExplFaultStickSurface>	expflts_;
TypeSet< Interval<int> >			insidergs_;
TypeSet< Interval<int> >			outsidergs_;
TypeSet< Interval<double> >			horoutrgs_;
const ODPolygon<float>&				plg_;
Array2D<unsigned char>*				bidinplg_;
};


uiBodyRegionDlg::uiBodyRegionDlg( uiParent* p )
    : uiDialog( p, Setup("Region constructor","Boundary settings",mNoHelpID) )
{
    setCtrlStyle( DoAndStay );
    //setHelpID( "dgb:104.0.4" );

    subvolfld_ =  new uiPosSubSel( this,  uiPosSubSel::Setup( !SI().has3D(),
		true).choicetype(uiPosSubSel::Setup::RangewithPolygon).
	    	seltxt("Geometry boundary").withstep(false) );

    table_ = new uiTable( this, uiTable::Setup(4).rowgrow(true).fillrow(true)
	    .rightclickdisabled(true).selmode(uiTable::Single), "Edges" );
    table_->attach( alignedBelow, subvolfld_ );
    table_->setColumnLabels( collbls ); 
    table_->setLeftMargin( 0 );
    table_->setSelectionBehavior( uiTable::SelectRows );
    table_->setColumnResizeMode( uiTable::ResizeToContents );
    table_->setRowResizeMode( uiTable::Interactive );
    table_->setColumnStretchable( cNameCol, true );

    addhorbutton_ = new uiPushButton( this, "&Add horizon",
	    mCB(this,uiBodyRegionDlg,addSurfaceCB), false );
    addhorbutton_->attach( rightOf, table_ );
    
    addfltbutton_ = new uiPushButton( this, "&Add fault",
	    mCB(this,uiBodyRegionDlg,addSurfaceCB), false );
    addfltbutton_->attach( alignedBelow, addhorbutton_ );

    removebutton_ = new uiPushButton( this, "&Remove", 
	    mCB(this,uiBodyRegionDlg,removeSurfaceCB), false );
    removebutton_->attach( alignedBelow, addfltbutton_ );
    removebutton_->setSensitive( false );

    outputfld_ = new uiIOObjSel( this, mIOObjContext(EMBody), "Output name" );
    outputfld_->setForRead( false );
    outputfld_->attach( alignedBelow, table_ ); 
}


uiBodyRegionDlg::~uiBodyRegionDlg()
{}


void uiBodyRegionDlg::addSurfaceCB( CallBacker* cb )
{
    const bool isflt = addfltbutton_==cb;
    if ( !isflt && addhorbutton_!=cb )
	return;

    PtrMan<CtxtIOObj> objio =  isflt ? mMkCtxtIOObj(EMFault3D)
				     : mMkCtxtIOObj(EMHorizon3D);
    PtrMan<uiIOObjSelDlg> dlg = new uiIOObjSelDlg( this, *objio, 0, true );
    if ( !dlg->go() )
	return;

    for ( int idx=0; idx<dlg->nrSel(); idx++ )
    {
	const MultiID& mid = dlg->selected( idx );
	if ( surfacelist_.indexOf(mid)!=-1 )
	    continue;
	
	PtrMan<IOObj> ioobj = IOM().get( mid );
	addSurfaceTableEntry( *ioobj, isflt, 0 );
    }
}


void uiBodyRegionDlg::addSurfaceTableEntry( const IOObj& ioobj,	bool isfault, 
	char side )
{
    const int row = surfacelist_.size();
    if ( row==table_->nrRows() )
	table_->insertRows( row, 1 );

    BufferStringSet sidenms;
    if ( isfault )
    {
	sidenms.add("Between min Inline and fault");
	sidenms.add("Between max Inline and fault");
	sidenms.add("Between min Crossline and fault");
	sidenms.add("Between max Crossline and fault");
    }
    else
    {
	sidenms.add("Follow Z increase side (Below)");
	sidenms.add("Follow Z decrease side (Above)");
    }
    
    uiComboBox* sidesel = new uiComboBox( 0, sidenms, 0 );
    sidesel->setCurrentItem( side==-1 ? 0 : 1 );
    
    table_->setCellObject( RowCol(row,cSideCol), sidesel );
    table_->setText( RowCol(row,cNameCol), ioobj.name() );
    table_->setCellReadOnly( RowCol(row,cNameCol), true );
    
    surfacelist_ += ioobj.key();

    removebutton_->setSensitive( surfacelist_.size() );
}


void uiBodyRegionDlg::removeSurfaceCB( CallBacker* )
{
    const int currow = table_->currentRow();
    if ( currow==-1 ) return;

    if ( currow<surfacelist_.size() )
	surfacelist_.removeSingle( currow );

    table_->removeRow( currow );
    removebutton_->setSensitive( surfacelist_.size() );
}


#define mRetErr(msg)  { uiMSG().error( msg ); return false; }

bool uiBodyRegionDlg::acceptOK( CallBacker* cb )
{
    if ( !surfacelist_.size() )
	mRetErr("Please select at least one boundary");

    if ( outputfld_->isEmpty() )
	mRetErr("Please select choose a name for the output");

    if ( !outputfld_->commitInput() )
	return false;

    const bool res = createImplicitBody();
    if ( res )
    {
	BufferString msg = "The body ";
	msg += outputfld_->getInput();
	msg += " created successfully";
	uiMSG().message( msg.buf() );
    }

    return false; //Make the dialog stay.
}



bool uiBodyRegionDlg::createImplicitBody()
{
    MouseCursorChanger mcc( MouseCursor::Wait );

    TypeSet<char> sides;
    bool hasfaults = false;
    for ( int idx=0; idx<surfacelist_.size(); idx++ )
    {
	mDynamicCastGet(uiComboBox*, selbox, 
		table_->getCellObject(RowCol(idx,cSideCol)) );    
    	sides += mCast(char,selbox->currentItem());
	
	if ( !hasfaults )
	{
    	    RefMan<EM::EMObject> emobj = 
		EM::EMM().loadIfNotFullyLoaded( surfacelist_[idx] );
    	    mDynamicCastGet(EM::Fault3D*,emflt,emobj.ptr());
	    if ( emflt ) hasfaults = true;
	}
    }
   
    CubeSampling cs = subvolfld_->envelope();
    cs.zrg.start -= cs.zrg.step; cs.zrg.stop += cs.zrg.step;
    cs.hrg.start.inl -= cs.hrg.step.inl; cs.hrg.stop.inl += cs.hrg.step.inl;
    cs.hrg.start.crl -= cs.hrg.step.crl; cs.hrg.stop.crl += cs.hrg.step.crl;

    mDeclareAndTryAlloc( Array3DImpl<float>*, arr,
	    Array3DImpl<float> (cs.nrInl(),cs.nrCrl(),cs.nrZ()) );
    if ( !arr )
	return false;
    
    uiTaskRunner taskrunner( this );
    ODPolygon<float> dummy;
    mDynamicCastGet(Pos::PolyProvider3D*,plgp,subvolfld_->curProvider());

    if ( hasfaults )
    {
	ImplicitBodyRegionExtractor ext( surfacelist_, sides, cs, *arr, 
     		plgp ? plgp->polygon() : dummy );
    
	if ( !taskrunner.execute(ext) )
    	    mRetErr("Extracting body region failed.")
    }
    else
    {
	BodyExtractorFromHorizons ext( surfacelist_, sides, cs, *arr, 
     		plgp ? plgp->polygon() : dummy );
	if ( !taskrunner.execute(ext) )
    	    mRetErr("Extracting body from horizons failed.")
    }

    RefMan<EM::MarchingCubesSurface> emcs = 
	new EM::MarchingCubesSurface(EM::EMM());

    emcs->surface().setVolumeData( 0, 0, 0, *arr, 0, &taskrunner);
    emcs->setInlSampling(SamplingData<int>(cs.hrg.start.inl,cs.hrg.step.inl));
    emcs->setCrlSampling(SamplingData<int>(cs.hrg.start.crl,cs.hrg.step.crl));
    emcs->setZSampling(SamplingData<float>(cs.zrg.start,cs.zrg.step));

    emcs->setMultiID( outputfld_->key() );
    emcs->setName( outputfld_->getInput() );
    emcs->setFullyLoaded( true );
    emcs->setChangedFlag();

    EM::EMM().addObject( emcs );
    PtrMan<Executor> exec = emcs->saver(); 
    if ( !exec ) 
 	mRetErr( "Body saving failed" ) 

    MultiID key = emcs->multiID(); 
    PtrMan<IOObj> ioobj = IOM().get( key ); 
    if ( !ioobj->pars().find( sKey::Type() ) ) 
    { 
	ioobj->pars().set( sKey::Type(), emcs->getTypeStr() ); 
	if ( !IOM().commitChanges( *ioobj ) ) 
	    mRetErr( "Writing body to disk failed, no permision?" ) 
    } 

    if ( !taskrunner.execute(*exec) )
	mRetErr("Saving body failed");

    return true;
}
