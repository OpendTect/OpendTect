#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2019
________________________________________________________________________

-*/

#include "uiwellmod.h"

#include "odcommonenums.h"
#include "welldata.h"
#include "uidialog.h"
#include "uigroup.h"

class MnemonicSelection;
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

    void			noLogFilterCB(CallBacker*);
    void			mnemFilterCB(CallBacker*);

protected:


    void			selButPush(CallBacker*);
    void			selChgCB(CallBacker*);

    uiListBox*			welllist_;
    uiListBox*			loglist_;
    uiListBoxFilter*		logfilter_;
    uiListBox*			markerlist_;
    uiListBoxFilter*		markerfilter_;

    uiToolButton*		fromwellbut_;
    uiToolButton*		fromlogbut_;
    uiToolButton*		frommarkerbut_;

    const ObjectSet<Well::Data>* wds_ = nullptr;
};
