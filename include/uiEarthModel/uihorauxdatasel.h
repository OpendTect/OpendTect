#pragma once
/*+
___________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          April 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "bufstringset.h"
#include "multiid.h"
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
			HorizonAuxDataInfo(bool load);
			/*!<If load is true, object will fill itself with
			    all horizons with at least one aux data. */
			
			HorizonAuxDataInfo(const HorizonAuxDataInfo& n);

			~HorizonAuxDataInfo();
					
	BufferStringSet			hornms_;
	TypeSet<MultiID>		mids_;
	TypeSet<BufferStringSet>	auxdatanms_;		    
    };

    			uiHorizonAuxDataSel(uiParent*,const MultiID&,int auxidx,
				const HorizonAuxDataInfo* auxinfo = 0);

    int			nrHorizonsWithData() const { return nrhorswithdata_; }
    const MultiID&	selectedID() const	   { return selmid_; }
    int			auxdataidx() const	   { return auxidx_; }
protected:

    void		auxidxChg(CallBacker*);
    void		selCB(CallBacker*);

    uiPushButton*	selbut_;
    uiGenInput*		horfld_;
    uiGenInput*		auxfld_;
    MultiID		selmid_;
    int			auxidx_;
    int			nrhorswithdata_;
    uiHorizonAuxDataDlg* dlg_; 
};


