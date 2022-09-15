#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/


#include "uiwellmod.h"
#include "uidialog.h"

class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiLabeledListBox;
class uiRadioButton;
class uiToolButton;
class uiWellMan;
class uiWellPartServer;
class uiWellSel;
namespace Coords { class uiCoordSystemSel; }
namespace Well { class Data; }

/*!<This export facility provides one location to export Well Track,
checkShot Models, D2T Model and Markers from well export menu in ASCII format*/


mExpClass(uiWell) uiWellExportFacility : public uiDialog
{ mODTextTranslationClass(uiWellExportFacility)
public:
			    uiWellExportFacility(uiParent*,
						    uiWellPartServer&);
			    ~uiWellExportFacility();

    inline static uiString	sWellTrack() { return tr("Well Track"); }
    inline static uiString	sLogs()      { return uiStrings::sLogs(); }
    inline static uiString	sCheckShot() { return tr("CheckShot Data"); }
    inline static uiString	sD2TModel()  { return tr("Depth-Time Model"); }
    inline static uiString	sMarkerUIKey()
					{ return uiStrings::sMarker(mPlural); }

protected :
    uiWellSel*			wellselfld_;
    uiGenInput*			ztypefld_;
    uiGenInput*			traveltymfld_;
    uiLabeledListBox*		loglist_;
    uiGenInput*			basenmfld_;
    uiFileInput*		outfilefld_;
    uiToolButton*		wellmanbut_;
    uiToolButton*		refreshbut_;
    uiGenInput*			zrangefld_;
    uiGroup*			loggrp_;

    uiCheckBox*			welltrack_	= nullptr;
    uiCheckBox*			checkshotdata_	= nullptr;
    uiCheckBox*			d2tmodel_	= nullptr;
    uiCheckBox*			markers_	= nullptr;
    uiCheckBox*			selobjbox_	= nullptr;
    uiWellMan*			wellman_	= nullptr;

    uiWellPartServer&		wellpartserver_;

    void			inputChngCB(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			updateDataCB(CallBacker*);
    void			updateTravelTimeFld(CallBacker*);
    void			selChngCB(CallBacker*);
    void			updateSelButCB(CallBacker*);
    void			refreshCB(CallBacker*);

    bool			exportWellTrack(const char*);
    bool			exportWellLogs(const char*);
    bool			exportD2TModel(const char*,bool ischksht);
    bool			exportMarkers(const char*);

    void			writeLogs(od_ostream&);
    void			writeLogHeader(od_ostream&);
    void			setDefaultRange(bool zinft);

    od_ostream*			getOutputStream(const char* fp,
						const BufferString& type);
    RefMan<Well::Data>		wd_ = nullptr;

    static uiString		strmErrMsg()
				{ return tr("Location is not writable"); }
};
