#ifndef emtracker_h
#define emtracker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 RCS:           $Id: emtracker.h,v 1.5 2005-03-17 14:52:34 cvsnanne Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include "emposid.h"


namespace Geometry { class Element; };
namespace EM { class EMObject; };

namespace MPE
{

class SectionTracker;
class TrackPlane;

class EMTracker
{
public:
    				EMTracker( EM::EMObject* );
    virtual			~EMTracker();

    const char*			objectName() const;
    EM::ObjectID		objectID() const;

    virtual bool		isEnabled() const	{ return isenabled; }
    virtual void		enable(bool yn)		{ isenabled=yn; }
    virtual bool		setSeeds(const ObjectSet<Geometry::Element>&,
					 const char* name) = 0;
    virtual bool		trackSections(const TrackPlane&);
    virtual bool		trackIntersections(const TrackPlane&);

    SectionTracker*		getSectionTracker(EM::SectionID,
	    					  bool create=false);

    const char*			errMsg() const;

protected:
    virtual SectionTracker*	createSectionTracker(EM::SectionID) = 0;
    bool			isenabled;
    ObjectSet<SectionTracker>	sectiontrackers;
    BufferString		errmsg;
    EM::EMObject*		emobject;
};


typedef EMTracker*(*EMTrackerCreationFunc)(EM::EMObject*);


class TrackerFactory
{
public:
			TrackerFactory(const char* emtype,
				       EMTrackerCreationFunc);
    const char*		emObjectType() const;
    EMTracker*		create(EM::EMObject* emobj=0) const;

protected:
    EMTrackerCreationFunc	createfunc;
    const char*			type;

};

}; // namespace MPE

#endif

