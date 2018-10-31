#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2003
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uiapplserv.h"
#include "notify.h"
#include "bufstringset.h"
#include "dbkey.h"
#include "uistring.h"

namespace Well { class Data; class LogDisplayParSet; }

class DBKeySet;
class uiBulkD2TModelImport;
class uiBulkLogImport;
class uiBulkMarkerImport;
class uiBulkTrackImport;
class uiD2TMLogSelDlg;
class uiD2TModelGen;
class uiSimpleMultiWellCreate;
class uiWell2RandomLineDlg;
class uiWellDispPropDlg;
class uiWellImportAsc;
class uiWellMan;

/*!
\ingroup uiWell
\brief Part Server for Wells
*/

mExpClass(uiWell) uiWellPartServer : public uiApplPartServer
{ mODTextTranslationClass(uiWellPartServer);
public:
				uiWellPartServer(uiApplService&);
				~uiWellPartServer();

    const char*			name() const		{ return "Wells"; }

				// Services
    void			importTrack();
    void			importLogs();
    void			importMarkers();
    void			bulkImportTrack();
    void			bulkImportLogs();
    void			bulkImportMarkers();
    void			bulkImportD2TModel();

    void			manageWells();
    void			launchRockPhysics();
    bool			selectWells(DBKeySet&);

    bool			hasLogs(const DBKey&) const;
    void			getLogNames(const DBKey&,
					    BufferStringSet&) const;

    bool			editDisplayProperties(const DBKey&);
    void			displayIn2DViewer(const DBKey&);

    bool			dispLineOnCreation(){ return disponcreation_; }
    DBKey			getRandLineDBKey()   { return dbkey_; }
    void			selectWellCoordsForRdmLine();
    void			getRdmLineCoordinates(TypeSet<Coord>&);
    void			sendPreviewEvent();
    void			closePropDlg(const DBKey&);
    Notifier<uiWellPartServer>	randLineDlgClosed;
    Notifier<uiWellPartServer>	uiwellpropDlgClosed;
    void			setPreviewIds( const TypeSet<int>& ids )
				{ previewids_ = ids; }
    TypeSet<int>&		getPreviewIds()	{ return previewids_; }

    void			createSimpleWells()	{ simpImp(0); }
    const DBKeySet&		createdWellIDs()	{ return crwellids_; }

    void			doLogTools();

    void			createWellFromPicks();
    const char*			askWellName();
    bool			setupNewWell(BufferString&,Color&);
    bool			storeWell(const TypeSet<Coord3>&,const char* nm,
					  DBKey&,bool addwellhead=false);

    static int		        evPreviewRdmLine();
    static int			evCleanPreview();
    static int			evDisplayWell();

protected:

    uiWellImportAsc*		uiwellimpdlg_;
    uiWell2RandomLineDlg*	rdmlinedlg_;
    ObjectSet<uiWellDispPropDlg> wellpropdlgs_;
    uiD2TModelGen*		uid2tmgen_;
    uiD2TMLogSelDlg*		uid2tmlogseldlg_;
    uiSimpleMultiWellCreate*	impsimpledlg_;
    uiBulkTrackImport*		impbulktrackdlg_;
    uiBulkLogImport*		impbulklogdlg_;
    uiBulkMarkerImport*		impbulkmrkrdlg_;
    uiBulkD2TModelImport*	impbulkd2tdlg_;
    uiWellMan*			manwelldlg_;
    TypeSet<int>		previewids_;

    DBKeySet			crwellids_; // for uiSimpleMultiWellCreate

    bool			disponcreation_;
    DBKey			dbkey_;
    bool			allapplied_;

    void			importReadyCB(CallBacker*);
    void			rdmlnDlgDeleted(CallBacker*);
    void			rdmlnDlgClosed(CallBacker*);
    void			wellPropDlgClosed(CallBacker*);
    void			saveWellDispProps(const Well::Data*);
    void			applyAll(CallBacker*);
    void			simpImp(CallBacker*);
    void			simpleImpDlgClosed(CallBacker*);
    void			survChangedCB(CallBacker*);
    int				getPropDlgIndex(const DBKey&);

};
