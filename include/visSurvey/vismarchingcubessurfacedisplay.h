#ifndef vismarchingcubessurfacedisplay_h
#define vismarchingcubessurfacedisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismarchingcubessurfacedisplay.h,v 1.1 2007-09-04 21:26:17 cvskris Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

namespace visBase
{
    class MarchingCubesSurface;
    class Transformation;
    class BoxDragger;
};


namespace EM { class MarchingCubesSurface; }


namespace visSurvey
{
class Scene;

/*!\brief Used for displaying welltracks, markers and logs


*/

class MarchingCubesDisplay : public visBase::VisualObjectImpl,
			     public visSurvey::SurveyObject
{
public:
    static MarchingCubesDisplay*create()
				mCreateDataObj(MarchingCubesDisplay);

    MultiID			getMultiID() const;
    bool			isInlCrl() const	{ return true; }

    bool			hasColor() const	{ return true; }
    Color			getColor() const;
    void			setColor(Color);
    bool			allowMaterialEdit() const { return true; }

    void			setDisplayTransformation(mVisTrans*);
    mVisTrans*			getDisplayTransformation();

    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

    bool			setVisSurface(visBase::MarchingCubesSurface*);
    				//!<Creates an EMObject for it.
    bool			setEMID(const EM::ObjectID&);

    EM::ObjectID		getEMID() const;

protected:

    static const char*		sKeyEarthModelID()	{ return "EM ID"; }

    virtual			~MarchingCubesDisplay();

    visBase::BoxDragger*		initialdragger_;
    visBase::MarchingCubesSurface*	displaysurface_;
    EM::MarchingCubesSurface*		emsurface_;
};

};


#endif
