#ifndef emsurfaceedgeline_h
#define emsurfaceedgeline_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurfaceedgeline.h,v 1.6 2004-09-03 08:21:49 kristofer Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "callback.h"
#include "refcount.h"

template <class T> class MathFunction;



namespace EM
{

class EdgeLineSegment;
class Surface;
class SurfaceRelation;
typedef EdgeLineSegment*(*EdgeLineCreationFunc)(EM::Surface&, const SectionID&);

/*!\brief

*/

#define mEdgeLineSegmentClone(clss,clssname) \
virtual EdgeLineSegment* clone() const { return new clss(*this); } \
static clss* create( EM::Surface& surf, const EM::SectionID& sect ) \
			{ return new clss( surf, sect ); } \
static const char* sClassName() { return #clssname; } \
virtual const char* className() const { return clss::sClassName(); }

#define mEdgeLineSegmentFactoryEntry(clss ) \
EM::EdgeLineSegmentFactory fact##clss( clss::sClassName(), \
	                (EM::EdgeLineCreationFunc) clss::create )



class EdgeLineSegmentFactory;

class EdgeLineSegment : public CallBackClass
{ 
public:
    				mEdgeLineSegmentClone(EdgeLineSegment,Default);

			EdgeLineSegment( EM::Surface&, const EM::SectionID& );
			EdgeLineSegment( const EdgeLineSegment& );
			~EdgeLineSegment();
    void		setSection(const EM::SectionID& s) { section=s; }
    void		setTime2Depth( const MathFunction<float>* );
    bool		shouldTrack(int) const { return true; }
    bool		shouldExpand() const { return false; }
    			//The two above should be removed/renamed

    bool		haveIdenticalSettings( const EdgeLineSegment& ) const;
    			/*!<\returns true if the segments are of the same
			     	     type and have the same settigs */

    int			size() const;
    int			indexOf( const RowCol&, bool forward=true ) const;
    void		remove(int);
    void		remove(int,int);
    void		removeAll();
    void		insert(int, const RowCol&);
    void		set(int, const RowCol&);
    void		copyNodesFrom( const TypeSet<RowCol>&,
	    			       bool reverse=false );
    void		copyNodesFrom( const EdgeLineSegment*,
	    			       bool reverse=false );
    const RowCol&	operator[](int idx) const;
    const EdgeLineSegment& operator+=(const RowCol&);

    bool		isClosed() const;
    bool		isContinuedBy( const EdgeLineSegment* ) const;
    int			findNeighborTo( const RowCol&, bool forward ) const;
    RowCol		first() const;
    RowCol		last() const;
		
    bool		isByPassed( int idx, const EdgeLineSegment* prev,
				    const EdgeLineSegment* next ) const;

    virtual bool	canTrack() const { return true; }
    virtual bool	reTrack( const EdgeLineSegment* prev,
	    			 const EdgeLineSegment* next );
    virtual bool	track( int idx, bool forward,
	    		       const EdgeLineSegment* prev=0,
	   		       const EdgeLineSegment* next=0 )
			{
			    removeCache();
			    return trackWithCache( idx, forward, prev, next );
			}

    void		reverse();
    			/*!<Changes the order of the nodes */
    void		makeLine( const RowCol& from, const RowCol& to );

    virtual void	fillPar( IOPar& ) const;
    virtual bool	usePar( const IOPar& );

    NotifierAccess*	changeNotifier();
    static EM::EdgeLineSegment*	factory(const IOPar&,EM::Surface& surf,
	    				const EM::SectionID& sect);

    const EM::Surface&	getSurface() const { return surface; }
    EM::Surface&	getSurface() { return surface; }
    EM::SectionID	getSection() const { return section; }

    const TypeSet<RowCol>&	getNodes() const { return nodes; }
    virtual void		commitChanges() {}
    				/*!< During tracking, the line might want to
				     move the surface slightly to get a perfect
				     match between surfaces. These changes are
				     stored since we don't want to change the
				     surface if not neccessary. These changes
				     will be applied to the surface when this
				     function is called. This function is
				     normally only called from
				     EM::EdgeLine::insertSegments.
				*/
protected:
    virtual bool	internalIdenticalSettings(const EdgeLineSegment&) const;
    			/*!<\returns true if the segments are of the same
			     	     type and have the same settigs */

    virtual bool	isNodeOK( const RowCol& );
    virtual bool	trackWithCache( int idx, bool forward,
	    		       const EdgeLineSegment* prev=0,
	   		       const EdgeLineSegment* next=0 );
    virtual void	removeCache() {}

    bool		isDefined( const RowCol& ) const;
    bool		isAtEdge( const RowCol& ) const;
    bool		isConnToNext(int idx) const;
    bool		isConnToPrev(int idx) const;

    bool		getNeighborNode(int idx, bool forward, RowCol&,
	    				const EdgeLineSegment* prev,
	    				const EdgeLineSegment* next ) const;
    bool		getSurfaceStart( int idx,bool clockwise,RowCol& ) const;

    EM::Surface&	surface;
    EM::SectionID	section;

    static const char*	key;


private:
    friend					class EdgeLineSegmentFactory;
    static ObjectSet<EdgeLineSegmentFactory>&	factories();
    static const char*				classnamestr;

    void			posChangeCB(CallBacker*);

