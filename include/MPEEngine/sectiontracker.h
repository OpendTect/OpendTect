#ifndef sectiontracker_h
#define sectiontracker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: sectiontracker.h,v 1.1 2005-01-06 09:25:55 kristofer Exp $
________________________________________________________________________

-*/

#include "basictask.h"
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
    				SectionTracker( SectionSourceSelector* = 0,
						SectionExtender* = 0,
						SectionAdjuster*  = 0);
    virtual			~SectionTracker();
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

protected:
    BufferString		errmsg;

    SectionSourceSelector*	selector_;
    SectionExtender*		extender_;
    SectionAdjuster*		adjuster_;
};

};

#endif

