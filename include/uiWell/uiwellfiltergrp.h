#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"

#include "mnemonics.h"
#include "odcommonenums.h"
#include "welldata.h"
#include "uidialog.h"
#include "uigroup.h"

class uiComboBox;
class uiListBox;
class uiListBoxFilter;
class uiTableView;
class uiToolButton;
class uiWellTableModel;


mExpClass(uiWell) uiWellFilterGrp : public uiGroup
{ mODTextTranslationClass(uiWellFilterGrp)
public:
				uiWellFilterGrp(uiParent*,
					OD::Orientation orient=OD::Horizontal);
				uiWellFilterGrp(uiParent*,
					const ObjectSet<Well::Data>&,
					const BufferStringSet& lognms,
					const BufferStringSet& markernms,
					OD::Orientation orient=OD::Horizontal);
				uiWellFilterGrp(uiParent*,
					const ObjectSet<Well::Data>&,
					const MnemonicSelection& mns,
					const BufferStringSet& markernms,
					OD::Orientation orient=OD::Horizontal);
				~uiWellFilterGrp();

    void			setFilterItems(const ObjectSet<Well::Data>&,
					       const BufferStringSet& lognms,
					       const BufferStringSet& mrkrnms);
    void			setFilterItems(const ObjectSet<Well::Data>&,
					       const MnemonicSelection& mns,
					       const BufferStringSet& mrkrnms);

    void			setSelected(const DBKeySet& wellids,
					    const BufferStringSet& logs,
					    const BufferStringSet& mrkrs);
    void			setSelected(const BufferStringSet& wells,
					    const BufferStringSet& logs,
					    const BufferStringSet& mrkrs);
    void			setSelected(const BufferStringSet& wells,
					    const MnemonicSelection& mns,
					    const BufferStringSet& mrkrs);
    void			setSelected(const DBKeySet& wellids,
					    const MnemonicSelection& mns,
					    const BufferStringSet& mrkrs);
    void			setMnemonicsSensitive(const MnemonicSelection&);

    void			getSelected(DBKeySet& wellids,
					    BufferStringSet& logs,
					    BufferStringSet& mrkrs) const;
    void			getSelected(BufferStringSet& wells,
					    BufferStringSet& logs,
					    BufferStringSet& mrkrs) const;
    void			getSelected(BufferStringSet& wells,
					    MnemonicSelection& mns,
					    BufferStringSet& mrkrs) const;
    void			getSelected(DBKeySet& wellids,
					    MnemonicSelection& mns,
					    BufferStringSet& mrkrs) const;

    BufferStringSet		getSelectedMarkers() const;

    void			noLogFilterCB(CallBacker*);
    void			mnemFilterCB(CallBacker*);
    void			wellTypeFilter(OD::WellType);
    void			markerZoneFilter(const BufferString& topmrkrnm,
					     const BufferString& botmrkrnm);
    void			depthRangeFilter(const Interval<float> depthrg);
    void			logValRangeFilter(const MnemonicSelection& mns,
				    const TypeSet<Interval<float>>& logvalrg);
    bool			isLogMode() const { return initdesc_.logmode_; }
    void			setLogMode(bool yn=true);

    Notifier<uiWellFilterGrp>	markerSelectionChg;

protected:


    void			initGrp(CallBacker*);
    void			selButPush(CallBacker*);
    void			selChgCB(CallBacker*);
    void			markerSelChgCB(CallBacker*);
    void			fromSelTypeChgdCB(CallBacker*);
    void			fillListBoxes();
    void			fillInitSelection(const BufferStringSet& wllnms,
						  const BufferStringSet& lognms,
						  const BufferStringSet& mrkrs);
    void			fillInitSelection(const BufferStringSet& wllnms,
						  const MnemonicSelection& mns,
						  const BufferStringSet& mrkrs);
    void			setSelection(const BufferStringSet& wellnms,
					     const BufferStringSet& lognms,
					     const BufferStringSet& mrkrnms);
    void			setSelection(const BufferStringSet& wellnms,
					     const MnemonicSelection& mns,
					     const BufferStringSet& mrkrnms);

    void			setMaxLinesForLists();

    mStruct(uiWell) InitDesc
    {
	BufferStringSet			selwellnms_;
	BufferStringSet			sellognms_;
	BufferStringSet			selmrkrnms_;
	MnemonicSelection		selmns_;
	bool				logmode_ = true;
	const ObjectSet<Well::Data>*	wds_ = nullptr;
	MnemonicSelection		mns_;
	BufferStringSet			lognms_;
	BufferStringSet			markernms_;
    };

    uiGroup*			maingrp_;
    uiGroup*			optionsgrp_;
    uiListBox*			welllist_;
    uiListBox*			logormnslist_;
    uiListBoxFilter*		logormnsfilter_;
    uiListBox*			markerlist_;
    uiListBoxFilter*		markerfilter_;

    uiToolButton*		fromwellbut_;
    uiToolButton*		fromlogormnsbut_;
    uiToolButton*		frommarkerbut_;
    uiComboBox*			wellseloptionscb_		= nullptr;
    uiComboBox*			seloptionscb_			= nullptr;
    OD::Orientation		orient_;
    InitDesc			initdesc_;


};
