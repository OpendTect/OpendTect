#ifndef horizon2dtracker_h
#define horizon2dtracker_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: horizon2dtracker.h,v 1.3 2008-02-20 20:19:33 cvskris Exp $
________________________________________________________________________


-*/

#include "emtracker.h"
#include "emposid.h"

namespace EM { class Horizon2D; };

namespace MPE
{

class Horizon2DSeedPicker;

class Horizon2DTracker : public EMTracker
{
public:
    			Horizon2DTracker(EM::Horizon2D* =0);
			~Horizon2DTracker();

    static EMTracker*	create(EM::EMObject* =0);
    static void		initClass();

    bool		is2D() const				{ return true; }		
    EMSeedPicker*	getSeedPicker(bool createifnotpresent=true);

protected:

    EM::Horizon2D*	getHorizon2D();
    const EM::Horizon2D*getHorizon2D() const;

    SectionTracker*		createSectionTracker(EM::SectionID);
    Horizon2DSeedPicker*	seedpicker_;
};

}; // Namespace MPE

#endif
