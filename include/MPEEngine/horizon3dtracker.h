#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "mpeenginemod.h"
#include "emtracker.h"
#include "emposid.h"

namespace EM { class Horizon3D; }

namespace MPE
{

class Horizon3DSeedPicker;

/*!
\brief EMTracker to track EM::Horizon3D.
*/

mExpClass(MPEEngine) Horizon3DTracker : public EMTracker
{
public:
    				Horizon3DTracker(EM::Horizon3D* =0);

    static EMTracker*		create(EM::EMObject* =0);
    static void			initClass();

    EMSeedPicker*		getSeedPicker(
					bool createifnotpresent=true) override;

    static const char*		keyword();

protected:

    				~Horizon3DTracker();
    EM::Horizon3D*		getHorizon();
    const EM::Horizon3D*	getHorizon() const;

    SectionTracker*		createSectionTracker() override;
    Horizon3DSeedPicker*	seedpicker_;
};

} // namespace MPE

