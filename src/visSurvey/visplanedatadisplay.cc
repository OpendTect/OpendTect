/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visplanedatadisplay.cc,v 1.1 2002-03-21 14:05:11 bert Exp $";

#include "visplanedatadisplay.h"
#include "geompos.h"
#include "vistexturerect.h"
#include "volumeaccess.h"
#include "arrayndimpl.h"
#include "position.h"
#include "survinfo.h"
#include "visselman.h"
#include "visdataman.h"
#include "visrectangle.h"

#include "sorting.h"

visSurvey::PlaneDataDisplay::PlaneDataDisplay(
			visSurvey::PlaneDataDisplay::Type type_ )
	: trect( visBase::TextureRect::create(true) )
	, type( type_ )
	, selected( false )
	, attrsel( -1, false )
{
    trect->ref();
    trect->selection()->notify( mCB(this,PlaneDataDisplay,select));
    trect->deSelection()->notify( mCB(this,PlaneDataDisplay,deSelect));

    BinID startbid = SI().range().start;
    BinID stopbid = SI().range().stop;
    StepInterval<double> vrgd = SI().zRange();

    StepInterval<float> inlrange( startbid.inl, stopbid.inl, SI().step().inl );
    StepInterval<float> crlrange( startbid.crl, stopbid.crl, SI().step().crl );
    StepInterval<float> vrg( vrgd.start, vrgd.stop, vrgd.step );

    if ( type==Inline )
    {
	trect->getRectangle().setOrientation( visBase::Rectangle::YZ );
	trect->getRectangle().setOrigo(
		Geometry::Pos(inlrange.start, crlrange.start, vrg.start ));
	trect->getRectangle().setWidth( vrg.width(), crlrange.width() );

	trect->getRectangle().setRange( 0, vrg );
	trect->getRectangle().setRange( 1, crlrange );
	trect->getRectangle().setRange( 2, inlrange );

	trect->getRectangle().setDraggerSize( vrg.width()*0.1,crlrange.width()*0.003,
				   inlrange.width()*0.05);
    }
    else if ( type==Crossline )
    {
	trect->getRectangle().setOrientation( visBase::Rectangle::XZ );
	trect->getRectangle().setOrigo(
		Geometry::Pos(inlrange.start, crlrange.start, vrg.start ));
	trect->getRectangle().setWidth( inlrange.width(), vrg.width() );

	trect->getRectangle().setRange( 0, inlrange );
	trect->getRectangle().setRange( 1, vrg );
	trect->getRectangle().setRange( 2, crlrange );
	trect->getRectangle().setDraggerSize( inlrange.width()*0.1, 0.1,
				   crlrange.width()*0.1 );
    }
    else
    {
	trect->getRectangle().setOrientation( visBase::Rectangle::XY );
	trect->getRectangle().setOrigo(
		Geometry::Pos(inlrange.start, crlrange.start, vrg.start ));
	trect->getRectangle().setWidth( inlrange.width(), crlrange.width() );

	trect->getRectangle().setRange( 0, inlrange );
	trect->getRectangle().setRange( 1, crlrange );
	trect->getRectangle().setRange( 2, vrg );
	trect->getRectangle().setDraggerSize( inlrange.width()*0.15,
				   crlrange.width()*0.15, 1 );
    }

    trect->getRectangle().setSnapping( true );
    trect->useTexture( false );
}


visSurvey::PlaneDataDisplay::~PlaneDataDisplay()
{
    trect->selection()->remove( mCB(this,PlaneDataDisplay,select));
    trect->deSelection()->remove( mCB(this,PlaneDataDisplay,deSelect));

    trect->unRef();
}


