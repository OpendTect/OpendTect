#ifndef vishingline_h
#define vishingline_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          4-11-2002
 RCS:           $Id: vishingeline.h,v 1.1 2004-04-28 09:59:31 nanne Exp $
________________________________________________________________________

          
-*/

#include "visobject.h"

namespace visBase { class PolyLine; class DrawStyle; class Transformation; };
namespace EM { class HingeLine; };

namespace visSurvey
{

class HingeLineDisplay : public visBase::VisualObjectImpl
{
public:
    static HingeLineDisplay*	create()
				mCreateDataObj(HingeLineDisplay);

    void			setHingeLine(const EM::HingeLine*);
    bool			setHingeLine(int);
    void			setTransformation(visBase::Transformation*);
    visBase::Transformation*	getTransformation();

protected:
    				~HingeLineDisplay();
    void			updateHingeLineChangeCB(CallBacker*);
    const EM::HingeLine*	hingeline;
    ObjectSet<visBase::PolyLine> polylines;
    visBase::Transformation*	transformation;
    visBase::DrawStyle*		drawstyle;
};


}; // namespace visSurvey

#endif
