#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2005
________________________________________________________________________

-*/


#include "uiattributesmod.h"
#include "bufstringset.h"
#include "ioobj.h"


#include "uidialog.h"

class DataPointSet;
class DataPointSetDisplayMgr;
class uiDataPointSet;
class uiPosProvider;
class uiPosFilterSetSel;
class uiSeis2DLineNameSel;
class uiListBox;
class uiGenInput;
namespace Attrib { class DescSet; class SelInfo; }


mExpClass(uiAttributes) uiAttribCrossPlot : public uiDialog
{ mODTextTranslationClass(uiAttribCrossPlot);
public:

				uiAttribCrossPlot(uiParent*,
					  const Attrib::DescSet&);
				~uiAttribCrossPlot();

    void			setDescSet(const Attrib::DescSet&);
    void			setDisplayMgr( DataPointSetDisplayMgr* dispmgr )
				    { dpsdispmgr_ = dispmgr; }

    const DataPointSet&		getDPS() const;

protected:

    const Attrib::DescSet&	ads_;
    Attrib::SelInfo*		attrinfo_;
    TypeSet<int>		selidxs_;
    DBKeySet			selids_;
    TypeSet<BufferStringSet>	linenmsset_;

    uiListBox*			attrsfld_;
    uiPosProvider*		posprovfld_;
    uiPosFilterSetSel*		posfiltfld_;
    uiGenInput*			posfiltmodefld_;
    uiListBox*			lnmfld_;
    DataPointSet*		curdps_;
    DataPointSetDisplayMgr*	dpsdispmgr_;

    void			adsChg();
    DBKey			getSelectedID() const;
    void			getLineNames(BufferStringSet&);
    void			filterPosProv(bool chosen);
    void			initDlg(CallBacker*);
    void			lineChecked(CallBacker*);
    void			attrChecked(CallBacker*);
    void			attrChanged(CallBacker*);

    bool			acceptOK();

};
