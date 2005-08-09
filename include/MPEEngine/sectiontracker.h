#ifndef sectiontracker_h
#define sectiontracker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: sectiontracker.h,v 1.8 2005-08-09 15:50:29 cvskris Exp $
________________________________________________________________________

-*/

#include "basictask.h"
#include "cubesampling.h"
#include "emposid.h"
#include "geomelement.h"

class BinIDValue;

namespace Attrib { class SelSpec; }

namespace MPE
{

class TrackPlane;


class SectionSourceSelector;
class SectionExtender;
class SectionAdjuster;


class SectionTracker
{
public:
    				SectionTracker(SectionSourceSelector* = 0,
					       SectionExtender* = 0,
					       SectionAdjuster* = 0);
    virtual			~SectionTracker();
    EM::SectionID		sectionID() const;
    virtual bool		init();

    void			reset();

    SectionSourceSelector*	selector();
    const SectionSourceSelector* selector() const;
    SectionExtender*		extender();
    const SectionExtender*	extender() const;
    SectionAdjuster*		adjuster();
    const SectionAdjuster*	adjuster() const;

    virtual bool		select();
    virtual bool		extend();
    virtual bool		adjust();
    const char*			errMsg() const;

    void			useAdjuster(bool yn)	{ useadjuster_=yn; }
    bool			adjusterUsed() const	{ return useadjuster_; }

    void			setSetupID(const MultiID& id)	{ setupid_=id; }
    const MultiID&		setupID() const		{ return setupid_; }

    const Attrib::SelSpec&	getDisplaySpec() const;
    void			setDisplaySpec(const Attrib::SelSpec&);

    void			getNeededAttribs(
	    			    ObjectSet<const Attrib::SelSpec>&) const;
    virtual CubeSampling	getAttribCube( const Attrib::SelSpec& ) const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
    BufferString		errmsg_;
    bool			useadjuster_;
    MultiID			setupid_;
    Attrib::SelSpec&		displayas_;

    SectionSourceSelector*	selector_;
    SectionExtender*		extender_;
    SectionAdjuster*		adjuster_;

    static const char*		trackerstr;
    static const char*		useadjusterstr;
};

};

#endif

