#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		March 2009
________________________________________________________________________


-*/

#include "vissurveymod.h"
#include "dpsdispmgr.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "vistransform.h"

class DataPointSet;
class Executor;

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
    const DataPointSetDisplayProp* dispProp() const	{ return dpsdispprop_; }
    bool			hasColor() const override { return true; }

    void			update(TaskRunner*);
    Executor*			getUpdater();
    void			updateColors();
    bool			setDataPack(DataPack::ID);
    const DataPointSet*		getDataPack() const	{ return data_; }
    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;
    const visBase::PointSet*	getPointSet() const	{ return pointset_; }

    const char*			errMsg() const override { return errmsg_.str();}

    bool			removeSelections(TaskRunner*) override;
    bool			selectable() const override	{ return true; }
    bool			canRemoveSelection() const override
				{ return true; }
    bool			allowMaterialEdit() const override
				{ return false;}
    void			setPixelDensity(float) override;

    void			getMousePosInfo(const visBase::EventInfo&,
					    Coord3& xyzpos,
					    BufferString& val,
					    BufferString& info) const override;
    void			getMousePosInfo(const visBase::EventInfo& ei,
						IOPar& iop ) const override
				{ return SurveyObject::getMousePosInfo(ei,iop);}

protected:
				~PointSetDisplay();
    DataPointSetDisplayProp*	dpsdispprop_;
    visBase::PointSet*		pointset_;
    DataPointSet*		data_;
    const mVisTrans*		transformation_;

};

};


