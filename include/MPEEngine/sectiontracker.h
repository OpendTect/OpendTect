#ifndef sectiontracker_h
#define sectiontracker_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: sectiontracker.h,v 1.15 2009/07/22 16:01:16 cvsbert Exp $
________________________________________________________________________

-*/

#include "task.h"
#include "cubesampling.h"
#include "emposid.h"
#include "geomelement.h"

class BinIDValue;

namespace Attrib { class SelSpec; }
namespace EM { class EMObject; }

namespace MPE
{

class TrackPlane;


class SectionSourceSelector;
class SectionExtender;
class SectionAdjuster;


mClass SectionTracker
{
public:
    				SectionTracker( EM::EMObject&,
						const EM::SectionID&,
						SectionSourceSelector* = 0,
					        SectionExtender* = 0,
					        SectionAdjuster* = 0);
    virtual			~SectionTracker();
    EM::SectionID		sectionID() const;
    virtual bool		init();

    void			reset();

    bool			trackWithPlane( const TrackPlane& plane );

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

    void			useAdjuster(bool yn);
    bool			adjusterUsed() const;

    void			setSetupID(const MultiID& id);
    const MultiID&		setupID() const;
    bool			hasInitializedSetup() const;

    void			setSeedOnlyPropagation(bool yn);
    bool			propagatingFromSeedOnly() const;

    const Attrib::SelSpec&	getDisplaySpec() const;
    void			setDisplaySpec(const Attrib::SelSpec&);

    void			getNeededAttribs(
	    			    ObjectSet<const Attrib::SelSpec>&) const;
    virtual CubeSampling	getAttribCube(const Attrib::SelSpec&) const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    void			removeUnSupported(TypeSet<EM::SubID>&) const;
protected:

    bool			erasePositions(
	    				const TypeSet<EM::SubID>& selectedpos,
					const TypeSet<EM::SubID>& excludedpos,
	    				bool addtohistory) const;
    void			getLockedSeeds(TypeSet<EM::SubID>& lockedseeds);

    EM::EMObject&		emobject;
    EM::SectionID		sid;

    BufferString		errmsg;
    bool			useadjuster;
    MultiID			setupid;
    Attrib::SelSpec&		displayas;
    bool			seedonlypropagation;

    SectionSourceSelector*	selector_;
    SectionExtender*		extender_;
    SectionAdjuster*		adjuster_;

    

    static const char*		trackerstr;
    static const char*		useadjusterstr;
    static const char*		seedonlypropstr;
};

};

#endif

