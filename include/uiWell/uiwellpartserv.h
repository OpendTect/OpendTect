#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uiapplserv.h"
#include "bufstringset.h"
#include "uistring.h"

namespace Well { class Data; class LogDisplayParSet; }

class uiBulkDirectionalImport;
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
class uiWellMgrInfoDlg;

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
    void			bulkImportDirectional();
    void			showWellMgrInfo();

    void			manageWells();
    void			launchRockPhysics();
    bool			selectWells(TypeSet<MultiID>&);

    bool			hasLogs(const MultiID&) const;
    void			getLogNames(const MultiID&,
					    BufferStringSet&) const;

    bool			editDisplayProperties(const MultiID&,OD::Color);
    void			displayIn2DViewer(const MultiID&);

    bool			dispLineOnCreation(){ return disponcreation_; }
    const char*			getRandLineMultiID()   { return multiid_; }
    void			selectWellCoordsForRdmLine();
    void			getRdmLineCoordinates(TypeSet<Coord>&);
    void			sendPreviewEvent();
    Notifier<uiWellPartServer>	randLineDlgClosed;
    Notifier<uiWellPartServer>	uiwellpropDlgClosed;
    void			setPreviewIds( const TypeSet<int>& ids )
				{ previewids_ = ids; }
    TypeSet<int>&		getPreviewIds()	{ return previewids_; }

    void			createSimpleWells()	{ simpImp(0); }
    const BufferStringSet&	createdWellIDs()	{ return crwellids_; }

    void			doLogTools();

    void			createWellFromPicks();
    const char*			askWellName();
    bool			setupNewWell(BufferString&,OD::Color&);
    bool			storeWell(const TypeSet<Coord3>&,const char* nm,
					  MultiID&,bool addwellhead=false);

    static int			evPreviewRdmLine();
    static int			evCleanPreview();
    static int			evDisplayWell();

protected:

    uiWellImportAsc*		uiwellimpdlg_		= nullptr;
    uiWell2RandomLineDlg*	rdmlinedlg_		= nullptr;
    uiD2TModelGen*		uid2tmgen_		= nullptr;
    uiD2TMLogSelDlg*		uid2tmlogseldlg_	= nullptr;
    uiSimpleMultiWellCreate*	impsimpledlg_		= nullptr;

    uiBulkTrackImport*		impbulktrackdlg_	= nullptr;
    uiBulkDirectionalImport*	impbulkdirwelldlg_	= nullptr;
    uiBulkLogImport*		impbulklogdlg_		= nullptr;
    uiBulkMarkerImport*		impbulkmrkrdlg_		= nullptr;
    uiBulkD2TModelImport*	impbulkd2tdlg_		= nullptr;
    uiWellMan*			manwelldlg_		= nullptr;
    uiWellMgrInfoDlg*		wellmgrinfodlg_		= nullptr;
    TypeSet<int>		previewids_;

    BufferStringSet		crwellids_; // for uiSimpleMultiWellCreate

    bool			disponcreation_;
    const char*			multiid_;
    bool			allapplied_;

    void			cleanup();
    void			importReadyCB(CallBacker*);
    void			rdmlnDlgDeleted(CallBacker*);
    void			rdmlnDlgClosed(CallBacker*);
    void			saveWellDispProps(ConstRefMan<Well::Data>&);
    void			saveWellDispProps(ConstRefMan<Well::Data>&,
						  const MultiID&);
    void			applyAll(CallBacker*);
    void			applyTabProps(CallBacker*);
    void			resetAllProps(CallBacker*);
    void			simpImp(CallBacker*);
    void			simpleImpDlgClosed(CallBacker*);
    void			survChangedCB(CallBacker*);

};

