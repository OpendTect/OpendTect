#ifndef vispointsetdisplay_h
#define vispointsetdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		March 2009
 RCS:		$Id: vispointsetdisplay.h,v 1.17 2012/06/26 09:49:27 cvssatyaki Exp $
________________________________________________________________________


-*/

#include "dpsdispmgr.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "vistransform.h"

class DataPointSet;

namespace visBase { class PointSet; class Transformation; class EventCatcher; }

namespace visSurvey
{


mClass PointSetDisplay : public visBase::VisualObjectImpl,
			 public visSurvey::SurveyObject
{
public:
    static PointSetDisplay*     create()
				mCreateDataObj(PointSetDisplay);
    				~PointSetDisplay();

    void			setPointSize(int);
    int				getPointSize() const;

    void			setDispProp(const DataPointSetDisplayProp*);
    void			setPointSet();
    bool			hasColor() const 	{ return true; }

    void			update();
    bool			setDataPack(int);
    const DataPointSet*		getDataPack() const 	{ return data_; }
    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    const char*			errMsg() const { return errmsg_.str(); }

    virtual void		setSceneEventCatcher(visBase::EventCatcher*);
    void			removeSelection(const Selector<Coord3>&,
	    					TaskRunner* tr=0);
    bool			selectable() const		{ return true; }
    bool			canRemoveSelection() const	{ return true; }
    bool			allowMaterialEdit() const	{ return true; }

protected:

    DataPointSetDisplayProp*	dpsdispprop_;
    visBase::PointSet*		pointset_;
    DataPointSet*		data_;
    const mVisTrans*		transformation_;
    visBase::EventCatcher*	eventcatcher_;
};

};


#endif
