#ifndef uihorizonsortdlg_h
#define uihorizonsortdlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          January 2006
 RCS:           $Id: uihorizonsortdlg.h,v 1.2 2006-05-26 08:13:25 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
#include "cubesampling.h"
#include "multiid.h"

namespace EM { class Horizon; }

class IOPar;
class uiIOObjSelGrp;

class uiHorizonSortDlg : public uiDialog
{
public:
				uiHorizonSortDlg(uiParent*);
				~uiHorizonSortDlg();

    void			setParConstraints(const IOPar&,
	    					  bool includeconstraints,
						  bool allowcnstrsabsent);
    void			getSortedHorizons(
					ObjectSet<EM::Horizon>&) const;
    CubeSampling		getBoundingBox() const	{ return bbox_; }

protected:
    bool			acceptOK(CallBacker*);
    void			getSelectedHorizons(TypeSet<MultiID>&) const;

    uiIOObjSelGrp*		ioobjselgrp_;

    CubeSampling		bbox_;
    ObjectSet<EM::Horizon>	horizons_;
};

#endif
