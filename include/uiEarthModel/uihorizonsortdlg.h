#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          January 2006
________________________________________________________________________

-*/


#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "trckeyzsampling.h"
#include "multiid.h"

namespace EM { class Horizon; }

class uiSurfaceSel;

mExpClass(uiEarthModel) uiHorizonSortDlg : public uiDialog
{ mODTextTranslationClass(uiHorizonSortDlg);
public:

				uiHorizonSortDlg(uiParent*,bool is2d,
						 bool loadneeded=true);
				~uiHorizonSortDlg();

    void			setParConstraints(const IOPar&,
	    					  bool includeconstraints,
						  bool allowcnstrsabsent);
    void			getSortedHorizons(
					ObjectSet<EM::Horizon>&) const;
    void			getSortedHorizonIDs(TypeSet<MultiID>&) const;
    void			setSelected(const TypeSet<MultiID>&);
    TrcKeyZSampling		getBoundingBox() const	{ return bbox_; }

protected:

    bool			acceptOK(CallBacker*) override;
    void			getSelectedHorizons(TypeSet<MultiID>&) const;
    void			updateRelationTree(const TypeSet<MultiID>&);
    bool			sortFromRelationTree(const TypeSet<MultiID>&);

    uiSurfaceSel*		horsel_;

    bool			is2d_;
    bool			loadneeded_;
    TrcKeyZSampling		bbox_;
    ObjectSet<EM::Horizon>	horizons_;
    TypeSet<MultiID>		horids_;

};

