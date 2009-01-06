#ifndef horizon3dtracker_h
#define horizon3dtracker_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: horizon3dtracker.h,v 1.6 2009-01-06 10:48:18 cvsranojay Exp $
________________________________________________________________________


-*/

#include "emtracker.h"
#include "emposid.h"

namespace EM { class Horizon3D; };

namespace MPE
{

class Horizon3DSeedPicker;

/*!\brief Horizon tracker
*/

mClass Horizon3DTracker : public EMTracker
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
    Horizon3DSeedPicker*	seedpicker;
};

} // namespace MPE

#endif
