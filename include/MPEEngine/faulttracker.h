#ifndef faulttracker_h
#define faulttracker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2005
 RCS:           $Id: faulttracker.h,v 1.1 2005-12-12 17:52:19 cvskris Exp $
________________________________________________________________________

-*/

#include "emtracker.h"
#include "sectionadjuster.h"
#include "sectionextender.h"

namespace EM { class Fault; };

namespace MPE
{

class FaultSeedPicker;

class FaultTracker : public EMTracker
{
public:
			FaultTracker(EM::Fault* =0);
			~FaultTracker();

    static EMTracker*	create(EM::EMObject* =0);
    static void		initClass();

    bool		trackIntersections(const TrackPlane&);
    EMSeedPicker*	getSeedPicker(bool createifnotpresent);

protected:
    EM::Fault*		getFault();
    const EM::Fault*	getFault() const;
    FaultSeedPicker*	seedpicker;

    SectionTracker*	createSectionTracker(EM::SectionID);
};


}; // namespace MPE

#endif

