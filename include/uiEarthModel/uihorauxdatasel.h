#pragma once
/*+
___________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          April 2010
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "bufstringset.h"
#include "dbkey.h"
#include "uigroup.h"

class uiHorizonAuxDataDlg;
class uiGenInput;
class uiPushButton;

mExpClass(uiEarthModel) uiHorizonAuxDataSel : public uiGroup
{ mODTextTranslationClass(uiHorizonAuxDataSel);
public:

    mExpClass(uiEarthModel) HorizonAuxDataInfo
    {
    public:
			HorizonAuxDataInfo(bool load,uiParent* waitinfoprnt=0);
			/*!<If load is true, object will fill itself with
			    all horizons with at least one aux data. */

			HorizonAuxDataInfo(const HorizonAuxDataInfo& n);

			~HorizonAuxDataInfo();

	BufferStringSet			hornms_;
	DBKeySet			dbkys_;
	TypeSet<BufferStringSet>	auxdatanms_;
    };

			uiHorizonAuxDataSel(uiParent*,const DBKey&,int auxidx,
				const HorizonAuxDataInfo* auxinfo = 0);

    int			nrHorizonsWithData() const { return nrhorswithdata_; }
    const DBKey&	selectedID() const	   { return selmid_; }
    int			auxdataidx() const	   { return auxidx_; }
protected:

    void		auxidxChg(CallBacker*);
    void		selCB(CallBacker*);

    uiPushButton*	selbut_;
    uiGenInput*		horfld_;
    uiGenInput*		auxfld_;
    DBKey		selmid_;
    int			auxidx_;
    int			nrhorswithdata_;
    uiHorizonAuxDataDlg* dlg_;
};
