/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
 RCS:		$Id: emfaultstickpainter.cc,v 1.16 2012-08-08 05:47:54 cvssalil Exp $
________________________________________________________________________

-*/

#include "emfaultstickpainter.h"

#include "faultstickset.h"
#include "emfaultstickset.h"
#include "emmanager.h"
#include "emobject.h"
#include "flatposdata.h"
#include "survinfo.h"
#include "trigonometry.h"

namespace EM
{

FaultStickPainter::FaultStickPainter( FlatView::Viewer& fv,
				      const EM::ObjectID& oid )
    : viewer_(fv)
    , emid_(oid)
    , markerlinestyle_( LineStyle::Solid,2,Color(0,255,0) )
    , markerstyle_( MarkerStyle2D::Square, 4, Color(255,255,0) )
    , activestickid_( -1 )
    , is2d_( false )
    , linenm_( 0 )
    , path_(0)
    , flatposdata_(0)
    , lsetid_( 0 )
    , abouttorepaint_( this )
    , repaintdone_( this )
    , linenabled_( true )
    , knotenabled_( true )
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
    {
	emobj->ref();
	emobj->change.notify( mCB(this,FaultStickPainter,fssChangedCB) );
    }
    cs_.setEmpty();
}


FaultStickPainter::~FaultStickPainter()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
    {
	emobj->change.remove( mCB(this,FaultStickPainter,fssChangedCB) );
	emobj->unRef();
    }

    removePolyLine();
    viewer_.handleChange( FlatView::Viewer::Annot );
}


void FaultStickPainter::setCubeSampling( const CubeSampling& cs, bool update )
{
    cs_ = cs;
}


void FaultStickPainter::setPath( const TypeSet<BinID>* path )
{
    path_ = path;
}


void FaultStickPainter::setFlatPosData( const FlatPosData* fps )
{
    if ( path_ )
	flatposdata_ = fps;
}


void FaultStickPainter::paint()
{
    removePolyLine();
    addPolyLine();
    viewer_.handleChange( FlatView::Viewer::Annot );
}


