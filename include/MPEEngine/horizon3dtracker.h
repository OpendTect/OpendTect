#ifndef horizontracker_h
#define horizontracker_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: horizon3dtracker.h,v 1.1 2005-12-12 17:52:19 cvskris Exp $
________________________________________________________________________


-*/

#include "emtracker.h"
#include "emposid.h"

namespace EM { class Horizon; };

namespace MPE
{

class ConsistencyChecker;
class HorizonSeedPicker;

/*!\brief Horizon tracker
*/

class HorizonTracker : public EMTracker
{
public:
    			HorizonTracker(EM::Horizon* =0);
			~HorizonTracker();
     
    static EMTracker*	create(EM::EMObject* =0);
    static void		initClass();

    bool		trackIntersections(const TrackPlane&);

    EMSeedPicker*	getSeedPicker(bool createifnotpresent=true);

protected:

    EM::Horizon*	getHorizon();
    const EM::Horizon*	getHorizon() const;

    SectionTracker*	createSectionTracker(EM::SectionID);
    ConsistencyChecker* getConsistencyChecker();
    
    void 		interpolateSeeds(const TypeSet<EM::PosID>*,
    					 EM::SectionID );

    ConsistencyChecker*	consistencychecker;
    HorizonSeedPicker*	seedpicker;
};

}; // Namespace MPE

#endif
