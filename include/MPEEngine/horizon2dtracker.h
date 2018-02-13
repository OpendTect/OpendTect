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

namespace EM { class Horizon2D; }

namespace MPE
{

class Horizon2DSeedPicker;

/*!
\brief EMTracker to track EM::Horizon2D.
*/

mExpClass(MPEEngine) Horizon2DTracker : public EMTracker
{
public:
				Horizon2DTracker(EM::Horizon2D* =0);

    static EMTracker*		create(EM::Object* =0);
    static void			initClass();

    bool			is2D() const			{ return true; }
    EMSeedPicker*		getSeedPicker(bool createifnotpresent=true);

    static const char*		keyword();

protected:

				~Horizon2DTracker();

    EM::Horizon2D*		getHorizon2D();
    const EM::Horizon2D*	getHorizon2D() const;

    SectionTracker*		createSectionTracker();
    Horizon2DSeedPicker*	seedpicker_;
};

} // namespace MPE
