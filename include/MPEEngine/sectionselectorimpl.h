#ifndef sectionselectorimpl_h
#define sectionselectorimpl_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: sectionselectorimpl.h,v 1.6 2008-02-20 21:07:08 cvskris Exp $
________________________________________________________________________

-*/

#include "sectionselector.h"

#include "position.h"
#include "sets.h"


namespace EM { class Horizon3D; }


namespace MPE
{

class BinIDSurfaceSourceSelector : public SectionSourceSelector
{
public:
    			BinIDSurfaceSourceSelector(const EM::Horizon3D&,
			       			   const EM::SectionID&);
    void		setTrackPlane(const MPE::TrackPlane&);
    int			nextStep() { return 0; }

protected:
    const EM::Horizon3D&	surface_;
};



class SurfaceSourceSelector : public SectionSourceSelector
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

