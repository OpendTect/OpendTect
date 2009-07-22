#ifndef horizon2dtracker_h
#define horizon2dtracker_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: horizon2dtracker.h,v 1.5 2009-07-22 16:01:16 cvsbert Exp $
________________________________________________________________________


-*/

#include "emtracker.h"
#include "emposid.h"

namespace EM { class Horizon2D; };

namespace MPE
{

class Horizon2DSeedPicker;

mClass Horizon2DTracker : public EMTracker
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
