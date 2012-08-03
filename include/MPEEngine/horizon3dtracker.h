#ifndef horizon3dtracker_h
#define horizon3dtracker_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: horizon3dtracker.h,v 1.10 2012-08-03 13:00:30 cvskris Exp $
________________________________________________________________________


-*/

#include "mpeenginemod.h"
#include "emtracker.h"
#include "emposid.h"

namespace EM { class Horizon3D; };

namespace MPE
{

class Horizon3DSeedPicker;

/*!\brief Horizon tracker
*/

mClass(MPEEngine) Horizon3DTracker : public EMTracker
{
public:
    				Horizon3DTracker(EM::Horizon3D* =0);
     
    static EMTracker*		create(EM::EMObject* =0);
    static void			initClass();

    bool			trackIntersections(const TrackPlane&);

    EMSeedPicker*		getSeedPicker(bool createifnotpresent=true);

    static const char*		keyword();

protected:

    				~Horizon3DTracker();
    EM::Horizon3D*		getHorizon();
    const EM::Horizon3D*	getHorizon() const;

    SectionTracker*		createSectionTracker(EM::SectionID);
    Horizon3DSeedPicker*	seedpicker;
};

} // namespace MPE

#endif

