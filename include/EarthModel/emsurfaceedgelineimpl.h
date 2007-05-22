#ifndef emsurfaceedgelineimpl_h
#define emsurfaceedgelineimpl_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurfaceedgelineimpl.h,v 1.11 2007-05-22 03:23:22 cvsnanne Exp $
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


class SurfaceCutLine : public EdgeLineSegment
{
public:
    			mEdgeLineSegmentClone(SurfaceCutLine,CutLine);
			SurfaceCutLine( EM::Horizon3D&, const EM::SectionID& );
    int		    	reTrackOrderIndex() const { return 2; }

    bool		canTrack() const { return cuttinghorizon_; }
    const EM::Horizon3D* cuttingHorizon() const { return cuttinghorizon_; }
    virtual bool	shouldHorizonTrack(int,const RowCol& trackdir) const;
    virtual bool	shouldHorizonExpand() const { return true; }

    static float	getMeshDist();

    bool		trackWithCache( int, bool, const EdgeLineSegment*,
					const EdgeLineSegment* );

    virtual void	setTime2Depth( const FloatMathFunction* f) { t2d=f; }
    void		setCuttingSurface( const EM::Horizon3D* cs, bool pos )
			{ cuttinghorizon_ = cs; cutonpositiveside = pos; }

    bool		isNodeOK(const RowCol&) const;

    static
    SurfaceCutLine*	createCutFromEdges( EM::Horizon3D& surface,
					    const EM::SectionID& section,
					    int relidx,
					    const FloatMathFunction* t2d );
    			/*!< Creates a cutline from a surface relation.
			     \variable relidx	Surface relation that
			     			will be tracked.
			*/

    static
    SurfaceCutLine*	createCutFromSeed( EM::Horizon3D& surface,
					   const EM::SectionID& section,
					   int relidx, const RowCol& seed,
					   bool boothdirs,
					   const FloatMathFunction* t2d );
    			/*!< Creates a cutline from a surface relation.
			     \variable relidx	Surface relation that
			     			will be tracked.
			*/
    static void		computeDistancesAlongLine( const EM::EdgeLine&,
					       const EM::Horizon3D&,
					       const  FloatMathFunction* t2d,
					       TypeSet<RowCol>&,
					       TypeSet<float>&,
					       bool negate,
					       bool usecaching );

    void		fillPar(IOPar& par) const;
    bool		usePar( const IOPar&);

protected:
    bool		internalIdenticalSettings(
	    				const EM::EdgeLineSegment&) const;
    void		removeCache();
    void		commitChanges();
    bool		isAtCuttingEdge(int idx) const;

    float		computeScore( const RowCol&,
	    			      bool& changescorepos,
				      Coord3& scorepos, const RowCol* source);

    bool		getCuttingPositions( const Coord&,
	    				     TypeSet<EM::PosID>& );
    
    TypeSet<RowCol>	cacherc;
    TypeSet<Coord3>	poscache;
    TypeSet<float>	distcache;
    BoolTypeSet		ischanged;
    const float		meshdist;

    const FloatMathFunction*	t2d;
    const EM::Horizon3D* cuttinghorizon_;
    bool		cutonpositiveside;

    static const char*	cuttingobjectstr;
    static const char*	possidestr;

};


};

#endif
