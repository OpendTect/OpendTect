#ifndef horizon2dselector_h
#define horizon2dselector_h

/*+
________________________________________________________________________
 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 RCS:           $Id: horizon2dselector.h,v 1.1 2006-05-01 15:13:06 cvskris Exp $
________________________________________________________________________
*/

#include "sectionselector.h"
#include "trackplane.h"

namespace EM { class Horizon2D; };

namespace MPE
{

class Horizon2DSelector : public SectionSourceSelector
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
