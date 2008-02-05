#ifndef visfaultdisplay_h
#define visfaultdisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visfaultdisplay.h,v 1.1 2008-02-05 22:09:11 cvskris Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"


namespace Geometry
{ class ExplFaultStickSurface; }

namespace visBase
{
    class GeomIndexedShape;
    class Transformation;
};


namespace EM { class Fault; }


namespace visSurvey
{
class MPEEditor;

/*!\brief Used for displaying welltracks, markers and logs


*/

class FaultDisplay : public visBase::VisualObjectImpl,
		     public visSurvey::SurveyObject
{
public:
    static FaultDisplay*	create()
				mCreateDataObj(FaultDisplay);

    MultiID			getMultiID() const;
    bool			isInlCrl() const	{ return false; }

    bool			hasColor() const	{ return true; }
    Color			getColor() const;
    void			setColor(Color);
    bool			allowMaterialEdit() const { return true; }
    NotifierAccess*		materialChange();

    void			setDisplayTransformation(mVisTrans*);
    mVisTrans*			getDisplayTransformation();
    void			setRightHandSystem(bool);

    void			setSceneEventCatcher(visBase::EventCatcher*);

    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

    bool			setEMID(const EM::ObjectID&);
    EM::ObjectID		getEMID() const;

protected:

    static const char*		sKeyEarthModelID()	{ return "EM ID"; }

    virtual			~FaultDisplay();
    void			pickCB(CallBacker*);

    visBase::GeomIndexedShape*		displaysurface_;
    visBase::EventCatcher*		eventcatcher_;

    Geometry::ExplFaultStickSurface*	explicitsurface_;
    EM::Fault*				emfault_;
    visSurvey::MPEEditor*		editor_;
    visBase::Transformation*		displaytransform_;
};

};


#endif
