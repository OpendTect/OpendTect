#ifndef horizontracker_h
#define horizontracker_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: horizon3dtracker.h,v 1.3 2007-05-22 03:23:22 cvsnanne Exp $
________________________________________________________________________


-*/

#include "emtracker.h"
#include "emposid.h"

namespace EM { class Horizon3D; };

namespace MPE
{

class ConsistencyChecker;
class Horizon3DSeedPicker;

/*!\brief Horizon tracker
*/

class Horizon3DTracker : public EMTracker
{
public:
    				Horizon3DTracker(EM::Horizon3D* =0);
				~Horizon3DTracker();
     
    static EMTracker*		create(EM::EMObject* =0);
    static void			initClass();

    bool			trackIntersections(const TrackPlane&);

    EMSeedPicker*		getSeedPicker(bool createifnotpresent=true);

protected:

    EM::Horizon3D*		getHorizon();
    const EM::Horizon3D*	getHorizon() const;

    SectionTracker*		createSectionTracker(EM::SectionID);
    ConsistencyChecker*		getConsistencyChecker();
    
    ConsistencyChecker*		consistencychecker;
    Horizon3DSeedPicker*	seedpicker;
};

} // namespace MPE

#endif