int visSurvey::PlaneDataDisplay::getNewTextureData(bool manippos)
{
    if ( calclock.isLocked() ) return -1;

    calclock.lock();
    BinIDSampler bidsel;

    bidsel.start.inl = mNINT(manippos
			? trect->getRectangle().manipOrigo().x
			: trect->getRectangle().origo().x);
    bidsel.start.crl = mNINT(manippos
			? trect->getRectangle().manipOrigo().y
			: trect->getRectangle().origo().y);

    bidsel.stop = bidsel.start;

    StepInterval<float> zrg;
    zrg.step = SI().zRange().step;

    zrg.start = (manippos
	    ? trect->getRectangle().manipOrigo().z
	    : trect->getRectangle().origo().z);
    zrg.stop = zrg.start;

    if ( trect->getRectangle().orientation()==visBase::Rectangle::XY )
    {
	stopbid.inl += mNINT(trect->getRectangle().width( 0 ));
	stopbid.crl += mNINT(trect->getRectangle().width( 1 ));
    }
    else if ( trect->getRectangle().orientation()==visBase::Rectangle::XZ )
    {
	stopbid.inl += mNINT(trect->getRectangle().width( 0 ));
	zrg.stop += trect->getRectangle().width( 1 );
    }
    else
    {
	stopbid.crl += mNINT(trect->getRectangle().width(1));
	zrg.stop += trect->getRectangle().width(0);
    }



    const char* res = manager.dTectEng().createAttribValues( vr );

    if ( res )
    {
	cerr << "Canceled" << endl;
	trect->getRectangle().resetManip();
	return 0;
    }
    else
    {
	const ValuedVolumeAccess& va =
	      manager.dTectEng().getAttribValues( elemId() );

	int nrlines = va.numberOfLines( 0 );

	if ( !nrlines )
	{
	    trect->getRectangle().resetManip();
	    return 1;
	}
	
	int maxnrpoints = 0;
	for ( int idx=0; idx<nrlines; idx++ )
	    maxnrpoints = mMAX( maxnrpoints, va.numberOfPoints( 0, idx ));

	if ( !maxnrpoints )
	{
	    trect->getRectangle().resetManip();
	    return -1;
	}
	
	int nrinl = ( stopbid.inl - startbid.inl ) / SI().step().inl + 1;
	int nrcrl = ( stopbid.crl - startbid.crl ) / SI().step().crl + 1;
	bool inlwise = nrcrl > nrinl;

	VolumeAccess::Index volpos;

	if ( va.sliceType() == VolumeAccess::Hor )
	{
  	    Array2DImpl<float> arraydata( inlwise ? nrlines : maxnrpoints,
					  inlwise ? maxnrpoints : nrlines );

	    for ( volpos.line_index=0;
		  volpos.line_index<nrlines;
		  volpos.line_index++ )
	    {
		for ( volpos.point_index=0;
		      volpos.point_index<maxnrpoints;
		      volpos.point_index++ )
		{
		    float val =
			volpos.point_index<va.numberOfPoints(0,
					volpos.line_index )
			? va.value( volpos ) : 0;

		    arraydata.set( 
			inlwise ? volpos.line_index : volpos.point_index, 
			inlwise ? volpos.point_index : volpos.line_index,
			val );
		}
	    }

	    trect->setData( arraydata );
	    trect->useTexture( true );
	}
	else if ( va.sliceType() == VolumeAccess::Inline )
	{
	    int crosslines[nrlines];
	    int idxs[nrlines];

	    for ( volpos.line_index=0;
		  volpos.line_index<nrlines;
		  volpos.line_index++ )
	    {
		VolumeAccess::Pos pos = va.position( volpos );
		BinID binid = SI().transform( Coord( pos.x, pos.y ));
		crosslines[volpos.line_index] = binid.crl;
		idxs[volpos.line_index] = volpos.line_index;
	    }

	    sort_coupled( crosslines, idxs, nrlines );

	    Array2DImpl<float> arraydata( maxnrpoints, nrlines );

	    for ( int idx=0; idx<nrlines; idx++ )
	    {
		volpos.line_index = idxs[idx];

		for ( volpos.point_index=0;
		      volpos.point_index<maxnrpoints;
		      volpos.point_index++ )
		{
		    float val = va.value( volpos );
		    arraydata.set( volpos.point_index, idx, val);
		}
	    }

	    trect->setData( arraydata );
	    trect->useTexture( true );
	}
	else 
	{
	    int inlines[nrlines];
	    int idxs[nrlines];

	    for ( volpos.line_index=0;
		  volpos.line_index<nrlines;
		  volpos.line_index++ )
	    {
		VolumeAccess::Pos pos = va.position( volpos );
		BinID binid = SI().transform( Coord( pos.x, pos.y ));
		inlines[volpos.line_index] = binid.inl;
		idxs[volpos.line_index] = volpos.line_index;
	    }

	    sort_coupled( inlines, idxs, nrlines );
	    if ( inlines[0] != startbid.inl )
	    {
		
	    }
	    Array2DImpl<float> arraydata( nrlines, maxnrpoints );

	    for ( int idx=0; idx<nrlines; idx++ )
	    {
		volpos.line_index = idxs[idx];

		for ( volpos.point_index=0;
		      volpos.point_index<maxnrpoints;
		      volpos.point_index++ )
		{
		    float val =
			volpos.point_index<va.numberOfPoints(0,
					volpos.line_index )
			? va.value( volpos ) : 0;

		    arraydata.set( volpos.line_index, volpos.point_index, val);
		}
	    }

	    trect->setData( arraydata );
	    trect->useTexture( true );
	}
    }
    
    return 1;
}


void visSurvey::PlaneDataDisplay::turnOn(bool n) { trect->turnOn(n); }


bool visSurvey::PlaneDataDisplay::isOn() const { return trect->isOn(); }


void visSurvey::PlaneDataDisplay::setMaterial( visBase::Material* nm)
{ trect->setMaterial(nm); }


const visBase::Material* visSurvey::PlaneDataDisplay::getMaterial() const
{ return trect->getMaterial(); }


visBase::Material* visSurvey::PlaneDataDisplay::getMaterial()
{ return trect->getMaterial(); }


SoNode* visSurvey::PlaneDataDisplay::getData() { return trect->getData(); }


void visSurvey::PlaneDataDisplay::select()
{
    if ( selected ) return;
    selected = true;
    trect->getRectangle().displayDraggers( true );
}


void visSurvey::PlaneDataDisplay::deSelect()
{
    if ( !selected ) return;
    selected = false;

    if ( trect->getRectangle().isManipRectOnObject() )
    {
	trect->getRectangle().displayDraggers(false);
	return;
    }

    int res = -1;
    if ( elemId()!=-1 ) 
	res=getNewTextureData( true );
    
    if ( !res ) trect->getRectangle().resetManip();
    else
    {
	trect->getRectangle().moveObjectToManipRect();
	if ( res==-1 &&  trect->usesTexture() )
	    trect->useTexture( false );
    }

    trect->getRectangle().displayDraggers(false);
}


i_Notifier* visSurvey::PlaneDataDisplay::selection()
{ return trect->selection(); }


i_Notifier* visSurvey::PlaneDataDisplay::deSelection()
{ return trect->deSelection(); }
