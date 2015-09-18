#ifndef vispointsetdisplay_h
#define vispointsetdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		March 2009
 RCS:		$Id$
________________________________________________________________________


-*/

#include "vissurveymod.h"
#include "dpsdispmgr.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "vistransform.h"

class DataPointSet;

namespace visBase { class PointSet; class Transformation; }

namespace visSurvey
{


mExpClass(visSurvey) PointSetDisplay : public visBase::VisualObjectImpl,
			 public visSurvey::SurveyObject
{
public:
				PointSetDisplay();
				mDefaultFactoryInstantiation(
				    visSurvey::SurveyObject,PointSetDisplay,
				    "PointSetDisplay",
				    toUiString(sFactoryKeyword()) );

    void			setPointSize(int);
    int				getPointSize() const;

    void			setDispProp(const DataPointSetDisplayProp*);
    bool			hasColor() const	{ return true; }

    void			update(TaskRunner*);
    bool			setDataPack(int);
    const DataPointSet*		getDataPack() const	{ return data_; }
    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    const char*			errMsg() const { return errmsg_.str(); }

    void			removeSelection(const Selector<Coord3>&,
						TaskRunner* tr=0);
    bool			selectable() const		{ return true; }
    bool			canRemoveSelection() const	{ return true; }
    bool			allowMaterialEdit() const	{ return true; }
    virtual void		setPixelDensity(float);

    virtual void		getMousePosInfo(const visBase::EventInfo&,
					    Coord3& xyzpos,
					    BufferString& val,
					    BufferString& info) const;
    void			getMousePosInfo(const visBase::EventInfo& ei,
						IOPar& iop ) const
				{ return SurveyObject::getMousePosInfo(ei,iop);}

protected:
				~PointSetDisplay();
    DataPointSetDisplayProp*	dpsdispprop_;
    visBase::PointSet*		pointset_;
    DataPointSet*		data_;
    const mVisTrans*		transformation_;
};

};


#endif

