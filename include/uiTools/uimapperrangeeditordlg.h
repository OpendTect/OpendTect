#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Dec 2008
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"

#include "datapack.h"

class uiPushButton;
class uiMapperRangeEditor;
class uiStatsDisplay;

namespace ColTab { class Mapper; class Sequence; }

mExpClass(uiTools) uiMultiMapperRangeEditWin : public uiDialog
{ mODTextTranslationClass(uiMultiMapperRangeEditWin)
public:
					uiMultiMapperRangeEditWin(uiParent*,
						int nr,DataPackMgr::ID dmid);
					~uiMultiMapperRangeEditWin();

    uiMapperRangeEditor*		getuiMapperRangeEditor(int);
    void				setDataPackID(int visid,
	    				       DataPack::ID,int version);
    void				setColTabMapper(int,
							const ColTab::Mapper&);
    void				setColTabSeq(int,
						const ColTab::Sequence&);
    void				setActiveAttribID(int id);
    int					activeAttrbID()
					{ return activeattrbid_; }
    const ColTab::Mapper&		activeMapper()
					{ return *activectbmapper_; }
    Notifier<uiMultiMapperRangeEditWin>	rangeChange;

protected:

    uiPushButton*			statbut_;
    ObjectSet<uiMapperRangeEditor>	mapperrgeditors_;
    ObjectSet<uiStatsDisplay>		statsdisplays_;
    int					activeattrbid_;
    const ColTab::Mapper*		activectbmapper_;
    DataPackMgr&			dpm_;
    TypeSet<DataPack::ID>		datapackids_;

    void				mouseMoveCB(CallBacker*);
    void				rangeChanged(CallBacker*);
    void				showStatDlg(CallBacker*);
    void				dataPackDeleted(CallBacker*);
};
