/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiosgviewer.cc,v 1.1 2011-12-08 16:26:24 cvskris Exp $";

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
#endif

class uiOsgViewer
{
public:
			uiOsgViewer();
			~uiOsgViewer();

    void                addView(uiOsgViewBase*); //Make sure it's kept alive
    void                removeView(uiOsgViewBase*);

    static uiOsgViewer& getCommonViewer();

protected:
#ifdef __have_osg__
    osg::ref_ptr<osgViewer::CompositeViewer>	osgviewer_;
#endif
    ObjectSet<uiOsgViewBase>     		views_;
};


uiOsgViewBase::uiOsgViewBase()
    : osgview_( 0 )
{}


void uiOsgViewBase::detachView()
{
    if ( viewer_ )
	viewer_->removeView( this );
    viewer_ = 0;

#ifdef __have_osg__
    if ( osgview_ ) osgview_->unref();
#endif
    osgview_ = 0;
}


void uiOsgViewBase::setOsgView( osgViewer::View* view )
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


void uiOsgViewer::addView(uiOsgViewBase* view )
{
#ifdef __have_osg__
    osgviewer_->addView( view->getOsgView() );
#endif
    view->setViewer( this );
    views_ += view;
}


void uiOsgViewer::removeView( uiOsgViewBase* view )
{
#ifdef __have_osg__
    osgviewer_->removeView( view->getOsgView() );
#endif
    views_ -= view;
}
