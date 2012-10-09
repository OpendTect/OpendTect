/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id$";


#include "SoPolygonSelect.h"

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/caches/SoCache.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/system/gl.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoSceneManager.h>


SO_NODE_SOURCE(SoPolygonSelect);

void SoPolygonSelect::initClass(void)
{
    SO_NODE_INIT_CLASS(SoPolygonSelect, SoNode, "Node");
    SO_ENABLE( SoGLRenderAction, SoViewVolumeElement );
    SO_ENABLE( SoHandleEventAction, SoViewportRegionElement );
}


SoPolygonSelect::SoPolygonSelect(void) 
    : mousedown_( false )
    , isgrabbing_( false )
    , dependencychecker_( 0 )
{
    SO_NODE_CONSTRUCTOR( SoPolygonSelect );

    SO_NODE_ADD_FIELD( mode, (SoPolygonSelect::POLYGON) );

    SO_NODE_DEFINE_ENUM_VALUE( Modes, OFF );
    SO_NODE_DEFINE_ENUM_VALUE( Modes, RECTANGLE );
    SO_NODE_DEFINE_ENUM_VALUE( Modes, POLYGON );
    SO_NODE_SET_SF_ENUM_TYPE( mode, Modes );
}


static SoRayPickAction* raypickaction_ = 0;

SoPolygonSelect::~SoPolygonSelect()
{
    if ( dependencychecker_ ) dependencychecker_->unref();

    if ( raypickaction_ )
    {
	delete raypickaction_;
	raypickaction_ = 0;
    }
}


SbVec2f SoPolygonSelect::projectPointToScreen( const SbVec3f& pt ) const
{
    SbVec3f modelpt;
    modelmatrix_.multMatrixVec( pt, modelpt );

    SbVec3f res;
    vv_.projectToScreen( modelpt, res );

    if ( res[2] > 1.0 )
	return SbVec2f( 1e30, 1e30 ); // Undefined

    return SbVec2f( res[0], res[1] );
}


bool SoPolygonSelect::projectPointFromScreen( const SbVec2f& pt,
       					      SbLine& line ) const
{
    SbLine modelline;
    vv_.projectPointToLine( pt, modelline );

    const SbMatrix inversemodelmatrix = modelmatrix_.inverse();
    inversemodelmatrix.multLineMatrix( modelline, line );

    return true;
}


void SoPolygonSelect::GLRender( SoGLRenderAction* action )
{
    SoState* state = action->getState();

    if ( dependencychecker_ && !dependencychecker_->isValid(state) )
    {
	if ( !mousedown_ )
	    polygon_.truncate( 0 );

	dependencychecker_->unref();
	dependencychecker_ = 0;
	polygonChange.invokeCallbacks( this );
    }

    if ( !dependencychecker_ )
    {
	const SbBool storedinvalid = SoCacheElement::setInvalid( false ); 
	const SoElement* elt =
	    state->getConstElement( SoViewVolumeElement::getClassStackIndex());
	state->push();

	dependencychecker_ = new SoCache( state );
	dependencychecker_->ref();
	dependencychecker_->addElement( elt );
	SoCacheElement::set(state, dependencychecker_); 
	state->pop();
	SoCacheElement::setInvalid(storedinvalid); 
	modelmatrix_ = SoModelMatrixElement::get(state);
	vv_ = SoViewVolumeElement::get(state);
    }

    if ( polygon_.getLength()<2 )
	return;

    SoMaterialBundle mb(action);
    mb.sendFirst();

    const SbMatrix world2model = modelmatrix_.inverse();

    glBegin( GL_LINE_LOOP );
    const SbPlane plane = vv_.getPlane( 0 );
    const SbVec3f normal = plane.getNormal();
    glNormal3fv( normal.getValue() );

    const float zpos = vv_.nearDist + 0.001*vv_.nearToFar;

    for ( int idx=0; idx<polygon_.getLength(); idx++ )
    {
	const SbVec3f worldvec = vv_.getPlanePoint( zpos, polygon_[idx] );
	SbVec3f modelvec;
	world2model.multMatrixVec( worldvec, modelvec );
	glVertex3fv( modelvec.getValue() );
    }

    glEnd();
}


static float tabletpressure_ = 0.0;

void SoPolygonSelect::setTabletPressure( float newpressure )
{ tabletpressure_ = newpressure; }


