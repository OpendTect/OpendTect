/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : October 2011
-*/

static const char* rcsID = "$Id: uibodyregiondlg.cc,v 1.3 2011-11-01 05:00:56 cvsraman Exp $";

#include "uibodyregiondlg.h"

#include "arrayndimpl.h"
#include "emsurfacetr.h"
#include "embodytr.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "emhorizon3d.h"
#include "emfault3d.h"
#include "ioman.h"
#include "marchingcubes.h"
#include "polygon.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uitaskrunner.h"
#include "uitable.h"


#define mBelow 0
#define mAbove 1
#define mToMinInline 0
#define mToMaxInline 1
#define mToMinCrossline 2
#define mToMaxCrossline 3


static const char* collbls[] = { "Name", "Side", 0 };
#define cNameCol 0
#define cSideCol 1

class ImplicitBodyRegionExtractor : public ParallelTask
{
public:
ImplicitBodyRegionExtractor( const TypeSet<MultiID>& surflist, 
	const TypeSet<char>& sides, const CubeSampling& cs, Array3D<float>& res)
    : res_( res )
    , sides_( sides )
    , surflist_( surflist )
    , cs_( cs )
    {
	hors_.allowNull( true );
	fltsurplgs_.allowNull( true );
    }

~ImplicitBodyRegionExtractor()
{
    deepUnRef( hors_ );
    deepErase( fltsurplgs_ );
}

od_int64 nrIterations() const   	{ return cs_.hrg.totalNr(); }
const char* message() const		{ return "Extracting implicit body"; }

bool doPrepare( int )
{
    const BinID c0 = cs_.hrg.start + cs_.hrg.step;
    const BinID c1 = cs_.hrg.stop - cs_.hrg.step;

    for ( int idx=0; idx<surflist_.size(); idx++ )
    {
	RefMan<EM::EMObject> emobj = 
	    EM::EMM().loadIfNotFullyLoaded( surflist_[idx] );
	mDynamicCastGet( EM::Horizon3D*, newhor, emobj.ptr() );
	hors_ += newhor;
	fltsurplgs_ += 0;
	if ( newhor ) 
	{
	    newhor->ref();
	    continue;
	}
    
	mDynamicCastGet( EM::Fault3D*, newflt, emobj.ptr() );
	if ( !newflt ) continue;
	
	const Geometry::FaultStickSurface* flt = 
	    newflt->geometry().sectionGeometry(0);
	if ( !flt ) continue;

	fltsurplgs_.replace( idx, new ODPolygon<float>() );
	ODPolygon<float>& poly = *(fltsurplgs_[idx]);
	poly.setClosed( true );
	
	TypeSet<int> useinls;

	for ( int idy=0; idy<flt->nrSticks(); idy++ )
	{
	    const TypeSet<Coord3>* knots = flt->getStick(idy);
	    if ( !knots ) continue;

	    for ( int idz=0; idz<knots->size(); idz++ )
	    {
	    	const BinID bid = SI().transform( (*knots)[idz] );
		const Geom::Point2D<float> knot( bid.inl, bid.crl );
		const int lastidx = useinls.size()-1;

		if ( lastidx<0 || bid.inl>useinls[lastidx] )
		{
		    useinls += bid.inl;
		    poly.add( knot );
		}
		else if ( bid.inl<useinls[0] )
		{
		    useinls.insert( 0, bid.inl );
		    poly.insert( 0, knot );
		}
		else if ( bid.inl!=useinls[0] || bid.inl!=useinls[lastidx] )
		{
		    int i0=0, i1=lastidx, midx=(i0+i1+1)/2;
		    for ( ; ; )
		    {
			if ( bid.inl==useinls[midx] )
			    break;
			
			if ( bid.inl>useinls[midx] )
			    i0 = midx;
			else
			    i1 = midx;
			
			if ( i1-i0 < 2 )
			{
			    if ( bid.inl<=useinls[i0] )
				uiMSG().error("Something is wrong");
			    else
			    {
				useinls.insert( i1, bid.inl );
				poly.insert( i1, knot );
			    }
			    break;
			}
			
			midx=(i0+i1+1)/2;
		    }
		}
	    }
	}

	const int sz = poly.size();
	if ( sz<2 ) continue;
	
	const Geom::Point2D<float> first = poly.data()[0];
	const Geom::Point2D<float> last = poly.data()[sz-1];

	if ( sides_[idx]==mToMinInline )
	{
	    if ( last.y>first.y )
	    {
		poly.add( Geom::Point2D<float>(last.x,c1.crl) );
		poly.add( Geom::Point2D<float>(c0.inl,c1.crl) );
		poly.add( Geom::Point2D<float>(c0.inl,c0.crl) );
		poly.add( Geom::Point2D<float>(first.x,c0.crl) );
	    }
	    else
	    {
		poly.add( Geom::Point2D<float>(last.x,c0.crl) );
		poly.add( Geom::Point2D<float>(c0.inl,c0.crl) );
		poly.add( Geom::Point2D<float>(c0.inl,c1.crl) );
		poly.add( Geom::Point2D<float>(first.x,c1.crl) );
	    }
	}
	else if ( sides_[idx]==mToMaxInline )
	{
	    if ( last.y>first.y )
	    {
		poly.add( Geom::Point2D<float>(last.x,c1.crl) );
		poly.add( Geom::Point2D<float>(c1.inl,c1.crl) );
		poly.add( Geom::Point2D<float>(c1.inl,c0.crl) );
		poly.add( Geom::Point2D<float>(first.x,c0.crl) );
	    }
	    else
	    {
		poly.add( Geom::Point2D<float>(last.x,c0.crl) );
		poly.add( Geom::Point2D<float>(c1.inl,c0.crl) );
		poly.add( Geom::Point2D<float>(c1.inl,c1.crl) );
		poly.add( Geom::Point2D<float>(first.x,c1.crl) );
	    }
	}
	else if ( sides_[idx]==mToMinCrossline )
	{
	    if ( last.x>first.x )
	    {
		poly.add( Geom::Point2D<float>(c1.inl,last.y) );
		poly.add( Geom::Point2D<float>(c1.inl,c0.crl) );
		poly.add( Geom::Point2D<float>(c0.inl,c0.crl) );
		poly.add( Geom::Point2D<float>(c0.inl,first.y) );
	    }
	    else
	    {
		poly.add( Geom::Point2D<float>(c0.inl,last.y) );
		poly.add( Geom::Point2D<float>(c0.inl,c0.crl) );
		poly.add( Geom::Point2D<float>(c1.inl,c0.crl) );
		poly.add( Geom::Point2D<float>(c1.inl,first.y) );
	    }
	}
	else if ( sides_[idx]==mToMaxCrossline )
	{
	    if ( last.x>first.x )
	    {
		poly.add( Geom::Point2D<float>(c1.inl,last.y) );
		poly.add( Geom::Point2D<float>(c1.inl,c1.crl) );
		poly.add( Geom::Point2D<float>(c0.inl,c1.crl) );
		poly.add( Geom::Point2D<float>(c0.inl,first.y) );
	    }
	    else
	    {
		poly.add( Geom::Point2D<float>(c0.inl,last.y) );
		poly.add( Geom::Point2D<float>(c0.inl,c1.crl) );
		poly.add( Geom::Point2D<float>(c1.inl,c1.crl) );
		poly.add( Geom::Point2D<float>(c1.inl,first.y) );
	    }
	}
    }

    return true;
}


#define mContSetOutsideVal() \
    for ( int idz=0; idz<zsz; idz++ ) \
	res_.set( inlidx, crlidx, idz, 1 ); \
    continue


bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    const int surfsz = surflist_.size();
    const int inlsz = cs_.hrg.nrInl();
    const int crlsz = cs_.hrg.nrCrl();
    const int zsz = res_.info().getSize(2);

