#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "datapack.h"
#include "uidialog.h"

class uiMapperRangeEditor;
class uiPushButton;
class uiStatsDisplay;

namespace ColTab { class MapperSetup; class Sequence; }

mExpClass(uiTools) uiMultiMapperRangeEditWin : public uiDialog
{ mODTextTranslationClass(uiMultiMapperRangeEditWin);
public:
					uiMultiMapperRangeEditWin(uiParent*,
								  int nr);
					~uiMultiMapperRangeEditWin();

    uiMapperRangeEditor*		getuiMapperRangeEditor(int);
    void				setDataPack(int nr,const DataPack*,
						    int version=0);
    mDeprecated("Use setDataPack")
    void				setDataPackID(int nr,const DataPackID&,
						      const DataPackMgr::MgrID&,
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

private:

    mClass(uiTools) MapperData : public CallBacker
    {
    public:
				MapperData();
				~MapperData();

	void			setDataPack(const DataPack*);
	bool			hasDataPack() const	{ return datapack_; }
	ConstRefMan<DataPack>	getDataPack() const;

	uiStatsDisplay*		statsdisplay_	= nullptr;
	uiMapperRangeEditor*	mapperrgeditor_ = nullptr;

    private:
	WeakPtr<DataPack>	datapack_;

	void			dataPackDeleted(CallBacker*);
    };

    ObjectSet<MapperData>		mapperdatas_;

    uiPushButton*			statbut_;
    int					activeattrbid_		= -1;
    const ColTab::MapperSetup*		activectbmapper_	= nullptr;
    const ColTab::Sequence*		activectbseq_		= nullptr;

    void				mouseMoveCB(CallBacker*);
    void				rangeChanged(CallBacker*);
    void				sequenceChanged(CallBacker*);
    void				showStatDlg(CallBacker*);
};
