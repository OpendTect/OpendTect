#ifndef uihorizonsortdlg_h
#define uihorizonsortdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          January 2006
 RCS:           $Id: uihorizonsortdlg.h,v 1.10 2012-08-03 13:00:56 cvskris Exp $
________________________________________________________________________

-*/


#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "cubesampling.h"
#include "multiid.h"

namespace EM { class Horizon; }

class IOPar;
class uiSurfaceSel;

mClass(uiEarthModel) uiHorizonSortDlg : public uiDialog
{
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
    void			setConstSelected(const TypeSet<MultiID>&);
    CubeSampling		getBoundingBox() const	{ return bbox_; }
    void			setLineID(const MultiID&);

protected:

    bool			acceptOK(CallBacker*);
    void			getSelectedHorizons(TypeSet<MultiID>&) const;
    void			updateRelationTree(const TypeSet<MultiID>&);
    bool			sortFromRelationTree(const TypeSet<MultiID>&);

    uiSurfaceSel*		horsel_;

    bool			is2d_;
    bool			loadneeded_;
    CubeSampling		bbox_;
    TypeSet<MultiID>		constselids_;
    ObjectSet<EM::Horizon>	horizons_;
    TypeSet<MultiID>		horids_;

};

#endif