    for ( int idx=start; idx<=stop && shouldContinue(); idx++, addToNrDone(1) )
    {
	const int inlidx = idx / crlsz;
	const int crlidx = idx % crlsz;
	if ( !inlidx || inlidx==inlsz-1 || !crlidx || crlidx==crlsz-1 )
	{
	    mContSetOutsideVal();
	}

	const BinID bid = cs_.hrg.atIndex(inlidx,crlidx);
	const od_int64 bidsq = bid.toInt64();

	float minz, maxz;
	bool foundhorminz = false, foundhormaxz = false, inflt = true;
	for ( int idy=0; idy<surfsz; idy++ )
	{
	    if ( !inflt )
		break;

	    if ( !hors_[idy] ) 
	    {
		if ( fltsurplgs_[idy] )
    		    inflt = fltsurplgs_[idy]->isInside(
			    Geom::Point2D<float>(bid.inl,bid.crl),true,0.001);
		continue;
	    }
	    
	    float hz = hors_[idy]->getPos(hors_[idy]->sectionID(0),bidsq).z; 
	    if ( mIsUdf(hz) ) continue;

	    if ( sides_[idy]==mBelow )
	    {
		if ( !foundhorminz )
		{
		    minz = hz;
		    foundhorminz = true;
		}
		else if ( minz< hz )
		     minz = hz;
	    }
	    else
	    {
		if ( !foundhormaxz )
		{
    		    maxz = hz;
    		    foundhormaxz = true;
		}
		else if ( maxz>hz )
		    maxz = hz;
	    }
	}

	if ( !inflt || (!foundhorminz && !foundhormaxz) )
	{
	    mContSetOutsideVal();
	}

	if ( !foundhorminz )
	    minz = cs_.zrg.start;

	if ( !foundhormaxz )
	    maxz = cs_.zrg.stop;

	for ( int idz=1; idz<zsz-1; idz++ )
	{
	    const double curz = cs_.zrg.atIndex( idz );
	    const double val = curz<minz ? minz-curz : (curz>maxz ? curz-maxz :
		    -mMIN(curz-minz,maxz-curz) );
	    res_.set( inlidx, crlidx, idz, val );
	}
    	res_.set( inlidx, crlidx, 0, 1 );
	res_.set( inlidx, crlidx, zsz-1, 1 );
    }

