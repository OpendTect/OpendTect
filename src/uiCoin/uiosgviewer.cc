/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiosgviewer.h"

#include "uiobjbody.h"
#include "keystrs.h"

#include "survinfo.h"
#include "viscamera.h"
#include "vissurvscene.h"
#include "visdatagroup.h"

#include <osgViewer/View>
#include <osgViewer/CompositeViewer>
#include <osgQt/GraphicsWindowQt>

class uiOsgViewer
{
public:
			uiOsgViewer();
			~uiOsgViewer();

    void                addView(uiOsgViewHandle*); //Make sure it's kept alive
    void                removeView(uiOsgViewHandle*);

    static uiOsgViewer& getCommonViewer();

protected:
    osg::ref_ptr<osgViewer::CompositeViewer>	osgviewer_;
    ObjectSet<uiOsgViewHandle>     		views_;
};


uiOsgViewHandle::uiOsgViewHandle()
    : osgview_( 0 )
{}


void uiOsgViewHandle::detachView()
{
    if ( viewer_ )
	viewer_->removeView( this );
    viewer_ = 0;

    if ( osgview_ ) osgview_->unref();
    osgview_ = 0;
}


void uiOsgViewHandle::setOsgView( osgViewer::View* view )
{
    if ( osgview_ )
    {
	pErrMsg("Never, Never do this!" );
	return;
    }

    osgview_ = view;
    osgview_->ref();
    uiOsgViewer::getCommonViewer().addView( this );
}


uiOsgViewer& uiOsgViewer::getCommonViewer()
{
    static uiOsgViewer viewer;
    return viewer;
}


uiOsgViewer::uiOsgViewer()
    : osgviewer_( new osgViewer::CompositeViewer )
{
    osgviewer_->ref();
    osgviewer_->setThreadingModel( osgViewer::ViewerBase::SingleThreaded );
    osgviewer_->getEventVisitor()->setTraversalMask( visBase::EventTraversal );
    osgQt::setViewer( osgviewer_.get() );
}


uiOsgViewer::~uiOsgViewer()
{
    for ( int idx=views_.size()-1; idx>=0; idx-- )
    {
	removeView( views_[idx] );
	views_[idx]->setViewer( 0 );
    }

    osgviewer_->unref();
}


void uiOsgViewer::addView(uiOsgViewHandle* view )
{
    osgviewer_->addView( view->getOsgView() );
    view->setViewer( this );
    views_ += view;
}


void uiOsgViewer::removeView( uiOsgViewHandle* view )
{
    osgviewer_->removeView( view->getOsgView() );
    views_ -= view;
}
