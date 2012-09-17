#ifndef faulttracker_h
#define faulttracker_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2005
 RCS:           $Id: faulttracker.h,v 1.5 2010/11/06 16:21:05 cvsumesh Exp $
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

    static EMTracker*	create(EM::EMObject* =0);
    static void		initClass();

    bool		trackIntersections(const TrackPlane&);
    EMSeedPicker*	getSeedPicker(bool createifnotpresent);

protected:

    			~FaultTracker();
    EM::Fault3D*	getFault();
    const EM::Fault3D*	getFault() const;
    FaultSeedPicker*	seedpicker;

    SectionTracker*	createSectionTracker(EM::SectionID);
};


} // namespace MPE

#endif