void SoPolygonSelect::handleEvent( SoHandleEventAction* action )
{
    if ( action->isHandled() )
	return;

    if ( mode.getValue()==OFF )
	return;

    SoState* state = action->getState();
    const SoEvent* event = action->getEvent();

    if ( mousedown_ && SoMouseButtonEvent::isButtonReleaseEvent (
		                    event, SoMouseButtonEvent::BUTTON1 ))
    {
	if ( isgrabbing_ )
	    action->releaseGrabber();

	isgrabbing_ = false;

	mousedown_ = false;
	action->setHandled();
	paintStop.invokeCallbacks( this );
    }
    else if ( mousedown_ &&
	      event->getTypeId()==SoLocation2Event::getClassTypeId() )
    {
	action->setHandled();
	const SbVec2f pt =
	    event->getNormalizedPosition(SoViewportRegionElement::get(state));
	if ( mode.getValue()==RECTANGLE )
	{
	    if ( polygon_.getLength()!=4 )
	    {
		polygon_.append( SbVec2f(0,0) );
		polygon_.append( SbVec2f(0,0) );
		polygon_.append( SbVec2f(0,0) );
	    }

	    const SbVec2f& origin = polygon_[0];
	    polygon_[1] = SbVec2f( origin[0], pt[1] );
	    polygon_[2] = pt;
	    polygon_[3] = SbVec2f( pt[0], origin[1] );
	}
	else
	{
	    if ( polygon_.getLength()>50000 )
		return;

	    if ( mode.getValue()==POLYGON && tabletpressure_>0.0 )
		rub( pt );
	    else
		polygon_.append( pt );
	}

	polygonChange.invokeCallbacks( this );
	touch();
    }
    else if ( SoMouseButtonEvent::isButtonPressEvent (
		    event, SoMouseButtonEvent::BUTTON1 ) )
    {
	mousedown_ = true;
	action->setHandled();
	if ( !action->getGrabber() )
	{
	    isgrabbing_ = true;
	    action->setGrabber( this );
	}
	else
	    isgrabbing_ = false;

	polygon_.truncate( 0 );
	const SbVec2f pt =
	    event->getNormalizedPosition(SoViewportRegionElement::get(state));


	if ( mode.getValue()==POLYGON && tabletpressure_>0.0 )
	    initRub( pt );
	else
	    polygon_.append( pt );

	paintStart.invokeCallbacks( this );
	polygonChange.invokeCallbacks( this );
	touch();
    }
}


void SoPolygonSelect::clear()
{
    if ( !mousedown_ )
	polygon_.truncate( 0 );
}



void SoPolygonSelect::initRub( const SbVec2f& pt )
{
    prevpos_ = pt;
    rub( pt );
}


static void transform( SbList<SbVec2f>& poly, float factor,
		       float cosphi, float sinphi )
{
    for ( int idx=poly.getLength()-1; idx>=0; idx-- )
    {
	const SbVec2f vec = factor * poly[idx];
	poly[idx][0] = cosphi*vec[0] - sinphi*vec[1];
	poly[idx][1] = sinphi*vec[0] + cosphi*vec[1];
    }
}


static SbVec2f normalizedOutDir( const SbVec2f& dir, float rublen )
{
    SbVec2f normdir = dir / dir.length();

    if ( normdir[0] && normdir[1]>-1 && normdir[1]<1 )
	return normdir;

    if ( normdir[1] < 0 )
	return SbVec2f( (dir[0]>0 ? rublen : 0), -1 );

    return SbVec2f( (dir[0]<0 ? -rublen : 0), 1 );
}


static float outDirPar( const SbVec2f& dir, float rublen )
{
    if ( dir[1] <= -1 )
	return dir[0];
    if ( dir[1] >= 1 )
	return rublen + 2 - dir[0];
    if ( dir[0] < 0 )
	return dir[1]<0 ? dir[0] : -2-dir[0];

    return rublen + (dir[1]<0 ? dir[0] : 2-dir[0]);
}


#define mNoDir( dir ) ( !dir[0] && !dir[1] )

