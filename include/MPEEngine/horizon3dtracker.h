#ifndef horizon3dtracker_h
#define horizon3dtracker_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: horizon3dtracker.h,v 1.8 2010-11-06 16:21:05 cvsumesh Exp $
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
     
    static EMTracker*		create(EM::EMObject* =0);
    static void			initClass();

    bool			trackIntersections(const TrackPlane&);

    EMSeedPicker*		getSeedPicker(bool createifnotpresent=true);

protected:

    				~Horizon3DTracker();
    EM::Horizon3D*		getHorizon();
    const EM::Horizon3D*	getHorizon() const;

    SectionTracker*		createSectionTracker(EM::SectionID);
    Horizon3DSeedPicker*	seedpicker;
};

} // namespace MPE

#endif
