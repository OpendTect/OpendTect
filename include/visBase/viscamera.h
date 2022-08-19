#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"

#include "color.h"
#include "position.h"
#include "visdata.h"

namespace osg { class Camera; class RenderInfo; }

namespace visBase
{

class DrawCallback;

/*!\brief 
    keep osg camera status and render info
*/

mExpClass(visBase) Camera : public DataObject
{
public:

    static Camera*	create()
			mCreateDataObj( Camera );

    osg::Camera*	osgCamera() const;
    OD::Color		getBackgroundColor() const;
    Coord3		getTranslation() const;
    Coord3		getScale() const;
    void		getRotation(Coord3& vec,double& angle)const;
    void		getLookAtMatrix(Coord3&,Coord3&,Coord3&)const;

    void		setBackgroundColor(const OD::Color&);

    Notifier<Camera>		preDraw;
    Notifier<Camera>		postDraw;

    const osg::RenderInfo*	getRenderInfo() const { return renderinfo_; }
				//!<Only available during pre/post draw cb

private:
    friend			class DrawCallback;

    void			triggerDrawCallBack(const DrawCallback*,
						    const osg::RenderInfo&);

    virtual			~Camera();

    osg::Camera*		camera_;
    const osg::RenderInfo*	renderinfo_;
    DrawCallback*		predraw_;
    DrawCallback*		postdraw_;

};

} // namespace visBase
