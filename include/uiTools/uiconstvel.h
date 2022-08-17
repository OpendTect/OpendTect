#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck
 Date:		January 2017
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
