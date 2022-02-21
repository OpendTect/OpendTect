#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2019
________________________________________________________________________

-*/

#include "uiwellmod.h"

#include "oduicommon.h"
#include "welldata.h"
#include "uidialog.h"
#include "uigroup.h"

class MnemonicSelection;
class uiListBox;
class uiListBoxFilter;
class uiStatusBar;
class uiTableView;
class uiToolButton;
class uiWellTableModel;
template <class T> class Array2D;


mExpClass(uiWell) WellDataFilter
{ mODTextTranslationClass(WellDataFilter)
public:
				WellDataFilter(const ObjectSet<Well::Data>&);
				~WellDataFilter();

    void			getWellsFromLogs(
					const BufferStringSet& lognms,
					BufferStringSet& wellnms) const;
    void			getWellsWithNoLogs(
					BufferStringSet& wellnms) const;
    void			getWellsFromMarkers(
					const BufferStringSet& markernms,
					BufferStringSet& wellnms) const;
    void			getMarkersLogsFromWells(
					const BufferStringSet& wellnms,
					BufferStringSet& lognms,
					BufferStringSet& markernms) const;
    void			getLogPresence(
					const BufferStringSet& wellnms,
					const char* topnm,const char* botnm,
					const BufferStringSet& alllognms,
					Array2D<int>& presence,
					BufferStringSet& lognms,
					Well::Info::DepthType depthtype) const;
    void			getLogsForMnems(const MnemonicSelection& mns,
					BufferStringSet& lognms) const;

private:
    const ObjectSet<Well::Data>&	allwds_;
};


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
				~uiWellFilterGrp();

    void			setFilterItems(const ObjectSet<Well::Data>&,
					       const BufferStringSet& lognms,
					       const BufferStringSet& mrkrnms);
    void			setSelected(const BufferStringSet& wells,
					    const BufferStringSet& logs,
					    const BufferStringSet& mrkrs);
    void			getSelected(BufferStringSet& wells,
					    BufferStringSet& logs,
					    BufferStringSet& mrkrs) const;

    void			setStatusBar(uiStatusBar* sb)
				{ statusbar_ = sb; }
    void			noLogFilterCB(CallBacker*);
    void			mnemFilterCB(CallBacker*);

protected:


    void			selButPush(CallBacker*);
    void			selChgCB(CallBacker*);
    void			toStatusBar(const uiString&,int fldidx=0,
					    int msecs=-1);

    uiListBox*			welllist_;
    uiListBox*			loglist_;
    uiListBoxFilter*		logfilter_;
    uiListBox*			markerlist_;
    uiListBoxFilter*		markerfilter_;

    uiToolButton*		fromwellbut_;
    uiToolButton*		fromlogbut_;
    uiToolButton*		frommarkerbut_;

    uiStatusBar*		statusbar_ = nullptr;

    const ObjectSet<Well::Data>* wds_ = nullptr;
};
