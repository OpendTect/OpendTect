#ifndef emsurfaceedgelineimpl_h
#define emsurfaceedgelineimpl_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurfaceedgelineimpl.h,v 1.12 2008-02-20 21:39:16 cvskris Exp $
________________________________________________________________________


-*/

#include "emsurfaceedgeline.h"

namespace EM
{


class TerminationEdgeLineSegment : public EdgeLineSegment
{
public:
		    mEdgeLineSegmentClone(TerminationEdgeLineSegment, TermLine);
		    TerminationEdgeLineSegment( EM::Horizon3D& surf,
						const EM::SectionID& sect )
			: EdgeLineSegment( surf, sect ) {}

    virtual bool    shouldHorizonTrack(int,const RowCol&) const
    			{ return false; }
};


class SurfaceConnectLine : public EdgeLineSegment
{
public:
    			mEdgeLineSegmentClone(SurfaceConnectLine,ConnLine);
			SurfaceConnectLine( EM::Horizon3D& surf,
					    const EM::SectionID& sect )
			    : EdgeLineSegment( surf, sect ) {}

    virtual bool	shouldHorizonTrack(int,const RowCol&) const 
    			{ return false; }
    int		    	reTrackOrderIndex() const { return 1; }
    bool		isNodeOK(const RowCol&) const;

    void		setConnectingSection(const EM::SectionID& ns )
			{ connectingsection = ns; }
    EM::SectionID	connectingSection() const { return connectingsection; }

    void		fillPar(IOPar& par) const;
    bool		usePar( const IOPar&);

protected:
    static const char*	connectingsectionstr;
    EM::SectionID	connectingsection;
    bool		internalIdenticalSettings(
	    				const EM::EdgeLineSegment&) const;

};


};

#endif
