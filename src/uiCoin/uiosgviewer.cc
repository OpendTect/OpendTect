/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uiosgviewer.h"

#include "uiobjbody.h"
#include "keystrs.h"

#include "survinfo.h"
#include "viscamera.h"
#include "vissurvscene.h"
#include "visdatagroup.h"

#ifdef __have_osg__
#include <osgViewer/View>
#include <osgViewer/CompositeViewer>
#include <osgQt/GraphicsWindowQt>
#endif

class uiOsgViewer
{
public:
			uiOsgViewer();
			~uiOsgViewer();

    void                addView(uiOsgViewHandle*); //Make sure it's kept alive
    void                removeView(uiOsgViewHandle*);

    static uiOsgViewer& getCommonViewer();

protected:
#ifdef __have_osg__
    osg::ref_ptr<osgViewer::CompositeViewer>	osgviewer_;
#endif
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

#ifdef __have_osg__
    if ( osgview_ ) osgview_->unref();
#endif
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
#ifdef __have_osg__
    osgview_->ref();
    uiOsgViewer::getCommonViewer().addView( this );
#endif
}


uiOsgViewer& uiOsgViewer::getCommonViewer()
{
    static uiOsgViewer viewer;
    return viewer;
}


uiOsgViewer::uiOsgViewer()
#ifdef __have_osg__
    : osgviewer_( new osgViewer::CompositeViewer )
#endif
{
#ifdef __have_osg__
    osgviewer_->ref();
    osgviewer_->setThreadingModel( osgViewer::ViewerBase::SingleThreaded );
    osgQt::setViewer( osgviewer_.get() );
#endif
}


uiOsgViewer::~uiOsgViewer()
{
    for ( int idx=views_.size()-1; idx>=0; idx-- )
    {
	removeView( views_[idx] );
	views_[idx]->setViewer( 0 );
    }

#ifdef __have_osg__
    osgviewer_->unref();
#endif
}


void uiOsgViewer::addView(uiOsgViewHandle* view )
{
#ifdef __have_osg__
    osgviewer_->addView( view->getOsgView() );
#endif
    view->setViewer( this );
    views_ += view;
}


void uiOsgViewer::removeView( uiOsgViewHandle* view )
{
#ifdef __have_osg__
    osgviewer_->removeView( view->getOsgView() );
#endif
    views_ -= view;
}
