/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiod2dviewerbackend.h"

#include "view2ddata.h"


Viewer2DID OD2DViewer::allocateViewerID()
{
    mDefineStaticLocalObject( Threads::Atomic<int>, nextid, (0) );
    Viewer2DID id;
    id.set( nextid++ );
    return id;
}


Vis2DID OD2DViewer::allocateObjectID()
{
    return View2D::DataObject::allocateID();
}


uiOD2DViewerBackend::uiOD2DViewerBackend()
    : objectAdded(this)
    , objectToBeRemoved(this)
    , positionChanged(this)
    , viewerClosed(this)
{}


uiOD2DViewerBackend::~uiOD2DViewerBackend()
{
    detachAllNotifiers();
}


OD2DViewer::OpenResult2DV uiOD2DViewerBackend::openDisplayStack(
				const VisID&,FlatView::Viewer::VwrDest )
{
    OD2DViewer::OpenResult2DV result;
    result.status_ = OD2DViewer::OpenRequestStatus2DV::Unsupported;
    result.message_ = toUiString( "This viewer does not support opening a "
				 "display's full attribute stack." );
    return result;
}
