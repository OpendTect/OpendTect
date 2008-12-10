#ifndef uiwellpartserv_h
#define uiwellpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellpartserv.h,v 1.25 2008-12-10 10:05:18 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "welllog.h"
#include "welldata.h"

class MultiID;
class Coord3;
class Coord;
class Color;
template <class T> class Interval;
class uiWell2RandomLineDlg;

/*! \brief Part Server for Wells */

class uiWellPartServer : public uiApplPartServer
{
public:
				uiWellPartServer(uiApplService&);
				~uiWellPartServer();

    const char*			name() const		{ return "Wells"; }

    				// Services
    bool			importTrack();
    bool			importLogs();
    bool			importMarkers();

    void			manageWells();
    bool			selectWells(ObjectSet<MultiID>&);

    bool			editDisplayProperties(const MultiID&);
    bool			selectLogs(const MultiID&, 
						Well::LogDisplayParSet*&);
    bool			hasLogs(const MultiID&) const;

    bool			dispLineOnCreation(){ return disponcreation_; }
    const char*			getRandLineMultiID()   { return multiid_; }
    void			selectWellCoordsForRdmLine();
    void			getRdmLineCoordinates(TypeSet<Coord>&);
    void			sendPreviewEvent();
    void			rdmlnDlgClosed(CallBacker*);
    Notifier<uiWellPartServer>	randLineDlgClosed;
    void			setPreviewIds( const TypeSet<int>& ids )
				{ previewids_ = ids; }
    TypeSet<int>&		getPreviewIds() 	{ return previewids_; }
    
    void			createWellFromPicks();
    const char*			askWellName();
    bool			setupNewWell(BufferString&, Color&);

    bool			storeWell(const TypeSet<Coord3>&,const char* nm,
	    				  MultiID&);

    void			setSceneID( int id )	{ cursceneid_ = id; }
    int				getSceneID() const	{ return cursceneid_; }

    static const int            evPreviewRdmLine;
    static const int		evCleanPreview;
    
protected:

    uiWell2RandomLineDlg*	rdmlinedlg_;
    TypeSet<int>		previewids_;

    int				cursceneid_;
    bool			disponcreation_;
    const char*			multiid_;
    bool			allapplied_;

    void			saveWellDispProps(const Well::Data*);
    void			saveWellDispProps(const Well::Data&,const MultiID&);
    void			applyAll(CallBacker*);
};

/*!\mainpage Well User Interface

  Apart from nice visualisation import and management of well data must be
  done. The uiWellPartServer delivers the services needed.

*/


#endif
