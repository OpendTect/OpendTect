#ifndef horizon2dtracker_h
#define horizon2dtracker_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: horizon2dtracker.h,v 1.8 2012-08-03 13:00:30 cvskris Exp $
________________________________________________________________________


-*/

#include "mpeenginemod.h"
#include "emtracker.h"
#include "emposid.h"

namespace EM { class Horizon2D; };

namespace MPE
{

class Horizon2DSeedPicker;

mClass(MPEEngine) Horizon2DTracker : public EMTracker
{
public:
    			Horizon2DTracker(EM::Horizon2D* =0);

    static EMTracker*	create(EM::EMObject* =0);
    static void		initClass();

    bool		is2D() const				{ return true; }		
    EMSeedPicker*	getSeedPicker(bool createifnotpresent=true);

    static const char*	keyword();

protected:

    			~Horizon2DTracker();
    EM::Horizon2D*	getHorizon2D();
    const EM::Horizon2D*getHorizon2D() const;

    SectionTracker*		createSectionTracker(EM::SectionID);
    Horizon2DSeedPicker*	seedpicker_;
};

}; // Namespace MPE

#endif

