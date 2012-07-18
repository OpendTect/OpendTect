/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : Aug. 2008
___________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: polygonsurfeditor.cc,v 1.18 2012-07-18 09:13:20 cvsjaap Exp $";

#include "polygonsurfeditor.h"

#include "empolygonbody.h"
#include "emmanager.h"
#include "polygonsurfaceedit.h"
#include "mpeengine.h"
#include "selector.h"
#include "survinfo.h"
#include "trigonometry.h"

#include "undo.h"

namespace MPE
{

PolygonBodyEditor::PolygonBodyEditor( EM::PolygonBody& polygonsurf )
    : ObjectEditor(polygonsurf)
    , sowingpivot_(Coord3::udf())
{}


ObjectEditor* PolygonBodyEditor::create( EM::EMObject& emobj )
{
    mDynamicCastGet(EM::PolygonBody*,polygonsurf,&emobj);
    return polygonsurf ? new PolygonBodyEditor( *polygonsurf ) : 0;
}


void PolygonBodyEditor::initClass()
{ 
    MPE::EditorFactory().addCreator( create, EM::PolygonBody::typeStr() ); 
}


Geometry::ElementEditor* PolygonBodyEditor::createEditor(
						const EM::SectionID& sid )
{
    const Geometry::Element* ge = emObject().sectionGeometry( sid );
    if ( !ge ) return 0;

    mDynamicCastGet(const Geometry::PolygonSurface*,surface,ge);
    return !surface ? 0 : new Geometry::PolygonSurfEditor( 
	    		  *const_cast<Geometry::PolygonSurface*>(surface) );
}


static EM::PosID lastclicked_ = EM::PosID::udf();

void PolygonBodyEditor::setLastClicked( const EM::PosID& pid )
{
    lastclicked_ = pid;

    if ( sowingpivot_.isDefined() )
    {
	const Coord3 pos = emObject().getPos( pid );
	if ( pos.isDefined() )
	    sowinghistory_.insert( 0, pos );
    }
}


void PolygonBodyEditor::setSowingPivot( const Coord3 pos )
{
    if ( sowingpivot_.isDefined() && !pos.isDefined() )
	sowinghistory_.erase();

    sowingpivot_ = pos;
}


#define mCompareCoord( crd ) Coord3( crd, crd.z*zfactor )

void PolygonBodyEditor::getInteractionInfo( EM::PosID& nearestpid0,
					    EM::PosID& nearestpid1, 
					    EM::PosID& insertpid,
				    	    const Coord3& mousepos, 
					    float zfactor ) const
{
    nearestpid0 = EM::PosID::udf();
    nearestpid1 = EM::PosID::udf();
    insertpid = EM::PosID::udf();

    const Coord3& pos = sowingpivot_.isDefined() && sowinghistory_.isEmpty()
			? sowingpivot_ : mousepos;

    int polygon;
    EM::SectionID sid;
    const float mindist = getNearestPolygon( polygon, sid, pos, zfactor );
    if ( mIsUdf(mindist) )
    {
	if ( !emObject().nrSections() )
	    return;

	sid = emObject().sectionID( 0 );
	const Geometry::Element* ge = emObject().sectionGeometry( sid );
	if ( !ge ) return;

	mDynamicCastGet(const Geometry::PolygonSurface*,surface,ge);
	if ( !surface ) return;

	const StepInterval<int> rowrange = surface->rowRange();
	if ( !rowrange.isUdf() )
	    return;

	insertpid.setObjectID( emObject().id() );
	insertpid.setSectionID( sid );
	insertpid.setSubID( RowCol(0,0).toInt64() );
	return;
    }
    
    if ( fabs(mindist)>50 )
    {
	if ( !emObject().nrSections() )
	    return;

	sid = emObject().sectionID( 0 );
	const Geometry::Element* ge = emObject().sectionGeometry( sid );
	if ( !ge ) return;

	mDynamicCastGet(const Geometry::PolygonSurface*,surface,ge);
	if ( !surface ) return;

	const StepInterval<int> rowrange = surface->rowRange();
	if ( rowrange.isUdf() )
	    return;

	insertpid.setObjectID( emObject().id() );
	insertpid.setSectionID( sid );
	const int newpolygon = mindist>0 
	    ? polygon+rowrange.step 
	    : polygon==rowrange.start ? polygon-rowrange.step : polygon;

	insertpid.setSubID( RowCol(newpolygon,0).toInt64() );
	return;
    }
    
    getPidsOnPolygon( nearestpid0, nearestpid1, insertpid, polygon, sid, 
	    	      pos, zfactor );
}


bool PolygonBodyEditor::removeSelection( const Selector<Coord3>& selector )
{
    mDynamicCastGet( EM::PolygonBody*, polygonsurf, &emobject );
    if ( !polygonsurf )
	return false;

    bool change = false;
    for ( int sectidx=polygonsurf->nrSections()-1; sectidx>=0; sectidx--)
    {
	const EM::SectionID currentsid = polygonsurf->sectionID( sectidx );
	const Geometry::Element* ge = polygonsurf->sectionGeometry(currentsid);
	if ( !ge ) continue;

	mDynamicCastGet(const Geometry::PolygonSurface*,surface,ge);
	if ( !surface ) continue;

	const StepInterval<int> rowrange = surface->rowRange();
	if ( rowrange.isUdf() )
	    continue;

	for ( int polygonidx=rowrange.nrSteps(); polygonidx>=0; polygonidx-- )
	{
	    Coord3 avgpos( 0, 0, 0 );
	    const int curpolygon = rowrange.atIndex(polygonidx);
	    const StepInterval<int> colrange = surface->colRange( curpolygon );
	    if ( colrange.isUdf() )
		continue;

	    for ( int knotidx=colrange.nrSteps(); knotidx>=0; knotidx-- )
	    {
		const RowCol rc( curpolygon,colrange.atIndex(knotidx) );
		const Coord3 pos = surface->getKnot( rc );

		if ( !pos.isDefined() || !selector.includes(pos) )
		    continue;

		EM::PolygonBodyGeometry& fg = polygonsurf->geometry();
		const bool res = fg.nrKnots( currentsid,curpolygon)==1
		   ? fg.removePolygon( currentsid, curpolygon, true )
		   : fg.removeKnot( currentsid, rc.toInt64(), true );

		if ( res ) change = true;
	    }
	}
    }

    if ( change )
    {
	EM::EMM().undo().setUserInteractionEnd(
		EM::EMM().undo().currentEventID() );
    }

    return change;
}


float PolygonBodyEditor::getNearestPolygon( int& polygon, EM::SectionID& sid,
	const Coord3& mousepos, float zfactor ) const
{
    if ( !mousepos.isDefined() )
	return mUdf(float);

    int selsectionidx = -1, selpolygon;
    float mindist;

    for ( int sectionidx=emObject().nrSections()-1; sectionidx>=0; sectionidx--)
    {
	const EM::SectionID currentsid = emObject().sectionID( sectionidx );
	const Geometry::Element* ge = emObject().sectionGeometry( currentsid );
	if ( !ge ) continue;

	mDynamicCastGet(const Geometry::PolygonSurface*,surface,ge);
	if ( !surface ) continue;

	const StepInterval<int> rowrange = surface->rowRange();
	if ( rowrange.isUdf() ) continue;

	for ( int polygonidx=rowrange.nrSteps(); polygonidx>=0; polygonidx-- )
	{
	    Coord3 avgpos( 0, 0, 0 );
	    const int curpolygon = rowrange.atIndex(polygonidx);
	    const StepInterval<int> colrange = surface->colRange( curpolygon );
	    if ( colrange.isUdf() )
		continue;

	    int count = 0;
	    for ( int knotidx=colrange.nrSteps(); knotidx>=0; knotidx-- )
	    {
		const Coord3 pos = surface->getKnot(
			RowCol(curpolygon,colrange.atIndex(knotidx)));

		if ( pos.isDefined() )
		{
		    avgpos += mCompareCoord( pos );
		    count++;
		}
	    }

	    if ( !count ) continue;

	    avgpos /= count;

	    const Plane3 plane( surface->getPolygonNormal(curpolygon),
		    		avgpos, false );
	    const float disttoplane =
		plane.distanceToPoint( mCompareCoord(mousepos), true );

	    if ( selsectionidx==-1 || fabs(disttoplane)<fabs(mindist) )
	    {
		mindist = disttoplane;
		selpolygon = curpolygon;
		selsectionidx = sectionidx;
	    }
	}
    }

    if ( selsectionidx==-1 )
	return mUdf(float);

    sid = emObject().sectionID( selsectionidx );
    polygon = selpolygon;

    return mindist;
}


#define mRetNotInsideNext \
    if ( !nextdefined || (!sameSide3D(curpos,nextpos,v0,v1,0) && \
			  !sameSide3D(v0,v1,curpos,nextpos,0)) ) \
	return false;

#define mRetNotInsidePrev \
    if ( !prevdefined || (!sameSide3D(curpos,prevpos,v0,v1,0) && \
			  !sameSide3D(v0,v1,curpos,prevpos,0)) ) \
	return false;


bool PolygonBodyEditor::setPosition( const EM::PosID& pid, const Coord3& mpos )
{
    if ( !mpos.isDefined() ) return false;
    
    const BinID bid = SI().transform( mpos );
    if ( !SI().inlRange( true ).includes(bid.inl,false) || 
	 !SI().crlRange( true ).includes(bid.crl,false) || 
	 !SI().zRange( true ).includes(mpos.z,false) )
	return false;

    const Geometry::Element* ge = emObject().sectionGeometry(pid.sectionID());
    mDynamicCastGet( const Geometry::PolygonSurface*, surface, ge );
    if ( !surface ) return false;

    const RowCol rc = pid.getRowCol();
    const StepInterval<int> colrg = surface->colRange( rc.row );
    if ( colrg.isUdf() ) return false;
	
    const bool addtoundo = changedpids.indexOf(pid) == -1;
    if ( addtoundo )
	changedpids += pid;

    if ( colrg.nrSteps()<3 )
	return emobject.setPos( pid, mpos, addtoundo );

    const int zscale =  SI().zDomain().userFactor();   
    const int previdx = rc.col==colrg.start ? colrg.stop : rc.col-colrg.step;
    const int nextidx = rc.col<colrg.stop ? rc.col+colrg.step : colrg.start;
    
    Coord3 curpos = mpos; curpos.z *= zscale;
    Coord3 prevpos = surface->getKnot( RowCol(rc.row, previdx) );
    Coord3 nextpos = surface->getKnot( RowCol(rc.row, nextidx) );
    
    const bool prevdefined = prevpos.isDefined();
    const bool nextdefined = nextpos.isDefined();
    if ( prevdefined ) prevpos.z *= zscale; 
    if ( nextdefined ) nextpos.z *= zscale;

    for ( int knot=colrg.start; knot<=colrg.stop; knot += colrg.step )
    {
	const int nextknot = knot<colrg.stop ? knot+colrg.step : colrg.start;
	if ( knot==previdx || knot==rc.col )
	    continue;

	Coord3 v0 = surface->getKnot( RowCol(rc.row, knot) ); 
	Coord3 v1 = surface->getKnot( RowCol(rc.row,nextknot));
	if ( !v0.isDefined() || !v1.isDefined() )
 	    return false;

	v0.z *= zscale;
	v1.z *= zscale;
	if ( previdx==nextknot )
	{
	    mRetNotInsideNext
	}
	else if ( knot==nextidx ) 
	{
	    mRetNotInsidePrev
	} 
	else 
	{
	    mRetNotInsidePrev
	    mRetNotInsideNext
	}
    }

    return emobject.setPos( pid, mpos, addtoundo );
}


void PolygonBodyEditor::getPidsOnPolygon(  EM::PosID& nearestpid0,
	EM::PosID& nearestpid1, EM::PosID& insertpid, int polygon,
	const EM::SectionID& sid, const Coord3& mousepos, float zfactor ) const
{
    nearestpid0 = EM::PosID::udf();
    nearestpid1 = EM::PosID::udf();
    insertpid = EM::PosID::udf();
    if ( !mousepos.isDefined() ) return;

    const Geometry::Element* ge = emObject().sectionGeometry( sid );
    mDynamicCastGet(const Geometry::PolygonSurface*,surface,ge);
    if ( !surface ) return;

    const StepInterval<int> colrange = surface->colRange( polygon );
    if ( colrange.isUdf() ) return;
   
    const Coord3 mp = mCompareCoord(mousepos);
    TypeSet<int> knots;
    int nearknotidx = -1;
    Coord3 nearpos;
    float minsqptdist;
    for ( int knotidx=0; knotidx<colrange.nrSteps()+1; knotidx++ )
    {
	const Coord3 pt = 
	    surface->getKnot( RowCol(polygon,colrange.atIndex(knotidx)) );
	if ( !pt.isDefined() )
	    continue;

	float sqdist = 0;
	if ( sowinghistory_.isEmpty() || sowinghistory_[0]!=pt )
	{
	    sqdist = mCompareCoord(pt).sqDistTo( mCompareCoord(mousepos) );
	    if ( mIsZero(sqdist, 1e-4) ) //mousepos is duplicated.
		return;
	}

	 if ( nearknotidx==-1 || sqdist<minsqptdist )
	 {
	     minsqptdist = sqdist;
	     nearknotidx = knots.size();
	     nearpos = mCompareCoord( pt );
	 }
	 	 
	 knots += colrange.atIndex( knotidx );
    }

    if ( nearknotidx==-1 )
	return;

    nearestpid0.setObjectID( emObject().id() );
    nearestpid0.setSectionID( sid );
    nearestpid0.setSubID( RowCol(polygon,knots[nearknotidx]).toInt64() );
    if ( knots.size()<=2 )
    {
	insertpid = nearestpid0;
	insertpid.setSubID( RowCol(polygon,knots.size()).toInt64() );
	return;
    }

    double minsqedgedist = -1;
    int nearedgeidx;
    Coord3 v0, v1;
    for ( int knotidx=0; knotidx<knots.size(); knotidx++ )
    {
	const int col = knots[knotidx];
	Coord3 p0 = surface->getKnot(RowCol(polygon,col));
	Coord3 p1 = surface->getKnot( RowCol(polygon,
		    knots [ knotidx<knots.size()-1 ? knotidx+1 : 0 ]) );
	if ( !p0.isDefined() || !p1.isDefined() )
  	    continue;

	p0.z *= zfactor;
  	p1.z *= zfactor;

	const double t = (mp-p0).dot(p1-p0)/(p1-p0).sqAbs();
	if ( t<0 || t>1 )
	    continue;

	const double sqdist = mp.sqDistTo(p0+t*(p1-p0));
	if ( minsqedgedist==-1 || sqdist<minsqedgedist )
	{
	    minsqedgedist = sqdist;
	    nearedgeidx = knotidx;
	    v0 = p0;
	    v1 = p1;
	}
    }

    bool usenearedge = false;
    if ( minsqedgedist!=-1 && sowinghistory_.size()<=1 )
    {
	if ( nearknotidx==nearedgeidx ||
	     nearknotidx==(nearknotidx<knots.size()-1 ? nearknotidx+1 : 0) ||  	
	     ((v1-nearpos).cross(v0-nearpos)).dot((v1-mp).cross(v0-mp))<0  ||
	     minsqedgedist<minsqptdist )
	    usenearedge = true;
    }

    if ( usenearedge ) //use nearedgeidx only
    {
	if ( nearedgeidx<knots.size()-1 )
	{
	    nearestpid0.setSubID( 
		    RowCol(polygon,knots[nearedgeidx]).toInt64() );
	    nearestpid1 = nearestpid0;
	    nearestpid1.setSubID( 
		    RowCol(polygon,knots[nearedgeidx+1]).toInt64() );
	
	    insertpid = nearestpid1;
	}
	else
	{
	    insertpid = nearestpid0;
	    const int nextcol = knots[nearedgeidx]+colrange.step;
	    insertpid.setSubID( RowCol(polygon,nextcol).toInt64() );
	}
	    
	return;
    }
    else  //use nearknotidx only
    {
	Coord3 prevpos = surface->getKnot( RowCol(polygon,
		    knots[nearknotidx ? nearknotidx-1 : knots.size()-1]) );
	Coord3 nextpos = surface->getKnot( RowCol(polygon,
		    knots[nearknotidx<knots.size()-1 ? nearknotidx+1 : 0]) );

	bool takeprevious;
	if ( sowinghistory_.size() <= 1 )
	{
	    const bool prevdefined = prevpos.isDefined();
	    const bool nextdefined = nextpos.isDefined();	
	    if ( prevdefined ) prevpos.z *= zfactor;
	    if ( nextdefined ) nextpos.z *= zfactor;

	    takeprevious = prevdefined && nextdefined &&
			   sameSide3D(mp,prevpos,nearpos,nextpos,1e-3); 
	}
	else
	    takeprevious = sowinghistory_[1]==prevpos;

	if ( takeprevious )
	{
	    if ( nearknotidx )
	    {
		nearestpid1 = nearestpid0;
		nearestpid1.setSubID( 
			RowCol(polygon,knots[nearknotidx-1]).toInt64() );
		insertpid = nearestpid0;
	    }
	    else
	    {
		insertpid = nearestpid0;
		const int insertcol = knots[nearknotidx]-colrange.step;
		insertpid.setSubID(RowCol(polygon,insertcol).toInt64());
	    }
	}
	else 
	{
	    if ( nearknotidx<knots.size()-1 )
	    {
		nearestpid1 = nearestpid0;
		nearestpid1.setSubID( 
			RowCol(polygon,knots[nearknotidx+1]).toInt64() );
		insertpid = nearestpid1;
	    }
	    else
	    {
		insertpid = nearestpid0;
		const int insertcol = knots[nearknotidx]+colrange.step;
		insertpid.setSubID(RowCol(polygon,insertcol).toInt64());
	    }
	}
    }
}


};  // namespace MPE
