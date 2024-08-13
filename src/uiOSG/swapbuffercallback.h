#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include <osg/GraphicsContext>

#include "visscene.h"
#include "ui3dviewerbody.h"

/*!
\brief Class for a swap callback. This class triggers on the second
  render that the context is up an running, meaning that the scene
  can ask questions about the context.
*/
#ifdef OD_USE_QOPENGL
class SwapCallback : public osg::GraphicsContext::SwapCallback
{
public:
SwapCallback( OD3DViewer* body )
    : scene_( nullptr )
    , nrleft_( 2 )
    , body_( body )
{}


void swapBuffersImplementation( osg::GraphicsContext* gc ) override
{
    gc->swapBuffersImplementation();
    if ( scene_ && nrleft_ )
    {
	nrleft_--;
	if ( !nrleft_ )
	{
	    scene_->contextIsUp.trigger();
	    CallBack::addToMainThread(
		    mCB(body_,OD3DViewer,removeSwapCallback) );
	}
    }
}


    int				nrleft_;
    visBase::Scene*		scene_;
    OD3DViewer*			body_;
};

#else

class SwapCallback : public osg::GraphicsContext::SwapCallback
{
public:
    SwapCallback( ui3DViewerBody* body )
	: scene_( nullptr )
	, nrleft_( 2 )
	, body_( body )
    {}

    void swapBuffersImplementation(osg::GraphicsContext* gc) override
    {
	gc->swapBuffersImplementation();
	if ( scene_ && nrleft_ )
	{
	    nrleft_--;
	    if ( !nrleft_ )
	    {
		scene_->contextIsUp.trigger();
		CallBack::addToMainThread( mCB(body_,ui3DViewerBody,
					       removeSwapCallback));
	    }
	}
    }


    int				nrleft_;
    visBase::Scene*		scene_;
    ui3DViewerBody*		body_;
};

#endif
