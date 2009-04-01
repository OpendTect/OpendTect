#ifndef vispointsetdisplay_h
#define vispointsetdisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		March 2009
 RCS:		$Id: vispointsetdisplay.h,v 1.1 2009-04-01 07:00:24 cvssatyaki Exp $
________________________________________________________________________


-*/

#include "color.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "vistransform.h"

class DataPointSet;

namespace visBase { class IndexedPointSet; }

namespace visSurvey
{


mClass PointSetDisplay : public visBase::VisualObjectImpl,
			 public visSurvey::SurveyObject
{
public:
    static PointSetDisplay*     create()
				mCreateDataObj(PointSetDisplay);
    				~PointSetDisplay();

    Color			getColor() const	{ return color_; }
    void			setColor( Color col )	{ color_ = col; }

    bool			setDataPackID(int,DataPack::ID);
    void			setDisplayTransformation(mVisTrans*);
    mVisTrans*			getDisplayTransformation();
protected:

    Color			color_;
    visBase::IndexedPointSet*	pointset_;
};

};


#endif