    return true;
}

Array3D<float>&				res_;
const CubeSampling&			cs_;

const TypeSet<MultiID>&			surflist_;
const TypeSet<char>&			sides_;
ObjectSet<EM::Horizon>			hors_;
ObjectSet< ODPolygon<float> >		fltsurplgs_;//vertical flts
//ObjectSet<Geometry::FaultStickSurface>	faults_;
};


uiBodyRegionDlg::uiBodyRegionDlg( uiParent* p, EM::MarchingCubesSurface& emcs )
    : uiDialog( p, uiDialog::Setup("Body region constructor",
		"Surrounding boundaries", mNoHelpID) )
    , emcs_( emcs )		  
    , ctio_( EMBodyTranslatorGroup::ioContext() )		  
{
    //setHelpID( "dgb:104.0.4" );

    subvolfld_ =  new uiPosSubSel( this,  uiPosSubSel::Setup( !SI().has3D(),
		true).withstep(false).seltxt("Volume subselection") );

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

    ctio_.ctxt.forread = false;
    outputfld_ = new uiIOObjSel( this, ctio_, "Output region body" );
    outputfld_->attach( alignedBelow, table_ ); 
}


uiBodyRegionDlg::~uiBodyRegionDlg()
{}


void uiBodyRegionDlg::addSurfaceCB( CallBacker* cb )
{
    mDynamicCastGet( uiPushButton*, but, cb );
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
	surfacelist_.remove( currow );

    table_->removeRow( currow );
    removebutton_->setSensitive( surfacelist_.size() );
}


bool uiBodyRegionDlg::acceptOK( CallBacker* cb )
{
    if ( !surfacelist_.size() )
    {
	uiMSG().warning("Please select at least one boundary");
	return false;
    }

    if ( !outputfld_->commitInput() )
    {
	uiMSG().error( "Cannot create the output body" );
	return false;
    }

    return createImplicitBody();
}


bool uiBodyRegionDlg::createImplicitBody()
{
    TypeSet<char> sides;
    for ( int idx=0; idx<surfacelist_.size(); idx++ )
    {
	mDynamicCastGet(uiComboBox*, selbox, 
		table_->getCellObject(RowCol(idx,cSideCol)) );    
    	sides += selbox->currentItem();
    }
    
    CubeSampling cs = subvolfld_->envelope();
    cs.zrg.start -= cs.zrg.step;
    cs.zrg.stop += cs.zrg.step;
    cs.hrg.start.inl -= cs.hrg.step.inl;
    cs.hrg.stop.inl += cs.hrg.step.inl;
    cs.hrg.start.crl -= cs.hrg.step.crl;
    cs.hrg.stop.crl += cs.hrg.step.crl;

    mDeclareAndTryAlloc( Array3DImpl<float>*, arr,
	    Array3DImpl<float> (cs.nrInl(),cs.nrCrl(),cs.nrZ()) );
    if ( !arr )
	return false;
    
    ImplicitBodyRegionExtractor ext( surfacelist_, sides, cs, *arr );
    if ( !ext.execute() )
    {
	uiMSG().error("Extracting body region failed.");
	return false;
    }

    ::MarchingCubesSurface& mcs = emcs_.surface();
    mcs.removeAll();

    uiTaskRunner taskrunner( this );
    mcs.setVolumeData( 0, 0, 0, *arr, 0, &taskrunner);
    
    emcs_.setInlSampling(
	    SamplingData<int>(cs.hrg.start.inl,cs.hrg.step.inl) );
    emcs_.setCrlSampling(
	    SamplingData<int>(cs.hrg.start.crl,cs.hrg.step.crl) );
    emcs_.setZSampling( SamplingData<float>(cs.zrg.start,cs.zrg.step) );

    emcs_.setMultiID( ctio_.ioobj->key() );
    emcs_.setName( ctio_.ioobj->name() );
    emcs_.setFullyLoaded( true );
    emcs_.setChangedFlag();

    return true;
}





