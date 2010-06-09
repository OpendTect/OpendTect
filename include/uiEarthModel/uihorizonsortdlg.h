#ifndef uihorizonsortdlg_h
#define uihorizonsortdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          January 2006
 RCS:           $Id: uihorizonsortdlg.h,v 1.8 2010-06-09 10:40:54 cvsraman Exp $
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
    void			setConstSelected(const TypeSet<MultiID>&);
    CubeSampling		getBoundingBox() const	{ return bbox_; }
    void			setLineID(const MultiID&);

protected:

    bool			acceptOK(CallBacker*);
    void			getSelectedHorizons(TypeSet<MultiID>&) const;

    uiSurfaceSel*		horsel_;

    bool			is2d_;
    CubeSampling		bbox_;
    TypeSet<MultiID>		constselids_;
    ObjectSet<EM::Horizon>	horizons_;

};

#endif