void SoPolygonSelect::rub( const SbVec2f& pt )
{
    const float delta = 0.002;

    float rubradius = 0.3 * (tabletpressure_ - 0.2);

    if ( rubradius < 0.005 )
	rubradius = 0.005;

    const SbVec2f movevec = pt - prevpos_;
    const float movelen = movevec.length();

    const float cosphi = movelen ? movevec[0]/movelen : 1;
    const float sinphi = movelen ? movevec[1]/movelen : 0;

    int sz = polygon_.getLength();
    polybuf_.copy( polygon_ );
    polybuf_.append( prevpos_ );
    polybuf_.append( pt );
    prevpos_ = pt;

    transform( polybuf_, 1./rubradius, cosphi, -sinphi );

    SbVec2f c1 = polybuf_[sz];
    SbVec2f c2 = polybuf_[sz+1];
    if ( c1[0] > c2[0] )
    {
	c1 = polybuf_[sz+1];
	c2 = polybuf_[sz];
    }
    const float rublen = c2[0] - c1[0];

    int leftmostoutidx = -1;
    float leftmostoutval = 1e30;
    outdir_.truncate( 0 );

    for ( int idx=0; idx<sz; idx++ )
    {
	const SbVec2f dif1 = polybuf_[idx] - c1;
	const SbVec2f dif2 = polybuf_[idx] - c2;

	outdir_.append( SbVec2f(0,0) );
	if ( dif1[0]>=0 && dif2[0]<=0 )
	{
	    if ( dif1[1] < -1 )
		outdir_[idx] = SbVec2f( dif1[0], -1 );
	    else if ( dif2[1] > 1 )
		outdir_[idx] = SbVec2f( dif2[0], 1 );
	}
	else
	{
	    const SbVec2f dif = dif1[0]<0 ? dif1 : dif2;
	    const float diflen = dif.length();
	    if ( diflen > 1 )
		outdir_[idx] = normalizedOutDir( dif, rublen );
	}

	if ( !mNoDir(outdir_[idx]) && polybuf_[idx][0]<leftmostoutval )
	{
	    leftmostoutidx = idx;
	    leftmostoutval = polybuf_[idx][0];
	}
    }

    if ( leftmostoutidx == -1 )		// polygon in kernel
    {
	polybuf_[sz] = c1 + SbVec2f(0,-1);
	outdir_.append( SbVec2f(0,-1) );
	outdir_.append( SbVec2f(0,0) );
	leftmostoutidx = sz;
	sz += 2;
    }

    int startidx = leftmostoutidx;
    while ( true )
    {
	int idx = startidx - 1;
	if ( idx < 0 )
	    idx += sz;

	if ( idx == leftmostoutidx )	// kernel in polygon
	    return; 

	if ( mNoDir(outdir_[idx]) )
	    break;

	startidx = idx;
    }

    polygon_.truncate( 0 );

    int lastoutidx = -1;
    for ( int cnt=0; cnt<=sz; cnt++ )
    {
	int idx = startidx + cnt;
	if ( idx >= sz )
	    idx -= sz;
	int previdx = idx - 1;
	if ( previdx < 0 )
	    previdx += sz;

	if ( mNoDir(outdir_[idx]) )
	{
	    if ( lastoutidx == -1 )
		lastoutidx = previdx;
	}
	else if ( mNoDir(outdir_[previdx]) && cnt )
	{
	    const float t0 = outDirPar( outdir_[startidx], rublen );
	    const float t1 = outDirPar( outdir_[lastoutidx], rublen );
	    const float t2 = outDirPar( outdir_[idx], rublen );

	    if ( t0==t1 || (t0>t1 && t1<=t2 && t2<=t0)
			|| (t0<t1 && (t2<=t0 || t1<=t2)) )
	    {
		SbVec2f dir = outdir_[lastoutidx];
		float s0 = t1;

		while ( true )
		{
		    const SbVec2f center = s0<=rublen ? c1 : c2;
		    polygon_.append( center + dir );

		    if ( dir[1]<=-1 || dir[1]>=1 )
		    {
			dir[0] -= dir[1] * delta / rubradius;
			if ( fabs(dir[0]) > rublen )
			{
			    dir[0] += rublen * dir[1];
			    dir = normalizedOutDir( dir, rublen );
			}
		    }
		    else 
		    {
			const int sign = dir[0]<0 ? -1 : 1;
			dir += SbVec2f( -dir[1], dir[0] ) * delta / rubradius;
			dir = normalizedOutDir( dir, rublen );
			if ( sign*dir[0] <= 0 )
			    dir[1] = sign;
		    }

		    float s1 = outDirPar( dir, rublen );


		    if ( (s0<s1 && s0<t2 && t2<=s1 ) ||
			 (s0>s1 && (t2<=s1 || s0<t2)) ) break;

		    s0 = s1;
		}
		const SbVec2f center = t2<=rublen ? c1 : c2;
		polygon_.append( center + outdir_[idx] );

		lastoutidx = -1;
	    }
	}

	if ( lastoutidx==-1 && cnt<sz )
	    polygon_.append( polybuf_[idx] );
    }

    transform( polygon_, rubradius, cosphi, sinphi );
}


static SoSceneManager* activescenemgr_ = 0;

void SoPolygonSelect::setActiveSceneManager( SoSceneManager* scenemgr )
{ activescenemgr_ = scenemgr; }


const SoPath* SoPolygonSelect::rayPickThrough( const SbVec3f& displaypos,
					       int depthidx ) const
{
    if ( !activescenemgr_ )
	return 0;

    const SbViewportRegion& svpr( activescenemgr_->getViewportRegion() );

    if ( raypickaction_ )
	raypickaction_->setViewportRegion( svpr );
    else
	raypickaction_ = new SoRayPickAction( svpr );

    SbVec2f screenpos = projectPointToScreen( displaypos );
    raypickaction_->setNormalizedPoint( screenpos );
    raypickaction_->setRadius( 1 );
    raypickaction_->setPickAll( depthidx>0 );

    raypickaction_->apply( activescenemgr_->getSceneGraph() );
    SoPickedPoint* pickedpoint = raypickaction_->getPickedPoint( depthidx );
    return pickedpoint ? pickedpoint->getPath() : 0;
}