bool FaultStickPainter::addPolyLine()
{
    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid_ );

    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss ) return false;

    for ( int sidx=0; sidx<emfss->nrSections(); sidx++ )
    {
	int sid = emfss->sectionID( sidx );
	mDynamicCastGet( const Geometry::FaultStickSet*, fss,
			 emfss->sectionGeometry( sid ) );
	if ( fss->isEmpty() )
	    continue;

	RowCol rc;
	const StepInterval<int> rowrg = fss->rowRange();

	ObjectSet<StkMarkerInfo>* secmarkerlines = new ObjectSet<StkMarkerInfo>;
	sectionmarkerlines_ += secmarkerlines;

	for ( rc.row=rowrg.start; rc.row<=rowrg.stop; rc.row+=rowrg.step )
	{
	    StepInterval<int> colrg = fss->colRange( rc.row ); 

	    FlatView::AuxData* stickauxdata = viewer_.createAuxData( 0 );
	    stickauxdata->poly_.erase();
	    stickauxdata->linestyle_ = markerlinestyle_;
	    if ( rc.row == activestickid_ )
		stickauxdata->linestyle_.width_ *= 2;

	    stickauxdata->linestyle_.color_ = emfss->preferredColor();
	    stickauxdata->markerstyles_ += markerstyle_;
	    if ( !knotenabled_ )
		stickauxdata->markerstyles_.erase();
	    stickauxdata->enabled_ = linenabled_;

	    if ( emfss->geometry().pickedOn2DLine(sid,rc.row) )
	    {
		const MultiID* lset =
			    emfss->geometry().pickedMultiID( sid, rc.row );
		const char* lnm = emfss->geometry().pickedName( sid, rc.row );

		if ( !is2d_ || !matchString(lnm,linenm_) ||
		     !lset || *lset!=lsetid_ )
		    continue;
	    }
	    else if ( emfss->geometry().pickedOnPlane(sid,rc.row) )
	    {
		if ( cs_.isEmpty() && !path_ ) continue;
	    }
	    else continue;

	    if ( cs_.isEmpty() ) // this means this is a 2D or random Line
	    {
		if ( path_ )
		{
		    BinID bid;

		    for ( rc.col=colrg.start;rc.col<=colrg.stop;
			  rc.col+=colrg.step )
		    {
			const Coord3 pos = fss->getKnot( rc );
			bid = SI().transform( pos.coord() );
			int idx = path_->indexOf( bid );

			if ( idx < 0 ) continue;

			Coord3 editnormal( getNormalInRandLine(idx), 0 );
			const Coord3 nzednor = editnormal.normalize();
			const Coord3 stkednor =
			    emfss->geometry().getEditPlaneNormal(sid,rc.row);
			const bool equinormal =
			    mIsEqual(nzednor.x,stkednor.x,.001) &&
			    mIsEqual(nzednor.y,stkednor.y,.001) &&
			    mIsEqual(nzednor.z,stkednor.z,.00001);

			if ( !equinormal ) continue;

			stickauxdata->poly_ +=
			    FlatView::Point( flatposdata_->position(true,idx),
				    	     pos.z );
		    }
		}
		else
		{
		    for ( rc.col=colrg.start;rc.col<=colrg.stop;
			  rc.col+=colrg.step ) 
		    {
			const Coord3 pos = fss->getKnot( rc );
			float dist;
			if ( getNearestDistance(pos,dist) )
			    stickauxdata->poly_ +=
						FlatView::Point( dist , pos.z );
		    }
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

		const Coord3 nzednor = editnormal.normalize();
		const Coord3 stkednor = 
		    	emfss->geometry().getEditPlaneNormal(sid,rc.row);

		const bool equinormal =
		    mIsEqual(nzednor.x,stkednor.x,.001) &&
		    mIsEqual(nzednor.y,stkednor.y,.001) &&
		    mIsEqual(nzednor.z,stkednor.z,.00001);

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

		    for ( rc.col=colrg.start;rc.col<=colrg.stop;
			    			rc.col+=colrg.step )
		    {
			const Coord3& pos = fss->getKnot( rc );
			BinID knotbinid = SI().transform( pos );
			if (pointOnEdge2D(pos.coord(),extrcoord1,extrcoord2,.5)
			    || (cs_.defaultDir()==CubeSampling::Inl
				&& knotbinid.inl==extrbid1.inl)
			    || (cs_.defaultDir()==CubeSampling::Crl
				&& knotbinid.crl==extrbid1.crl) )
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
			if ( !mIsEqual(pos.z,cs_.zrg.start,.0001) )
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
		StkMarkerInfo* stkmkrinfo = new StkMarkerInfo;
		stkmkrinfo->marker_ = stickauxdata;
		stkmkrinfo->stickid_ = rc.row;
		(*secmarkerlines) += stkmkrinfo;
		viewer_.addAuxData( stickauxdata );
	    }
	}
    }

    return true;
}


void FaultStickPainter::enableLine( bool yn )
{
    if ( linenabled_ == yn )
	return;

    for ( int markidx=sectionmarkerlines_.size()-1; markidx>=0; markidx-- )
    {
	ObjectSet<StkMarkerInfo>* markerlines = sectionmarkerlines_[markidx];
	for ( int idy=markerlines->size()-1; idy>=0; idy-- )
	{
	    (*markerlines)[idy]->marker_->enabled_ = yn;
	}
    }

    linenabled_ = yn;
    viewer_.handleChange( FlatView::Viewer::Annot );
}


void FaultStickPainter::enableKnots( bool yn )
{
    if ( knotenabled_ == yn )
	return;

    for ( int markidx=sectionmarkerlines_.size()-1; markidx>=0; markidx-- )
    {
	ObjectSet<StkMarkerInfo>* markerlines = sectionmarkerlines_[markidx];
	for ( int idy=markerlines->size()-1; idy>=0; idy-- )
	{
	    if ( !yn )
		(*markerlines)[idy]->marker_->markerstyles_.erase();
	    else
		(*markerlines)[idy]->marker_->markerstyles_ += markerstyle_;
	}
    }

    knotenabled_ = yn;
    viewer_.handleChange( FlatView::Viewer::Annot );
}


void FaultStickPainter::repaintFSS()
{
    removePolyLine();
    addPolyLine();
    viewer_.handleChange( FlatView::Viewer::Annot );
}


void FaultStickPainter::removePolyLine()
{
    for ( int markidx=sectionmarkerlines_.size()-1; markidx>=0; markidx-- )
    {
	ObjectSet<StkMarkerInfo>* markerlines = sectionmarkerlines_[markidx];
	if ( !markerlines->size() ) continue;

	for ( int idy=markerlines->size()-1; idy>=0; idy-- )
	{
	    viewer_.removeAuxData( (*markerlines)[idy]->marker_ );
	}
    }

    deepErase( sectionmarkerlines_ );
}


void FaultStickPainter::fssChangedCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( const EM::EMObjectCallbackData&,
	    			cbdata, caller, cb );
    mDynamicCastGet(EM::EMObject*,emobject,caller);
    if ( !emobject ) return;

    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject);
    if ( !emfss ) return;

    if ( emobject->id() != emid_ ) return;

    switch ( cbdata.event )
    {
	case EM::EMObjectCallbackData::Undef:
	    break;
	case EM::EMObjectCallbackData::PrefColorChange:
	    {
		for ( int oidx=0; oidx<sectionmarkerlines_.size(); oidx++ )
		{
		    if ( !sectionmarkerlines_[oidx] ) continue;
		    ObjectSet<StkMarkerInfo>& stmkrinfos = 
						*sectionmarkerlines_[oidx];
		    
		    for( int iidx=0; iidx<stmkrinfos.size(); iidx++ )
		    {
			if ( !stmkrinfos[iidx] ) continue;

			stmkrinfos[iidx]->marker_->linestyle_.color_ =
			    				emfss->preferredColor();
			viewer_.updateProperties( *stmkrinfos[iidx]->marker_ );
		    }
		}
		break;
	    }
	case EM::EMObjectCallbackData::PositionChange:
	{
	    if ( emfss->hasBurstAlert() )
		return;
	    abouttorepaint_.trigger();
	    repaintFSS();
	    repaintdone_.trigger();
	    break;
	}
	case EM::EMObjectCallbackData::BurstAlert:
	{
	    if (  emobject->hasBurstAlert() )
		return;
	    abouttorepaint_.trigger();
	    repaintFSS();
	    repaintdone_.trigger();
	    break;
	}
	default:
	    break;
    }
}


