#ifndef uimpepartserv_h
#define uimpepartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          December 2004
 RCS:           $Id: uimpepartserv.h,v 1.2 2005-03-11 16:58:27 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "multiid.h"
#include "cubesampling.h"
#include "emposid.h"

class AttribDescSet;
class AttribSelSpec;
class AttribSliceSet;

namespace Geometry { class Element; }
class BufferStringSet;
//class Color;
//class CubeSampling;
//class IOObj;
//class IOPar;
//class LineStyle;
//class MarkerStyle3D;
//class uiPopupMenu;

//template <class T> class Array3D;

//namespace EM { class EdgeLineSegment; class Surface; };
namespace MPE
{ 
    class SurfaceConnectLine;
    class Tracker;
    class TrackManager;
    class Wizard;
};


/*! \brief Implementation of Tracking part server interface */

class uiMPEPartServer : public uiApplPartServer
{
public:
				uiMPEPartServer(uiApplService&,
						const AttribDescSet*);
				~uiMPEPartServer();
    void			setCurrentAttribDescSet(const AttribDescSet*);

    const char*			name() const		{ return "MPE";}

    int				getTrackerID(const MultiID&) const;
    int				getTrackerID(const char* name) const;
    void			getTrackerTypes(BufferStringSet&) const;
    int				addTracker(const char* trackertype,
	    				   const char* name);
				/*!<\note Eventual seeds become mine */
    int				addTracker(const MultiID&);

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
    const AttribSelSpec*	getAttribSelSpec() const;
    CubeSampling		getAttribCube(const AttribSelSpec&) const;
    const AttribSliceSet*	getAttribCache(const AttribSelSpec&) const;
    void			setAttribData(const AttribSelSpec&,
	    				      AttribSliceSet*);
    				/*!<\note Slices become mine */

    static const int		evStartSeedPick;
    static const int		evEndSeedPick;
    void			setSeed(ObjectSet<Geometry::Element>&);
    				/*!<\note Seeds become mine */

    static const int		evAddTreeObject;
    				/*!<Get trackerid via activeTrackerID */

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
    void			loadAttribData();

    const AttribDescSet*	attrset;
    MPE::Wizard*		wizard;

				//Interaction variables
    const AttribSelSpec*	eventattrselspec;
    int				activetrackerid;

    friend class		MPE::Wizard;
};


#endif
