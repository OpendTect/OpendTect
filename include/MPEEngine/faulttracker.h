#ifndef faulttracker_h
#define faulttracker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2005
 RCS:           $Id: faulttracker.h,v 1.2 2008-10-01 03:44:37 cvsnanne Exp $
________________________________________________________________________

-*/

#include "emtracker.h"
#include "sectionadjuster.h"
#include "sectionextender.h"

namespace EM { class Fault3D; };

namespace MPE
{

class FaultSeedPicker;

class FaultTracker : public EMTracker
{
public:
			FaultTracker(EM::Fault3D* =0);
			~FaultTracker();

    static EMTracker*	create(EM::EMObject* =0);
    static void		initClass();

    bool		trackIntersections(const TrackPlane&);
    EMSeedPicker*	getSeedPicker(bool createifnotpresent);

protected:
    EM::Fault3D*	getFault();
    const EM::Fault3D*	getFault() const;
    FaultSeedPicker*	seedpicker;

    SectionTracker*	createSectionTracker(EM::SectionID);
};


} // namespace MPE

#endif

