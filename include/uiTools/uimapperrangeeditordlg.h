#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"

#include "datapack.h"

class uiPushButton;
class uiMapperRangeEditor;
class uiStatsDisplay;

namespace ColTab { class MapperSetup; class Sequence; }

mExpClass(uiTools) uiMultiMapperRangeEditWin : public uiDialog
{ mODTextTranslationClass(uiMultiMapperRangeEditWin);
public:
					uiMultiMapperRangeEditWin(uiParent*,
						int nr,DataPackMgr::MgrID dmid);
					~uiMultiMapperRangeEditWin();

    uiMapperRangeEditor*		getuiMapperRangeEditor(int);
    void				setDataPackID(int nr,DataPackID,
						      int version=0);
    void				setColTabMapperSetup(int,
						const ColTab::MapperSetup&);
    void				setColTabSeq(int,
						const ColTab::Sequence&);
    void				setActiveAttribID(int id);
    int					activeAttrbID()
					{ return activeattrbid_; }
    const ColTab::MapperSetup&		activeMapperSetup()
					{ return *activectbmapper_; }
    const ColTab::Sequence&		activeSequence()
					{ return *activectbseq_; }

    Notifier<uiMultiMapperRangeEditWin>	rangeChange;
    Notifier<uiMultiMapperRangeEditWin>	sequenceChange;

protected:

    uiPushButton*			statbut_;
    ObjectSet<uiMapperRangeEditor>	mapperrgeditors_;
    int					activeattrbid_;
    const ColTab::MapperSetup*		activectbmapper_;
    const ColTab::Sequence*		activectbseq_;
    DataPackMgr&			dpm_;
    TypeSet<DataPackID>		datapackids_;
    ObjectSet<uiStatsDisplay>		statsdisplays_;

    void				mouseMoveCB(CallBacker*);
    void				rangeChanged(CallBacker*);
    void				sequenceChanged(CallBacker*);
    void				showStatDlg(CallBacker*);
    void				dataPackDeleted(CallBacker*);
};
