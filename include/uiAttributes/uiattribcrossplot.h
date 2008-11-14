#ifndef uiattribcrossplot_h
#define uiattribcrossplot_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: uiattribcrossplot.h,v 1.8 2008-11-14 05:36:19 cvssatyaki Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
class uiDataPointSet;
class uiPosProvider;
class uiPosFilterSetSel;
class uiListBox;
class uiComboBox;
namespace Attrib { class DescSet; }
namespace PosInfo { class Line2DData; }
namespace Pick { class Set; }


class uiAttribCrossPlot : public uiDialog
{
public:
					uiAttribCrossPlot(uiParent*,
						  const Attrib::DescSet&);
					~uiAttribCrossPlot();

    void				setDescSet(const Attrib::DescSet&);
    const Pick::Set&			getSelectedPts() const
					{ return *selptps_; }
    Notifier<uiAttribCrossPlot>		pointsSelected;

protected:

    const Attrib::DescSet& 		ads_;
    PosInfo::Line2DData* 		l2ddata_;

    uiListBox*				attrsfld_;
    uiPosProvider*			posprovfld_;
    uiPosFilterSetSel*			posfiltfld_;
    uiComboBox*				lnmfld_;
    uiDataPointSet*			uidps_;

    Pick::Set*				selptps_;
    void				adsChg();
    void				lnmChg(CallBacker*);
    void				createPickSet(CallBacker*);

    bool				acceptOK(CallBacker*);
};


#endif
