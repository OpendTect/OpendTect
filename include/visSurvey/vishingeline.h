#ifndef vishingline_h
#define vishingline_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          4-11-2002
 RCS:           $Id: vishingeline.h,v 1.3 2004-07-23 13:01:36 kristofer Exp $
________________________________________________________________________

          
-*/

#include "visobject.h"

namespace visBase { class IndexedPolyLine3D; class DrawStyle; class Transformation; };
namespace EM { class EdgeLineSet; };

namespace visSurvey
{

class EdgeLineSetDisplay : public visBase::VisualObjectImpl
{
public:
    static EdgeLineSetDisplay*	create()
				mCreateDataObj(EdgeLineSetDisplay);

    void			setEdgeLineSet(const EM::EdgeLineSet*);
    bool			setEdgeLineSet(int);
    const EM::EdgeLineSet*	getEdgeLineSet() const { return edgelineset; }
    void			setTransformation(visBase::Transformation*);
    visBase::Transformation*	getTransformation();

protected:
    				~EdgeLineSetDisplay();
    void			updateEdgeLineSetChangeCB(CallBacker*);
    const EM::EdgeLineSet*	edgelineset;
    ObjectSet<visBase::IndexedPolyLine3D> polylines;
    visBase::Transformation*	transformation;
    visBase::DrawStyle*		drawstyle;
};


}; // namespace visSurvey

#endif
