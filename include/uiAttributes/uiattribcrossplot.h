#ifndef uiattribcrossplot_h
#define uiattribcrossplot_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: uiattribcrossplot.h,v 1.11 2009-04-16 14:45:05 cvsbert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
class uiDataPointSet;
class uiPosProvider;
class uiPosFilterSetSel;
class uiSeis2DLineNameSel;
class uiListBox;
namespace Attrib { class DescSet; }
namespace PosInfo { class Line2DData; }


mClass uiAttribCrossPlot : public uiDialog
{
public:
					uiAttribCrossPlot(uiParent*,
						  const Attrib::DescSet&);
					~uiAttribCrossPlot();

    void				setDescSet(const Attrib::DescSet&);
    Notifier<uiAttribCrossPlot>		pointsSelected;
    Notifier<uiAttribCrossPlot>		pointsTobeRemoved;

protected:

    const Attrib::DescSet& 		ads_;
    PosInfo::Line2DData* 		l2ddata_;

    uiListBox*				attrsfld_;
    uiPosProvider*			posprovfld_;
    uiPosFilterSetSel*			posfiltfld_;
    uiSeis2DLineNameSel*		lnmfld_;
    uiDataPointSet*			uidps_;

    void				adsChg();
    void				lnmChg(CallBacker*);
    void				showSelPts(CallBacker*);
    void				removeSelPts(CallBacker*);

    bool				acceptOK(CallBacker*);
};


#endif