FlatView::AuxData* FaultStickPainter::getAuxData(
							 const EM::PosID* pid )
{
    if ( pid->objectID() != emid_ )
	return 0;

    return (*sectionmarkerlines_[0])[activestickid_]->marker_;
}


void FaultStickPainter::getDisplayedSticks(
					ObjectSet<StkMarkerInfo>& dispstkinfo )
{
    if ( !sectionmarkerlines_.size() ) return;

    dispstkinfo = *sectionmarkerlines_[0];
}


bool FaultStickPainter::hasDiffActiveStick( const EM::PosID* pid )
{
    if ( pid->objectID() != emid_ ||
	 pid->getRowCol().row != activestickid_ )
	return true;
    else
	return false;
}


void FaultStickPainter::setActiveStick( EM::PosID& pid )
{
    if ( pid.objectID() != emid_ ) return;

    if ( pid.getRowCol().row == activestickid_ ) return;

    for ( int stkidx=0; stkidx<sectionmarkerlines_[0]->size(); stkidx++ )
    {
	LineStyle& linestyle = 
	    (*sectionmarkerlines_[0])[stkidx]->marker_->linestyle_;
	if ( (*sectionmarkerlines_[0])[stkidx]->stickid_==activestickid_ )
	    linestyle.width_ = markerlinestyle_.width_;
	else if ( (*sectionmarkerlines_[0])[stkidx]->stickid_==
		  pid.getRowCol().row )
	    linestyle.width_ = markerlinestyle_.width_ * 2;
    }

    activestickid_ = pid.getRowCol().row;
    viewer_.handleChange( FlatView::Viewer::Annot );
}


bool FaultStickPainter::getNearestDistance( const Coord3& pos, float& dist )
{
    int posidx = -1;
    mSetUdf(dist);

    for ( int idx=coords_.size()-1; idx>=0; idx-- )
    {
	const float caldist = (float) pos.Coord::sqDistTo( coords_[idx] );
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


Coord FaultStickPainter::getNormalToTrace( int trcnr ) const
{
    int posid = -1;
    int sz = trcnos_.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( trcnos_[idx] == trcnr )
	{
	    posid = idx;
	    break;
	}
    }

    if ( posid == -1 || sz == 0 )
	return Coord(mUdf(float), mUdf(float));

    Coord pos = coords_[posid];
    Coord v1;
    if ( posid+1<sz )
	v1 = coords_[posid+1]- pos;
    else if ( posid-1>=0 )
	v1 = pos - coords_[posid-1];

    if ( v1.x == 0 )
	return Coord( 1, 0 );
    else if ( v1.y == 0 )
	return Coord( 0, 1 );
    else
    {
	double length = Math::Sqrt( v1.x*v1.x + v1.y*v1.y );
	return Coord( -v1.y/length, v1.x/length );
    }
}


Coord FaultStickPainter::getNormalInRandLine( int idx ) const
{
    if ( !path_ )
	return Coord(mUdf(float), mUdf(float));

    if ( idx < 0 || path_->size() == 0 )
	return Coord(mUdf(float), mUdf(float));

    BinID pivotbid = (*path_)[idx];
    BinID nextbid;

    if ( idx+1 < path_->size() )
	nextbid = (*path_)[idx+1];
    else if ( idx-1 > 0 )
	nextbid = (*path_)[idx-1];

    if ( pivotbid.inl == nextbid.inl )
	return  SI().binID2Coord().rowDir();
    else if ( pivotbid.crl == nextbid.crl )
	return SI().binID2Coord().colDir();

    return Coord(mUdf(float), mUdf(float));
}

} //namespace EM
