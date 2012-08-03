#ifndef horizon2dselector_h
#define horizon2dselector_h

/*+
________________________________________________________________________
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 RCS:           $Id: horizon2dselector.h,v 1.4 2012-08-03 13:00:30 cvskris Exp $
________________________________________________________________________
*/

#include "mpeenginemod.h"
#include "sectionselector.h"
#include "trackplane.h"

namespace EM { class Horizon2D; };

namespace MPE
{

mClass(MPEEngine) Horizon2DSelector : public SectionSourceSelector
{
public:
    				Horizon2DSelector(const EM::Horizon2D&,
						  const EM::SectionID&);
    void			setTrackPlane(const TrackPlane&);
    int				nextStep();

protected:
    const EM::Horizon2D&	horizon_;
    TrackPlane			trackplane_;
};

};


#endif

