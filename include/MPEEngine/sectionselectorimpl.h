#ifndef sectionselectorimpl_h
#define sectionselectorimpl_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: sectionselectorimpl.h,v 1.2 2005-01-18 12:57:10 kristofer Exp $
________________________________________________________________________

-*/

#include "sectionselector.h"

#include "position.h"
#include "sets.h"


namespace EM
{
    class Horizon;
    class HorizontalTube;
}


namespace MPE
{

class BinIDSurfaceSourceSelector : public SectionSourceSelector
{
public:
    			BinIDSurfaceSourceSelector( const EM::Horizon&,
			       			    const EM::SectionID& );
    void		setTrackPlane( const MPE::TrackPlane& );
    int			nextStep() { return 0; }

protected:
    const EM::Horizon&	surface;
};



class TubeSurfaceSourceSelector : public SectionSourceSelector
{
public:
    		TubeSurfaceSourceSelector( const EM::EMObject&,
					   const EM::SectionID& );
    void	setTrackPlane( const MPE::TrackPlane& );
    int		nextStep() { return 0; }

protected:
    const EM::EMObject&	emobject;
};




};

#endif

