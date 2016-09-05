#ifndef uihorizonsortdlg_h
#define uihorizonsortdlg_h

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
#include "dbkey.h"

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
    void			getSortedHorizonIDs(TypeSet<DBKey>&) const;
    void			setConstSelected(const TypeSet<DBKey>&);
    TrcKeyZSampling		getBoundingBox() const	{ return bbox_; }

protected:

    bool			acceptOK();
    void			getSelectedHorizons(TypeSet<DBKey>&) const;
    void			updateRelationTree(const TypeSet<DBKey>&);
    bool			sortFromRelationTree(const TypeSet<DBKey>&);

    uiSurfaceSel*		horsel_;

    bool			is2d_;
    bool			loadneeded_;
    TrcKeyZSampling		bbox_;
    TypeSet<DBKey>		constselids_;
    ObjectSet<EM::Horizon>	horizons_;
    TypeSet<DBKey>		horids_;

};

#endif
