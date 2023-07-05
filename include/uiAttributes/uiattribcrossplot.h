#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"

#include "bufstringset.h"
#include "datapointset.h"

class DataPointSetDisplayMgr;
class uiDataPointSet;
class uiListBox;
class uiPosFilterSetSel;
class uiPosProvider;
namespace Attrib { class DescSet; class SelInfo; }


mExpClass(uiAttributes) uiAttribCrossPlot : public uiDialog
{ mODTextTranslationClass(uiAttribCrossPlot);
public:
					uiAttribCrossPlot(uiParent*,
						const Attrib::DescSet&);
					~uiAttribCrossPlot();

    void				setDescSet(const Attrib::DescSet&);
    void				setDisplayMgr(
					    DataPointSetDisplayMgr* dispmgr )
					{ dpsdispmgr_ = dispmgr; }

    ConstRefMan<DataPointSet>		getDPS() const;

protected:

    const Attrib::DescSet& 		ads_;
    Attrib::SelInfo*			attrinfo_		= nullptr;
    TypeSet<int>			selidxs_;
    TypeSet<MultiID>			selids_;
    TypeSet<BufferStringSet>		linenmsset_;

    uiListBox*				attrsfld_;
    uiPosProvider*			posprovfld_;
    uiPosFilterSetSel*			posfiltfld_;
    uiListBox*				lnmfld_			= nullptr;
    RefMan<DataPointSet>		curdps_;
    DataPointSetDisplayMgr*		dpsdispmgr_		= nullptr;
    ObjectSet<uiDataPointSet>		dpsset_;

    void				adsChg();
    MultiID				getSelectedID() const;
    void				getLineNames(BufferStringSet&);
    void				initWin(CallBacker*);
    void				lineChecked(CallBacker*);
    void				attrChecked(CallBacker*);
    void				attrChanged(CallBacker*);

    bool				acceptOK(CallBacker*) override;
};
