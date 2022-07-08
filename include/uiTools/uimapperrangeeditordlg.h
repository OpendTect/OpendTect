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

namespace ColTab { class MapperSetup; class Sequence; }

mExpClass(uiTools) uiMultiMapperRangeEditWin : public uiDialog
{ mODTextTranslationClass(uiMultiMapperRangeEditWin);
public:
					uiMultiMapperRangeEditWin(uiParent*,
						int nr,DataPackMgr::MgrID dmid);
					~uiMultiMapperRangeEditWin();

    uiMapperRangeEditor*		getuiMapperRangeEditor(int);
    void				setDataPackID(int nr,DataPack::ID,
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
    TypeSet<DataPack::ID>		datapackids_;
    ObjectSet<uiStatsDisplay>		statsdisplays_;

    void				mouseMoveCB(CallBacker*);
    void				rangeChanged(CallBacker*);
    void				sequenceChanged(CallBacker*);
    void				showStatDlg(CallBacker*);
    void				dataPackDeleted(CallBacker*);
};

