#ifndef sectiontracker_h
#define sectiontracker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: sectiontracker.h,v 1.4 2005-03-15 16:53:29 cvsnanne Exp $
________________________________________________________________________

-*/

#include "basictask.h"
#include "emposid.h"
#include "geomelement.h"

class BinIDValue;


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

    void			useAdjuster(bool yn)	{ useadjuster=yn; }
    bool			adjusterUsed() const	{ return useadjuster; }

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
    BufferString		errmsg;
    bool			useadjuster;

    SectionSourceSelector*	selector_;
    SectionExtender*		extender_;
    SectionAdjuster*		adjuster_;

    static const char*		trackerstr;
    static const char*		useadjusterstr;
};

};

#endif

