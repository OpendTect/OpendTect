#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "uigeninput.h"

namespace Vel
{

mGlobal(uiTools) float getGUIDefaultVelocity();
			//!< If survey display unit is feet, it returns 8000
			//!< otherwise 2000. Its purpose is to get nice values
			//!< of velocity when initializing velocity fields

} // namespace Vel



mExpClass(uiTools) uiConstantVel : public uiGenInput
{ mODTextTranslationClass(uiConstantVel);
public:
			uiConstantVel(uiParent*,
				   float defvel=Vel::getGUIDefaultVelocity(),
				   const uiString& lbl=uiString::emptyString());

			//!< Internal velocities are survey independant (SI)
    void		setInternalVelocity(float);
    float		getInternalVelocity() const;

};
