#ifndef uihorizonsortdlg_h
#define uihorizonsortdlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          January 2006
 RCS:           $Id: uihorizonsortdlg.h,v 1.1 2006-04-28 15:22:54 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
#include "cubesampling.h"
#include "multiid.h"

namespace EM { class Horizon; }

class uiIOObjSelGrp;

class uiHorizonSortDlg : public uiDialog
{
public:
				uiHorizonSortDlg(uiParent*);
				~uiHorizonSortDlg();

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
