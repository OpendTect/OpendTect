#ifndef uiattribcrossplot_h
#define uiattribcrossplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: uiattribcrossplot.h,v 1.14 2009-09-30 14:26:14 cvsbert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
class DataPointSet;
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

    const DataPointSet&			getDPS() const;
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
    void				useLineName(bool);
    void				initWin(CallBacker*);
    void				lnmChg(CallBacker*);
    void				showSelPts(CallBacker*);
    void				removeSelPts(CallBacker*);

    bool				acceptOK(CallBacker*);
};


#endif
