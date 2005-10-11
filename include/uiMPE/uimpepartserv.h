#ifndef uimpepartserv_h
#define uimpepartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          December 2004
 RCS:           $Id: uimpepartserv.h,v 1.18 2005-10-11 19:58:43 cvskris Exp $
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

    int				getTrackerID(const EM::ObjectID&) const;
    int				getTrackerID(const char* name) const;
    void			getTrackerTypes(BufferStringSet&) const;
    bool			addTracker( const char* trackertype); 
    int				addTracker( const EM::ObjectID&,
	    				    const Coord3& pos );
    				/*!<Creates a new tracker for the object and
				    returns the trackerid of it or -1 if it
				    failed.
				    \param pos should contain the clicked
				           position. If the activevolume is not
					   set before, it will be centered
					   pos, otherwise, it will be expanded
					   to include pos. */
    EM::ObjectID		getEMObjectID(int trackerid) const;

    bool			startWizard(const char* tracktype,int startpg);
    bool			canAddSeed(int trackerid) const;
    void			addSeed(int trackerid);

    void			enableTracking(int trackerid,bool yn);
    bool			isTrackingEnabled(int trackerid) const;

    bool			showSetupDlg( const EM::ObjectID&,
	    				      const EM::SectionID&,
					      bool showcancelbutton=false );
    				/*!<\returns false if cancel was pressed. */

    void			showRelationsDlg(const EM::ObjectID&,
	    					 EM::SectionID);

    int				activeTrackerID() const;
    				/*!< returns the trackerid of the last event */

    static const int		evGetAttribData;
    CubeSampling		getActiveVolume() const;
    const Attrib::SelSpec*	getAttribSelSpec() const;
    const Attrib::SliceSet*	getAttribCache(const Attrib::SelSpec&) const;
    void			setAttribData(const Attrib::SelSpec&,
						    Attrib::SliceSet*);
    				/*!<\note Slices become mine */

    static const int		evStartSeedPick;
    static const int		evEndSeedPick;

    static const int		evAddTreeObject;
    				/*!<Get trackerid via activeTrackerID */
    static const int		evRemoveTreeObject;
    				/*!<Get trackerid via activeTrackerID */
    static const int		evShowToolbar;
    static const int		evWizardClosed;
    static const int		evInitFromSession;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
    bool			activeVolumeIsDefault() const;
    void			expandActiveVolume(const CubeSampling&);
    void			activeVolumeChange(CallBacker*);
    void			loadAttribData();
    void			loadEMObjectCB(CallBacker*);

    const Attrib::DescSet*	attrset;
    MPE::Wizard*		wizard;
    bool			blockdataloading;
    				/*!<Is checked when cb is issued from the
				    MPE::Engine about changed active volume */

				//Interaction variables
    const Attrib::SelSpec*	eventattrselspec;
    int				activetrackerid;

    friend class		MPE::Wizard;
};


#endif
