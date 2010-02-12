
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Feb 2010
 RCS:		$Id: emfault3dpainter.cc,v 1.1 2010-02-12 08:42:13 cvsumesh Exp $
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

namespace EM
{

Fault3DPainter::Fault3DPainter( FlatView::Viewer& fv )
    : viewer_(fv)
    , markerlinestyle_(LineStyle::Solid,2,Color(0,255,0))
    , markerstyle_(MarkerStyle2D::Square, 4, Color::White() )
{
    f3dmarkers_.allowNull();
    cs_.setEmpty();
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

    ObjectSet<Fault3DMarker>* f3dmarker = new ObjectSet<Fault3DMarker>;
    f3dmarkers_ += f3dmarker;

    for ( int sidx=0; sidx<emf3d->nrSections(); sidx++ )
    {
	int sid = emf3d->sectionID( sidx );
	mDynamicCastGet( const Geometry::FaultStickSet*, fss,
			 emf3d->sectionGeometry( sid ) );
	if ( fss->isEmpty() )
	    continue;
	
	Fault3DMarker* f3dsectionmarker = new Fault3DMarker;
	(*f3dmarker) += f3dsectionmarker;

	f3dsectionmarker->stickmarker_ = 
	    			new ObjectSet<FlatView::Annotation::AuxData>;

	//TODO paintSticks( emf3d,f3dsectionmarker->stickmarker_ );
	paintIntersection( emf3d,sid,f3dsectionmarker->intsecmarker_ );
    }

    return true;
}


void Fault3DPainter::paintIntersection( EM::Fault3D* f3d,
					const EM::SectionID& sid,
					FlatView::Annotation::AuxData* auxd )
{
    Geometry::IndexedShape* faultsurf = new Geometry::ExplFaultStickSurface(
	    f3d->geometry().sectionGeometry(sid), SI().zFactor() );
    faultsurf->setCoordList( new Coord3ListImpl, new Coord3ListImpl );
    if ( !faultsurf->update(true,0) )
	return;
    
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
	return;

    Coord3List* clist = idxshape->coordList();
    mDynamicCastGet(Coord3ListImpl*,interseclist,clist);
    if ( interseclist->getSize() )
    {
	FlatView::Annotation::AuxData* intsecauxdat = 
	    				new FlatView::Annotation::AuxData( 0 );
	intsecauxdat->poly_.erase();
	intsecauxdat->linestyle_ = markerlinestyle_;
	intsecauxdat->linestyle_.color_ = f3d->preferredColor();
	int nextpoint = interseclist->nextID( -1 );
	while ( nextpoint!=-1 )
	{
	    if ( nextpoint == 0 )
		nextpoint = interseclist->nextID( nextpoint );
	    if ( interseclist->isDefined(nextpoint) )
	    {
		const Coord3 pos = interseclist->get( nextpoint );
		BinID posbid =  SI().transform( pos.coord() );
		if ( cs_.nrZ() == 1 )
		    intsecauxdat->poly_ += FlatView::Point( posbid.inl, 
			    				    posbid.crl );
		else if ( cs_.nrCrl() == 1 )
		    intsecauxdat->poly_ += FlatView::Point( posbid.inl, pos.z );
		else if ( cs_.nrInl() == 1 )
		    intsecauxdat->poly_ += FlatView::Point( posbid.crl, pos.z );
	    }

	    nextpoint = interseclist->nextID( nextpoint );
	}

	viewer_.appearance().annot_.auxdata_ += intsecauxdat;
	auxd = intsecauxdat;
    }

    delete faultsurf;
    delete intersectn;
}

} //namespace EM
