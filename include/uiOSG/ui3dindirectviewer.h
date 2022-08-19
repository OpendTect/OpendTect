#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ui3dviewerbody.h"

class GraphicsWindowIndirect;

/*!
\brief Class used by ui3DViewer to render things indirectly.
*/

mClass(uiOSG) ui3DIndirectViewBody : public ui3DViewerBody
{
public:
				ui3DIndirectViewBody(ui3DViewer&,uiParent*);
				~ui3DIndirectViewBody();

    const QWidget*              qwidget_() const override;

protected:
    osgViewer::GraphicsWindow&	getGraphicsWindow() override;
    osg::GraphicsContext*       getGraphicsContext() override;


    GraphicsWindowIndirect*	graphicswin_;
};
