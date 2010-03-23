
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Feb 2010
 RCS:		$Id: emfault3dpainter.cc,v 1.3 2010-03-23 06:14:14 cvskarthika Exp $
________________________________________________________________________

-*/

#include "emfault3dpainter.h"

#include "emfault3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "explfaultsticksurface.h"
#include "explplaneintersection.h"
#include "positionlist.h"
#include "survinfo.h"
#include "trigonometry.h"

namespace EM
{

Fault3DPainter::Fault3DPainter( FlatView::Viewer& fv )
    : viewer_(fv)
    , markerlinestyle_(LineStyle::Solid,2,Color(0,255,0))
    , markerstyle_(MarkerStyle2D::Square, 4, Color(255,255,0) )
    , activef3did_( -1 )
    , activestickid_( mUdf(int) )
    , abouttorepaint_(this)
    , repaintdone_(this)
{
    f3dmarkers_.allowNull();
    cs_.setEmpty();
}


Fault3DPainter::~Fault3DPainter()
{
    while ( f3dmarkers_.size() )
	removeFault3D(0);

    deepErase( f3dinfos_ );
}


void Fault3DPainter::setCubeSampling( const CubeSampling& cs, bool update )
{ cs_ = cs; }


void Fault3DPainter::addFault3D( const MultiID& mid )
{
    const EM::ObjectID oid = EM::EMM().getObjectID( mid );
    addFault3D( oid );
}


void Fault3DPainter::addFault3D( const EM::ObjectID& oid )
{
    for ( int idx=0; idx<f3dinfos_.size(); idx++ )
	if ( f3dinfos_[idx]->id_ == oid )
	    return;

    Fault3DInfo* f3dinfo = new Fault3DInfo;
    f3dinfo->id_ = oid;
    f3dinfo->lineenabled_ = true;
    f3dinfo->nodeenabled_ = true;
    f3dinfo->name_ = EM::EMM().getObject( oid )->name();

    f3dinfos_ += f3dinfo;

    if ( !addPolyLine(oid) )
    {
	delete f3dinfos_.remove( f3dinfos_.size() - 1 );
	return;
    }

    viewer_.handleChange( FlatView::Viewer::Annot );
}


bool Fault3DPainter::addPolyLine( const EM::ObjectID& oid )
{
    RefMan<EM::EMObject> emobject = EM::EMM().getObject( oid );

    mDynamicCastGet(EM::Fault3D*,emf3d,emobject.ptr());
    if ( !emf3d ) return false;

    emf3d->change.notify( mCB(this,Fault3DPainter,fault3DChangedCB) );

    ObjectSet<Fault3DMarker>* f3dmarker = new ObjectSet<Fault3DMarker>;
    f3dmarkers_ += f3dmarker;

    for ( int sidx=0; sidx<emf3d->nrSections(); sidx++ )
    {
	int sid = emf3d->sectionID( sidx );

	Fault3DMarker* f3dsectionmarker = new Fault3DMarker;
	(*f3dmarker) += f3dsectionmarker;

	bool stickpained = paintSticks(emf3d,sid,f3dsectionmarker);
	bool intersecpainted = paintIntersection(emf3d,sid,f3dsectionmarker);
	if ( !stickpained && !intersecpainted )
	     return false;
    }

    return true;
}


bool Fault3DPainter::paintSticks(EM::Fault3D* f3d, const EM::SectionID& sid,
				 Fault3DMarker* f3dmaker )
{
    mDynamicCastGet( Geometry::FaultStickSurface*, fss,
		     f3d->sectionGeometry(sid) );

    if ( !fss || fss->isEmpty() )
	return false;

    if ( cs_.isEmpty() ) return false;

    RowCol rc;
    const StepInterval<int> rowrg = fss->rowRange();
    for ( rc.row=rowrg.start; rc.row<=rowrg.stop; rc.row+=rowrg.step )
    {
	StepInterval<int> colrg = fss->colRange( rc.row );
	FlatView::Annotation::AuxData* stickauxdata =
	 			new FlatView::Annotation::AuxData( 0 );
	stickauxdata->poly_.erase();
	stickauxdata->linestyle_ = markerlinestyle_;
	stickauxdata->linestyle_.color_ = f3d->preferredColor();
	if ( activef3did_ == f3d->id() )
	stickauxdata->markerstyles_ += markerstyle_;

	Coord3 editnormal( 0, 0, 1 ); 
	// Let's assume cs default dir. is 'Z'

	if ( cs_.defaultDir() == CubeSampling::Inl )
	    editnormal = Coord3( SI().binID2Coord().rowDir(), 0 );
	else if ( cs_.defaultDir() == CubeSampling::Crl )
	    editnormal = Coord3( SI().binID2Coord().colDir(), 0 );

	const Coord3 nzednor = editnormal.normalize();
	const Coord3 stkednor = f3d->geometry().getEditPlaneNormal(sid,rc.row);

	const bool equinormal =
	    mIsEqual(nzednor.x,stkednor.x,.0000099) &&
	    mIsEqual(nzednor.y,stkednor.y,.0000099) &&
	    mIsEqual(nzednor.z,stkednor.z,.0000099);

	if ( !equinormal ) continue;	

	// we need to deal in different way if cs direction is Z
	if ( cs_.defaultDir() != CubeSampling::Z )
	{
	    BinID extrbid1, extrbid2;
	    if ( cs_.defaultDir() == CubeSampling::Inl )
	    {
		extrbid1.inl = extrbid2.inl = cs_.hrg.inlRange().start;
		extrbid1.crl = cs_.hrg.crlRange().start;
		extrbid2.crl = cs_.hrg.crlRange().stop;
	    }
	    else if ( cs_.defaultDir() == CubeSampling::Crl )
	    {
		extrbid1.inl = cs_.hrg.inlRange().start;
		extrbid2.inl = cs_.hrg.inlRange().stop;
		extrbid1.crl = extrbid2.crl = cs_.hrg.crlRange().start;
	    }
	    
	    Coord extrcoord1, extrcoord2;
	    extrcoord1 = SI().transform( extrbid1 );
	    extrcoord2 = SI().transform( extrbid2 );

	    for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	    {
		const Coord3& pos = fss->getKnot( rc );
		if ( pointOnEdge2D(pos.coord(),extrcoord1,extrcoord2,5) )
		{
		    if ( cs_.defaultDir() == CubeSampling::Inl )
			stickauxdata->poly_ += FlatView::Point(
					SI().transform(pos.coord()).crl, pos.z);
		    else if ( cs_.defaultDir() == CubeSampling::Crl )
			stickauxdata->poly_ += FlatView::Point(
					SI().transform(pos.coord()).inl, pos.z);
		}
	    }
	}
	else
	{
	    for ( rc.col=colrg.start; rc.col<=colrg.stop; 
				      rc.col+=colrg.step )
	    {
		const Coord3 pos = fss->getKnot( rc );
		if ( !mIsEqual(pos.z,cs_.zrg.start,.0001) )
		    break;

		BinID binid = SI().transform(pos.coord());
		stickauxdata->poly_ +=
			FlatView::Point( binid.inl, binid.crl );
	    }
	}

	if ( stickauxdata->poly_.size() == 0 )
		delete stickauxdata;
	    else
	    {
		StkMarkerInfo* stkmkrinfo = new StkMarkerInfo;
		stkmkrinfo->marker_ = stickauxdata;
		stkmkrinfo->stickid_ = rc.row;
		f3dmaker->stickmarker_ += stkmkrinfo;
		viewer_.appearance().annot_.auxdata_ += stickauxdata;
	    }
    }

    return true;
}


bool Fault3DPainter::paintIntersection( EM::Fault3D* f3d,
					const EM::SectionID& sid,
					Fault3DMarker* f3dmaker )
{
    Geometry::IndexedShape* faultsurf = new Geometry::ExplFaultStickSurface(
	    f3d->geometry().sectionGeometry(sid), SI().zFactor() );
    faultsurf->setCoordList( new Coord3ListImpl, new Coord3ListImpl );
    if ( !faultsurf->update(true,0) )
	return false;
    
    BinID start( cs_.hrg.start.inl, cs_.hrg.start.crl );
    BinID stop(cs_.hrg.stop.inl, cs_.hrg.stop.crl );

    Coord3 p0( SI().transform(start), cs_.zrg.start );
    Coord3 p1( SI().transform(start), cs_.zrg.stop );
    Coord3 p2( SI().transform(stop), cs_.zrg.stop );
    Coord3 p3( SI().transform(stop), cs_.zrg.start );

    TypeSet<Coord3> pts;

    pts += p0; pts += p1; pts += p2; pts += p3;

    const Coord3 normal = (p1-p0).cross(p3-p0).normalize();

    Geometry::ExplPlaneIntersection* intersectn = 
					new Geometry::ExplPlaneIntersection;
    intersectn->setShape( *faultsurf );
    intersectn->addPlane( normal, pts );

    Geometry::IndexedShape* idxshape = intersectn;
    idxshape->setCoordList(new Coord3ListImpl, new Coord3ListImpl );
    if ( !idxshape->update(true,0) )
	return false;

    if ( !idxshape->getGeometry().size() )
	return false;

    const Geometry::IndexedGeometry* idxgeom = idxshape->getGeometry()[0];
    TypeSet<int> coordindices = idxgeom->coordindices_;

    if ( !coordindices.size() )
	return false;

    Coord3List* clist = idxshape->coordList();
    mDynamicCastGet(Coord3ListImpl*,intersecposlist,clist);
    TypeSet<Coord3> intersecposs;
    if ( intersecposlist->getSize() )
    {
	int nextposid = intersecposlist->nextID( -1 );
	while ( nextposid!=-1 )
	{
	    if ( intersecposlist->isDefined(nextposid) )
		intersecposs += intersecposlist->get( nextposid );
	    nextposid = intersecposlist->nextID( nextposid );
	}
    }

    FlatView::Annotation::AuxData* intsecauxdat = 
	    				new FlatView::Annotation::AuxData( 0 );
    intsecauxdat->poly_.erase();
    intsecauxdat->linestyle_ = markerlinestyle_;
    intsecauxdat->linestyle_.width_ = markerlinestyle_.width_/2;
    intsecauxdat->linestyle_.color_ = f3d->preferredColor();

    for ( int idx=0; idx<coordindices.size(); idx++ )
    {
	if ( coordindices[idx] == -1 )
	{
	    viewer_.appearance().annot_.auxdata_ += intsecauxdat;
	    f3dmaker->intsecmarker_ += intsecauxdat;
	    intsecauxdat = new FlatView::Annotation::AuxData( 0 );
	    intsecauxdat->poly_.erase();
	    intsecauxdat->linestyle_ = markerlinestyle_;
	    intsecauxdat->linestyle_.width_ = markerlinestyle_.width_/2;
	    intsecauxdat->linestyle_.color_ = f3d->preferredColor();
	    continue;
	}

	const Coord3 pos = intersecposs[coordindices[idx]];
	BinID posbid =  SI().transform( pos.coord() );
	if ( cs_.nrZ() == 1 )
	    intsecauxdat->poly_ += FlatView::Point( posbid.inl,
						    posbid.crl );
	else if ( cs_.nrCrl() == 1 )
	    intsecauxdat->poly_ += FlatView::Point( posbid.inl, pos.z );
	else if ( cs_.nrInl() == 1 )
	    intsecauxdat->poly_ += FlatView::Point( posbid.crl, pos.z );
    }

    viewer_.appearance().annot_.auxdata_ += intsecauxdat;
    f3dmaker->intsecmarker_ += intsecauxdat;
    
    delete faultsurf;
    delete intersectn;

    return true;
}


void Fault3DPainter::setActiveF3D( const EM::ObjectID& oid )
{
    if ( oid == activef3did_ ) return;

    int idx = -1;

    for ( int f3didx=0; f3didx<f3dinfos_.size(); f3didx++ )
    {
	if ( oid == -1 )
	{
	    if ( f3dinfos_[f3didx]->id_==activef3did_ )
	    { idx = f3didx; break; }
	}
	else if ( f3dinfos_[f3didx]->id_ == oid )
	{ idx = f3didx; break; }
    }

    activef3did_ = oid;

    if ( idx == -1 ) return;
    for ( int auxdid=0; auxdid < (*f3dmarkers_[idx])[0]->stickmarker_.size();
	    		auxdid++ )
    {
	TypeSet<MarkerStyle2D>& markerstyle =
	   (*f3dmarkers_[idx])[0]->stickmarker_[auxdid]->marker_->markerstyles_;
	if ( oid == -1 )
	    markerstyle.erase();
	else
	    markerstyle += markerstyle_;
    }

    viewer_.handleChange( FlatView::Viewer::Annot );
}


void Fault3DPainter::setActiveStick( EM::PosID& pid )
{
    if ( pid.objectID() != activef3did_ ) return;

    if ( pid.getRowCol().row == activestickid_ ) return;

    int idx = -1;

    for ( int f3didx=0; f3didx<f3dinfos_.size(); f3didx++ )
    {
	if ( f3dinfos_[f3didx]->id_ == pid.objectID() )
	{ idx = f3didx; break; }
    }

    if ( idx == -1 ) return;

    for ( int auxdid=0; auxdid < (*f3dmarkers_[idx])[0]->stickmarker_.size();
	    		auxdid++ )
    {
	LineStyle& linestyle = 
	    (*f3dmarkers_[idx])[0]->stickmarker_[auxdid]->marker_->linestyle_;
	if ( (*f3dmarkers_[idx])[0]->stickmarker_[auxdid]->stickid_==
	     activestickid_ )
	    linestyle.width_ = markerlinestyle_.width_;
	else if ( (*f3dmarkers_[idx])[0]->stickmarker_[auxdid]->stickid_ ==
		  pid.getRowCol().row )
	    linestyle.width_ = markerlinestyle_.width_ * 2;
    }

    activestickid_ = pid.getRowCol().row;
    viewer_.handleChange( FlatView::Viewer::Annot );
}


bool Fault3DPainter::hasDiffActiveStick( const EM::PosID* pid )
{
    if ( pid->objectID() != activef3did_ ||
	 pid->getRowCol().row != activestickid_ )
	return true;
    else
	return false;
}


FlatView::Annotation::AuxData* Fault3DPainter::getAuxData( const EM::PosID* pid)
{
    int idx = -1;

    for ( int f3didx=0; f3didx<f3dinfos_.size(); f3didx++ )
    {
	if ( f3dinfos_[f3didx]->id_ == pid->objectID() )
	{ idx = f3didx; break; }
    }

    if ( idx == -1 ) return 0;

    return (*f3dmarkers_[idx])[0]->stickmarker_[activestickid_]->marker_;
}


void Fault3DPainter::getDisplayedSticks( const EM::ObjectID& oid,
					ObjectSet<StkMarkerInfo>& dispstkinfo )
{
    int idx = -1;

    for ( int f3didx=0; f3didx<f3dinfos_.size(); f3didx++ )
    {
	if ( f3dinfos_[f3didx]->id_ == oid )
	{ idx = f3didx; break; }
    }

    if ( idx == -1 ) return;

    if ( !f3dmarkers_.size() || !f3dmarkers_[idx]->size() )
	return;

    dispstkinfo = (*f3dmarkers_[idx])[0]->stickmarker_;
}


void Fault3DPainter::removeFault3D( int idx )
{
    removePolyLine( idx );
    if ( EM::EMM().getObject(f3dinfos_[idx]->id_) )
    {
	mDynamicCastGet( EM::Fault3D*, f3d, 
	    EM::EMM().getObject(f3dinfos_[idx]->id_) );
	f3d->change.remove( mCB(this,Fault3DPainter,fault3DChangedCB) );
    }

    delete f3dinfos_[idx];
    f3dinfos_.remove( idx );
    viewer_.handleChange( FlatView::Viewer::Annot );
}


void Fault3DPainter::removePolyLine( int idx )
{
    ObjectSet<Fault3DMarker>* sectionmarkerlines = f3dmarkers_[idx];
    for ( int markidx=sectionmarkerlines->size()-1; markidx>=0; markidx-- )
    {
	Fault3DMarker* f3dmarker = (*sectionmarkerlines)[markidx];
	for ( int idi=f3dmarker->intsecmarker_.size()-1; idi>=0; idi-- )
	    viewer_.appearance().annot_.auxdata_ -= 
					f3dmarker->intsecmarker_[idi];
	for ( int ids=f3dmarker->stickmarker_.size()-1; ids>=0; ids-- )
	    viewer_.appearance().annot_.auxdata_ -= 
				f3dmarker->stickmarker_[ids]->marker_;
    }

    deepErase( *f3dmarkers_[idx] );
    delete f3dmarkers_[idx];
    f3dmarkers_.remove( idx );
}


void Fault3DPainter::repaintFault3D( const EM::ObjectID& oid)
{
    int f3didx = -1;

    for ( int idx = 0; idx<f3dinfos_.size(); idx++ )
	if ( f3dinfos_[idx]->id_ == oid )
	    { f3didx = idx; break; }

    if ( f3didx>=0 )
	removeFault3D( f3didx );

    addFault3D(oid);

    activestickid_ = mUdf(int);
}


void Fault3DPainter::fault3DChangedCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( const EM::EMObjectCallbackData&,
	    			cbdata, caller, cb );
    mDynamicCastGet(EM::EMObject*,emobject,caller);
    if ( !emobject ) return;

    mDynamicCastGet(EM::Fault3D*,emf3d,emobject);
    if ( !emf3d ) return;

    int f3dinfoidx = -1;

    for ( int idx=0; idx<f3dinfos_.size(); idx++ )
	if ( f3dinfos_[idx]->id_ == emobject->id() )
	{ f3dinfoidx = idx; break; }

    switch ( cbdata.event )
    {
	case EM::EMObjectCallbackData::Undef:
	    break;
	case EM::EMObjectCallbackData::PrefColorChange:
	    break;
	case EM::EMObjectCallbackData::PositionChange:
	{
	    abouttorepaint_.trigger();
	    repaintFault3D( emobject->id() );
	    repaintdone_.trigger();
	    break;
	}
	case EM::EMObjectCallbackData::BurstAlert:
	{
	    if (  emobject->hasBurstAlert() )
		return;
	    abouttorepaint_.trigger();
	    repaintFault3D( emobject->id() );
	    repaintdone_.trigger();
	    viewer_.handleChange( FlatView::Viewer::Annot );
	    break;
	}
	default:
	    break;
    }
}


} //namespace EM

