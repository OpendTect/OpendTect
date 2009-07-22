#ifndef vispointsetdisplay_h
#define vispointsetdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		March 2009
 RCS:		$Id: vispointsetdisplay.h,v 1.5 2009-07-22 16:01:25 cvsbert Exp $
________________________________________________________________________


-*/

#include "color.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "vistransform.h"

class DataPointSet;

namespace visBase { class PointSet; }

namespace visSurvey
{


mClass PointSetDisplay : public visBase::VisualObjectImpl,
			 public visSurvey::SurveyObject
{
public:
    static PointSetDisplay*     create()
				mCreateDataObj(PointSetDisplay);
    				~PointSetDisplay();

    Color			getColor() const;
    void			setColor(Color);
    bool			hasColor() const 	{ return true; }

    bool			setDataPack(const DataPointSet&);
    const DataPointSet&		getDataPack() const 	{ return data_; }
    void			setDisplayTransformation(mVisTrans*);
    mVisTrans*			getDisplayTransformation();
    bool			allowMaterialEdit() const	{ return true; }
protected:

    Color			color_;
    visBase::PointSet*		pointset_;
    DataPointSet&		data_;
};

};


#endif
