#ifndef uiwellpartserv_h
#define uiwellpartserv_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "bufstringset.h"

class Coord3;
class Coord;
class Color;
template <class T> class Interval;
class MultiID;
namespace Well { class Data; class LogDisplayParSet; }

class uiWell2RandomLineDlg;
class uiWellDispPropDlg;
class uiD2TModelGen;
class uiD2TMLogSelDlg;

/*! \brief Part Server for Wells */

mClass uiWellPartServer : public uiApplPartServer
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

    bool			hasLogs(const MultiID&) const;
    void			getLogNames(const MultiID&,
					    BufferStringSet&) const;

    bool			editDisplayProperties(const MultiID&);
    void			displayIn2DViewer(const MultiID&);
    bool			showAmplSpectrum(const MultiID&,const char*);

    bool			dispLineOnCreation(){ return disponcreation_; }
    const char*			getRandLineMultiID()   { return multiid_; }
    void			selectWellCoordsForRdmLine();
    void			getRdmLineCoordinates(TypeSet<Coord>&);
    void			sendPreviewEvent();
    void			rdmlnDlgClosed(CallBacker*);
    void			wellPropDlgClosed(CallBacker*);
    Notifier<uiWellPartServer>	randLineDlgClosed;
    Notifier<uiWellPartServer>	uiwellpropDlgClosed;
    void			setPreviewIds( const TypeSet<int>& ids )
				{ previewids_ = ids; }
    TypeSet<int>&		getPreviewIds() 	{ return previewids_; }

    void			createSimpleWells();
    const BufferStringSet&	createdWellIDs()	{ return crwellids_; }
    
    void			createWellFromPicks();
    const char*			askWellName();
    bool			setupNewWell(BufferString&, Color&);

    bool			storeWell(const TypeSet<Coord3>&,const char* nm,
	    				  MultiID&);

    void			setSceneID( int id )	{ cursceneid_ = id; }
    int				getSceneID() const	{ return cursceneid_; }

    static const int            evPreviewRdmLine();
    static const int		evCleanPreview();
    static const int		evDisplayWell();
    
protected:

    uiWell2RandomLineDlg*	rdmlinedlg_;
    uiWellDispPropDlg*		uiwellpropdlg_;
    uiD2TModelGen*		uid2tmgen_;
    uiD2TMLogSelDlg*		uid2tmlogseldlg_;
    TypeSet<int>		previewids_;

    BufferStringSet		crwellids_; // for uiSimpleMultiWellCreate

    int				cursceneid_;
    bool			disponcreation_;
    const char*			multiid_;
    bool			allapplied_;
    bool			isdisppropopened_;

    void			saveWellDispProps(const Well::Data*);
    void			saveWellDispProps(const Well::Data&,
						  const MultiID&);
    void			applyAll(CallBacker*);
    void			simpImp(CallBacker*);

public:
    void			doLogTools();
};

/*!\mainpage Well User Interface

  Apart from nice visualisation, import and management of well data must be
  done. The uiWellPartServer delivers the services needed.
*/

#endif