    Notifier<EdgeLineSegment>*	notifier;
    TypeSet<RowCol>		nodes;
};


class EdgeLineSegmentFactory 
{
public:
			EdgeLineSegmentFactory( const char* nm,
						EdgeLineCreationFunc f )
			    : name ( nm ), func( f )
			{ EM::EdgeLineSegment::factories() += this; }

    const char*			name;
    EdgeLineCreationFunc	func;
};


class EdgeLine : public CallBackClass
{
public:
    			EdgeLine( EM::Surface&, const EM::SectionID& );
    virtual		~EdgeLine() { deepErase( segments ); }
    EdgeLine*		clone() const;
    void		setSection( const EM::SectionID& );

    int			getSegment( const EM::PosID&, int* segpos=0 ) const;
    int			getSegment( const RowCol&, int* segpos=0 ) const;
    bool		isClosed() const;
    bool		isInside(const EM::PosID&, bool undefval ) const;
    bool		isInside(const RowCol&, bool undefval ) const;
    bool		isHole() const;
    int			computeArea() const;

    int			nrSegments() const { return segments.size(); }
    void		insertSegment( EdgeLineSegment*, int idx,
	    			       bool cutexisting );
    void		insertSegments( ObjectSet<EdgeLineSegment>&, int idx,
	    			       bool cutexisting );
    const EdgeLineSegment* getSegment(int idx) const { return segments[idx]; }
    EdgeLineSegment*	getSegment(int idx) { return segments[idx]; }

    const EM::Surface&	getSurface() const { return surface; }
    EM::SectionID	getSection() const { return section; }

    Notifier<EdgeLine>	changenotifier;	

    void		fillPar( IOPar& ) const;
    bool		usePar( const IOPar& );

protected:
    void			sectionChangeCB(CallBacker*);
    int				findNeighborTo( const RowCol& rc, int startseg,
	    					int startpos, bool forward,
	    					int* segpos ) const;

    void			reduceSegments();
    				/*!<Removes zero-length segments and
				    joins neighbor segments with identical
				    settings */
    EM::Surface&		surface;
    EM::SectionID		section;

    ObjectSet<EdgeLineSegment>	segments;

    static const char*		segmentprefixstr;
    static const char*		nrsegmentsstr;
};


class EdgeLineIterator
{
public:
    			EdgeLineIterator( const EM::EdgeLine& el_,
				bool forward_=true, int startseg_=0,
				int startpos_=0 )
			    : el( el_ )
			    , segment( startseg_ )
			    , nodeidx( startpos_ )
			    , startseg( startseg_ )
			    , startpos( startpos_ )
			    , forward( forward_ )
			    , nrturns( 0 )
			{}

    bool		isOK() const;
    int			nrTurns() const { return nrturns; }
    bool		next();

    const RowCol&	currentRowCol() const
    			{ return (*el.getSegment(segment))[nodeidx]; }
    EM::PosID		current() const;
    int			currentNodeIdx() const { return nodeidx; }
    int			currentSegment() const { return segment; }

protected:
    const EM::EdgeLine&	el;
    int			segment;
    int			nodeidx;
    int			startpos;
    int 		startseg;
    bool		forward;
    int			nrturns;
};


class EdgeLineSet : public CallBackClass
{
public:
    			EdgeLineSet( EM::Surface&, const EM::SectionID&);
    virtual		~EdgeLineSet();

    void		removeAll();

    EdgeLineSet*	clone() const;
    void		setSection(const EM::SectionID&);

    bool		isOnLine( const RowCol&, int* lineidx=0,
	     			  int* segmentidx=0, int* segmentpos=0 ) const;

    int			addLine( EdgeLine* line );

    int			nrLines() const			{ return lines.size(); }
    int			getMainLine() const;
    const EdgeLine*	getLine(int idx) const;
    EdgeLine*		getLine(int idx);

    bool		adaptToSurface();
    bool		findLines(EdgeLineCreationFunc=EdgeLineSegment::create);
    bool		removeAllNodesOutsideLines();

    const EM::Surface&	getSurface() const		{ return surface; }
    EM::Surface&	getSurface()			{ return surface; }
    EM::SectionID	getSection() const		{ return section; }


    Notifier<EdgeLineSet>	changenotifier;

    void		fillPar( IOPar& ) const;
    bool		usePar( const IOPar& );
protected:
    void		changedLineCB(CallBacker*);
    ObjectSet<EdgeLine>	lines;
    EM::Surface&	surface;
    EM::SectionID	section;

    static const char*	nrlinesstr;
    static const char*	lineprefixstr;
};


class EdgeLineManager : public CallBackClass
{
public:
    			EdgeLineManager( Surface& );
    virtual		~EdgeLineManager();
    EdgeLineSet*	getEdgeLineSet( const EM::SectionID&, bool create );
    const EdgeLineSet*	getEdgeLineSet( const EM::SectionID& ) const;
    void		cloneEdgeLineSet( const EM::SectionID& src,
	    				  const EM::SectionID& dst );
    void		removeSection( const SectionID& );
    void		removeLineSet( const SectionID& );
    void		removeAll();

    void		fillPar( IOPar& ) const;
    bool		usePar( const IOPar& );

    CNotifier<EdgeLineManager,SectionID>	addremovenotify;

protected:
    static const char*		sectionkey;
    Surface&			surface;
    ObjectSet<EdgeLineSet>	linesets;
};

};

#endif


