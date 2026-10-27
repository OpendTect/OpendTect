#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 dGBVisSurvey is the VSG counterpart of visSurvey.

-*/

#include "vsgsurveymod.h"

#include "visvsgtransform.h"

namespace Survey { class Geometry3D; }

namespace dGBVisSurvey
{

/*!\brief The display transformations of visSurvey::SceneTransformManager,
  computed for dGBVis::Transformation. */

mExpClass(vsgSurvey) SceneTransformManager
{
public:

    static void		computeUTM2DisplayTransform(const Survey::Geometry3D&,
				    float zfactor,float zmidpt,
				    mDGBVisTrans* res);
			//!<Given to all objects in XY-space

    static void		computeICRotationTransform(const Survey::Geometry3D&,
						   float zfactor,float zmidpt,
						   mDGBVisTrans* rotation,
						   mDGBVisTrans* disptrans);
};

} // namespace dGBVisSurvey
