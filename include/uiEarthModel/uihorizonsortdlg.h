#ifndef uihorizonsortdlg_h
#define uihorizonsortdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          January 2006
 RCS:           $Id: uihorizonsortdlg.h,v 1.7 2009-07-22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
#include "cubesampling.h"
#include "multiid.h"

namespace EM { class Horizon; }

class IOPar;
class uiSurfaceSel;

mClass uiHorizonSortDlg : public uiDialog
{
public:

				uiHorizonSortDlg(uiParent*,bool is2d);
				~uiHorizonSortDlg();

    void			setParConstraints(const IOPar&,
	    					  bool includeconstraints,
						  bool allowcnstrsabsent);
    void			getSortedHorizons(
					ObjectSet<EM::Horizon>&) const;
    CubeSampling		getBoundingBox() const	{ return bbox_; }
    void			setLineID(const MultiID&);

protected:

    bool			acceptOK(CallBacker*);
    void			getSelectedHorizons(TypeSet<MultiID>&) const;

    uiSurfaceSel*		horsel_;

    bool			is2d_;
    CubeSampling		bbox_;
    ObjectSet<EM::Horizon>	horizons_;

};

#endif
