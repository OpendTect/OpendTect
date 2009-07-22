#ifndef faulttracker_h
#define faulttracker_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2005
 RCS:           $Id: faulttracker.h,v 1.4 2009-07-22 16:01:16 cvsbert Exp $
________________________________________________________________________

-*/

#include "emtracker.h"
#include "sectionadjuster.h"
#include "sectionextender.h"

namespace EM { class Fault3D; };

namespace MPE
{

class FaultSeedPicker;

mClass FaultTracker : public EMTracker
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

