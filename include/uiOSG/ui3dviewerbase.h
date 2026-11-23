#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiosgmod.h"

#include "color.h"
#include "uiobj.h"

class FontData;

mExpClass(uiOSG) ui3DViewerBase : public uiObject
{
public:
			~ui3DViewerBase();

    virtual SceneID	sceneID() const					= 0;

    virtual void	setViewMode(bool)				= 0;
    virtual bool	isViewMode() const				= 0;

    virtual void	showRotAxis(bool)				= 0;
    virtual bool	rotAxisShown() const				= 0;

    virtual void	setAnnotationColor(const OD::Color&)		= 0;
    virtual OD::Color	getAnnotationColor() const			= 0;
    virtual void	setAnnotationFont(const FontData&)		= 0;

    virtual OD::Color	getBackgroundColor() const			= 0;
			//!<setBackgroundColor() is virtual in uiObject

protected:
			ui3DViewerBase(uiParent*,const char* nm,
				       uiObjectBody&);
};
