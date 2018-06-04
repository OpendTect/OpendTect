#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		March 2009
________________________________________________________________________


-*/

#include "vissurveycommon.h"
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
				    SurveyObject,PointSetDisplay,
				    "PointSetDisplay",
				    toUiString(sFactoryKeyword()) );
    virtual const char*		getClassName() const
				{ return sFactoryKeyword(); }

    void			setPointSize(int);
    int				getPointSize() const;

    void			setDispProp(const DataPointSetDisplayProp*);
    bool			hasColor() const	{ return true; }

    void			update(TaskRunner*);
    Executor*			getUpdater();
    void			updateColors();
    bool			setDataPack(DataPack::ID);
    const DataPointSet*		getDataPack() const	{ return data_; }
    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;
    const visBase::PointSet*	getPointSet() const	{ return pointset_; }

    const uiString&		errMsg() const { return errmsg_; }

    bool			removeSelections(TaskRunner*);
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
    RefMan<DataPointSet>	data_;
    const mVisTrans*		transformation_;

};

};
