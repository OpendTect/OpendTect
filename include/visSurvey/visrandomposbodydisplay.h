#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"

#include "emposid.h"
#include "emrandomposbody.h"
#include "visobject.h"
#include "visrandompos2body.h"
#include "vissurvobj.h"
#include "vistransform.h"

namespace EM { class RandomPosBody; }


namespace visSurvey
{
class Scene;

/*!\brief used for displaying a set of random picks in xyz coordinate.*/

mExpClass(visSurvey) RandomPosBodyDisplay : public visBase::VisualObjectImpl
					  , public SurveyObject
{ mODTextTranslationClass(RandomPosBodyDisplay);
public:
				RandomPosBodyDisplay();

				mDefaultFactoryInstantiation(
				    SurveyObject, RandomPosBodyDisplay,
				    "RandomPosBodyDisplay",
				    ::toUiString(sFactoryKeyword()) )

    MultiID			getMultiID() const override;
    bool			isInlCrl() const override	{ return false;}

    bool			hasColor() const override	{ return true; }
    OD::Color			getColor() const override;
    void			setColor(OD::Color) override;
    bool			allowMaterialEdit() const  override
				{ return true; }

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    bool			setVisBody(visBase::RandomPos2Body*);
				//!<Creates an EMObject for it.
    bool			setEMID(const EM::ObjectID&);
    EM::ObjectID		getEMID() const;
    const EM::RandomPosBody*	getEMBody() const	{ return embody_.ptr();}

    const char*			errMsg() const override { return errmsg_.str();}

protected:
				~RandomPosBodyDisplay();

    bool			updateVisFromEM();
    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    ConstRefMan<mVisTrans>	transform_;
    RefMan<visBase::RandomPos2Body> displaybody_;
    RefMan<EM::RandomPosBody>	embody_;

    static const char*		sKeyPSEarthModelID()	{ return "EM ID"; }
};

} // namespace visSurvey
