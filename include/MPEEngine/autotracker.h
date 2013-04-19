#ifndef autotracker_h
#define autotracker_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 RCS:           $Id$
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "emposid.h"
#include "executor.h"
#include "sets.h"
#include "sortedtable.h"
#include "cubesampling.h"

namespace EM { class EMObject; };
namespace Attrib { class SelSpec; }
namespace Geometry { class Element; }
template <class T> class Array2D;

/*!\brief %MPE stands for Model, Predict, Edit. Contains tracking and editing functions.*/

namespace MPE
{

class SectionTracker;
class SectionAdjuster;
class SectionExtender;
class EMTracker;

/*!
\brief Executor to auto track.
*/

mExpClass(MPEEngine) AutoTracker : public Executor
{
public:
				AutoTracker(EMTracker&,const EM::SectionID&);
				~AutoTracker();
    void			setNewSeeds(const TypeSet<EM::PosID>&);
    int				nextStep();
    void			setTrackBoundary(const CubeSampling&);
    void			unsetTrackBoundary();
    od_int64			nrDone() const		{ return nrdone_; }
    od_int64			totalNr() const		{ return totalnr_; }

    virtual const char*		message() const;

protected:
    bool			addSeed(const EM::PosID&);
    void			manageCBbuffer(bool block);
    void			reCalculateTotalNr();
    int				nrdone_;
    int				totalnr_;
    int				nrflushes_;
    int				flushcntr_;
    int 			stepcntallowedvar_;
    int				stepcntapmtthesld_;

    bool			trackingextriffail_;
    bool			burstalertactive_;

    const EM::SectionID		sectionid_;
    SortedTable<EM::SubID,char>	blacklist_;
    TypeSet<EM::SubID>		currentseeds_;
    EM::EMObject&		emobject_;
    SectionTracker*		sectiontracker_;
    SectionExtender*		extender_;
    SectionAdjuster*		adjuster_;
    Geometry::Element*          geomelem_;

    Array2D<float>*		horizon3dundoinfo_;
    RowCol			horizon3dundoorigin_;

    BufferString		execmsg_;
};


}; // namespace MPE

#endif


