#ifndef sectionselectorimpl_h
#define sectionselectorimpl_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id$
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "sectionselector.h"

#include "position.h"
#include "sets.h"


namespace EM { class Horizon3D; }


namespace MPE
{

mClass(MPEEngine) BinIDSurfaceSourceSelector : public SectionSourceSelector
{
public:
    			BinIDSurfaceSourceSelector(const EM::Horizon3D&,
			       			   const EM::SectionID&);
    void		setTrackPlane(const MPE::TrackPlane&);
    int			nextStep() { return 0; }

protected:
    const EM::Horizon3D&	surface_;
};



mClass(MPEEngine) SurfaceSourceSelector : public SectionSourceSelector
{
public:
    		SurfaceSourceSelector(const EM::EMObject&,
				      const EM::SectionID&);
    void	setTrackPlane(const MPE::TrackPlane&);
    int		nextStep() { return 0; }

protected:
    const EM::EMObject&	emobject_;
};


};

#endif


