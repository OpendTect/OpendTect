
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
 RCS:		$Id: emfaultstickpainter.cc,v 1.1 2010-01-21 10:06:41 cvsumesh Exp $
________________________________________________________________________

-*/

#include "emfaultstickpainter.h"

#include "faultstickset.h"
#include "emfaultstickset.h"
#include "emmanager.h"
#include "emobject.h"
#include "survinfo.h"
#include "trigonometry.h"

namespace EM
{

FaultStickPainter::FaultStickPainter( FlatView::Viewer& fv )
    : viewer_(fv)
    , markerlinestyle_(LineStyle::Solid,2,Color(0,255,0))
    , markerstyle_( MarkerStyle2D::Square, 4, Color::White() )
    , is2d_(false)
    , linenm_( 0 )
    , lineset_( 0 )
{
    faultmarkerline_.allowNull();
    cs_.setEmpty();
}


void FaultStickPainter::setCubeSampling( const CubeSampling& cs, bool update )
{
    cs_ = cs;
}


void FaultStickPainter::addFaultStickSet( const MultiID& mid )
{
    const EM::ObjectID oid = EM::EMM().getObjectID( mid );
    addFaultStickSet( oid );
}


void FaultStickPainter::addFaultStickSet( const EM::ObjectID& oid )
{
    for ( int idx=0; idx<fssinfos_.size(); idx++ )
	if ( fssinfos_[idx]->id_ == oid )
	    return;

    FaultStickSetInfo* fssinfo = new FaultStickSetInfo;
    fssinfo->id_ = oid;
    fssinfo->lineenabled_ = true;
    fssinfo->nodeenabled_ = true;
    fssinfo->name_ = EM::EMM().getObject( oid )->name();

    fssinfos_ += fssinfo;

    if ( !addPolyLine(oid) )
    {
	delete fssinfos_.remove( fssinfos_.size() - 1 );
	return;
    }

    viewer_.handleChange( FlatView::Viewer::Annot );
}


bool FaultStickPainter::addPolyLine( const EM::ObjectID& oid )
{
    RefMan<EM::EMObject> emobject = EM::EMM().getObject( oid );

    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss ) return false;

    ObjectSet<ObjectSet<FlatView::Annotation::AuxData> >* sectionmarkerlines =
		new ObjectSet<ObjectSet<FlatView::Annotation::AuxData> >;

    for ( int sidx=0; sidx<emfss->nrSections(); sidx++ )
    {
	int sid = emfss->sectionID( sidx );
	mDynamicCastGet( const Geometry::FaultStickSet*, fss,
			 emfss->sectionGeometry( sid ) );
	if ( fss->isEmpty() )
	    continue;

	RowCol rc;
	const StepInterval<int> rowrg = fss->rowRange();

	ObjectSet<FlatView::Annotation::AuxData>* markerlines = 
	    			new ObjectSet<FlatView::Annotation::AuxData>;
	(*sectionmarkerlines) += markerlines;

	for ( rc.row=rowrg.start; rc.row<=rowrg.stop; rc.row+=rowrg.step )
	{
	    if ( !fss->colRange( rc.row ).width() )
		    continue;

	    StepInterval<int> colrg = fss->colRange( rc.row ); 

	    FlatView::Annotation::AuxData* stickauxdata =
	 			new FlatView::Annotation::AuxData( 0 );
	    stickauxdata->poly_.erase();
	    stickauxdata->linestyle_ = markerlinestyle_;
	    stickauxdata->linestyle_.color_ = emfss->preferredColor();
	    stickauxdata->markerstyles_ += markerstyle_;

	    if ( emfss->geometry().pickedOn2DLine(sid,rc.row) )
	    {
		const MultiID* lset = emfss->geometry().lineSet( sid, rc.row );
		const char* lnm = emfss->geometry().lineName( sid, rc.row );

		if ( (lset != lineset_) || (!matchString(lnm,linenm_)) )
		    continue;
	    }
	    else if ( emfss->geometry().pickedOnPlane(sid,rc.row) )
	    {
		if ( cs_.isEmpty() ) continue;
	    }
	    else continue;

	    if ( cs_.isEmpty() ) // this means this is a 2D Line
	    {
		for ( rc.col=colrg.start;rc.col<=colrg.stop;rc.col+=colrg.step )
		{
		    const Coord3 pos = fss->getKnot( rc );
		    float dist;
		    if ( getNearestDistance(pos,dist) )
			stickauxdata->poly_ += FlatView::Point( dist , pos.z );
		}
	    }
	    else
	    {
		Coord3 editnormal( 0, 0, 1 ); 
		// Let's assume cs default dir. is 'Z'

		if ( cs_.defaultDir() == CubeSampling::Inl )
		    editnormal = Coord3( SI().binID2Coord().rowDir(), 0 );
		else if ( cs_.defaultDir() == CubeSampling::Crl )
		    editnormal = Coord3( SI().binID2Coord().colDir(), 0 );

		if ( editnormal.normalize() != 
			emfss->geometry().getEditPlaneNormal(sid,rc.row) )
		    continue;

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

		    for ( rc.col=colrg.start;rc.col<=colrg.stop;
			    			rc.col+=colrg.step )
		    {
			const Coord3& pos = fss->getKnot( rc );
			if (pointOnEdge2D(pos.coord(),extrcoord1,extrcoord2,.5))
			{
			    if ( cs_.defaultDir() == CubeSampling::Inl )
				stickauxdata->poly_ += FlatView::Point(
				       SI().transform(pos.coord()).crl, pos.z );
			    else if ( cs_.defaultDir() == CubeSampling::Crl )
				stickauxdata->poly_ += FlatView::Point(
				       SI().transform(pos.coord()).inl, pos.z );
			}
		    }
		}
		else
		{
		    for ( rc.col=colrg.start; rc.col<=colrg.stop; 
			    				rc.col+=colrg.step )
		    {
			const Coord3 pos = fss->getKnot( rc );
			if ( pos.z != cs_.zrg.start )
			    break;

			BinID binid = SI().transform(pos.coord());
			stickauxdata->poly_ +=
			    FlatView::Point( binid.inl, binid.crl );
		    }

		}
	    }

	    if ( stickauxdata->poly_.size() == 0 )
		delete stickauxdata;
	    else
	    {
		(*markerlines) += stickauxdata;
		viewer_.appearance().annot_.auxdata_ += stickauxdata;
	    }
	}
    }

    return true;
}


bool FaultStickPainter::getNearestDistance( const Coord3& pos, float& dist )
{
    int posidx = -1;
    mSetUdf(dist);

    for ( int idx=coords_.size()-1; idx>=0; idx-- )
    {
	const float caldist = pos.Coord::sqDistTo( coords_[idx] );
	if ( caldist < dist )
	{
	    dist = caldist;
	    posidx = idx;
	}
    }

    if ( posidx != -1 )
	dist = distances_[posidx];

    return posidx!=-1;
}

} //namespace EM
