#ifndef uimpepartserv_h
#define uimpepartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          December 2004
 RCS:           $Id: uimpepartserv.h,v 1.7 2005-07-28 10:53:49 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "multiid.h"
#include "cubesampling.h"
#include "emposid.h"

class BufferStringSet;

namespace Geometry { class Element; }
namespace MPE { class Wizard; }
namespace Attrib { class DescSet; class SelSpec; class SliceSet; }


/*! \brief Implementation of Tracking part server interface */

class uiMPEPartServer : public uiApplPartServer
{
public:
				uiMPEPartServer(uiApplService&,
						const Attrib::DescSet*);
				~uiMPEPartServer();
    void			setCurrentAttribDescSet(const Attrib::DescSet*);

    const char*			name() const		{ return "MPE";}

    int				getTrackerID(const MultiID&) const;
    int				getTrackerID(const char* name) const;
    void			getTrackerTypes(BufferStringSet&) const;
    int				addTracker(const char* trackertype,
	    				   const char* name);
				/*!<\note Eventual seeds become mine */
    int				addTracker(const MultiID&,const Coord3&);

    MultiID			getTrackerMultiID(int trackerid) const;

    bool			startWizard(const char* tracktype,int startpg);
    bool			canAddSeed(int trackerid) const;
    void			addSeed(int trackerid);
				/*!<\note Eventual seeds become mine */

    void			enableTracking(int trackerid,bool yn);
    bool			isTrackingEnabled(int trackerid) const;

    void			showSetupDlg(const MultiID&,EM::SectionID);
    void			showRelationsDlg(const MultiID&,EM::SectionID);

    int				activeTrackerID() const;
    				/*!< returns the trackerid of the last event */

    static const int		evGetAttribData;
    void			loadAttribData();
    CubeSampling		getActiveVolume() const;
    const Attrib::SelSpec*	getAttribSelSpec() const;
    const Attrib::SliceSet*	getAttribCache(const Attrib::SelSpec&) const;
    void			setAttribData(const Attrib::SelSpec&,
						    Attrib::SliceSet*);
    				/*!<\note Slices become mine */

    static const int		evStartSeedPick;
    static const int		evEndSeedPick;
    void			setSeed(ObjectSet<Geometry::Element>&);
    				/*!<\note Seeds become mine */

    static const int		evAddTreeObject;
    				/*!<Get trackerid via activeTrackerID */
    static const int		evShowToolbar;
    static const int		evInitFromSession;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
    void			createActiveVolume();
    void			updateVolumeFromSeeds();

    const Attrib::DescSet*	attrset;
    MPE::Wizard*		wizard;

				//Interaction variables
    const Attrib::SelSpec*	eventattrselspec;
    int				activetrackerid;

    CubeSampling		csfromseeds;

    friend class		MPE::Wizard;
};


#endif
