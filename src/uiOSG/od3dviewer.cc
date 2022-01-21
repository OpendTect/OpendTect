/*+
________________________________________________________________________

 Copyright:	dGB Beheer B.V.
 License:	https://dgbes.com/index.php/licensing
 Author:	Nanne Hemstra
 Date:		September 2021
________________________________________________________________________

-*/

#include "od3dviewer.h"

#include "ui3dviewer.h"
#include "visaxes.h"
#include "viscamera.h"
#include "vissurvscene.h"
#include "visthumbwheel.h"

#include "osgGeo/TrackballManipulator"

#include <osg/DisplaySettings>
#include <osg/Switch>


OD3DViewer::OD3DViewer( ui3DViewer& vwr, uiParent* p )
    : uiObjectBody(p,nullptr)
    , offscreenrenderswitch_(new osg::Switch)
    , offscreenrenderhudswitch_(new osg::Switch)
    , ui3dviewer_(vwr)
{
}


OD3DViewer::~OD3DViewer()
{
}


uiObject& OD3DViewer::uiObjHandle()
{
    return ui3dviewer_;
}


const QWidget* OD3DViewer::qwidget_() const
{
    return nullptr;
}


void OD3DViewer::setSceneID( int sceneid )
{
}



visBase::Scene* OD3DViewer::getScene()
{
    return scene_;
}


const visBase::Scene* OD3DViewer::getScene() const
{
    return cCast(OD3DViewer*,this)->getScene();
}



void OD3DViewer::setStereoType( ui3DViewer::StereoType st )
{
    stereotype_ = st;
    osg::ref_ptr<osg::DisplaySettings> ds = osg::DisplaySettings::instance();
    switch( st )
    {
    case ui3DViewer::None:
	ds->setStereo( false );
	break;
    case ui3DViewer::RedCyan:
	ds->setStereo( true );
	ds->setStereoMode( osg::DisplaySettings::ANAGLYPHIC );
	break;
    case ui3DViewer::QuadBuffer:
	ds->setStereo( true );
	ds->setStereoMode( osg::DisplaySettings::QUAD_BUFFER );
	break;
    default:
	ds->setStereo( false );
    }
}


ui3DViewer::StereoType OD3DViewer::getStereoType() const
{
    return stereotype_;
}


void OD3DViewer::setStereoOffset( float offset )
{
    osg::ref_ptr<osg::DisplaySettings> ds = osg::DisplaySettings::instance();
    stereooffset_ = offset;
    ds->setEyeSeparation( stereooffset_/100 );
    requestRedraw();
}


float OD3DViewer::getStereoOffset() const
{
    return stereooffset_;
}


void OD3DViewer::setBackgroundColor( const OD::Color& col )
{
    visBase::Scene* scene = getScene();
    if ( scene )
	scene->setBackgroundColor( col );
}


void OD3DViewer::setAnnotationColor( const OD::Color& col )
{
/*
    axes_->setAnnotationColor( col );
    horthumbwheel_->setAnnotationColor( col );
    verthumbwheel_->setAnnotationColor( col );
    distancethumbwheel_->setAnnotationColor( col );
*/
}


void OD3DViewer::requestRedraw()
{
}
