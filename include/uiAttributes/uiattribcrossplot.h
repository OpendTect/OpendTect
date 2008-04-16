#ifndef uiattribcrossplot_h
#define uiattribcrossplot_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: uiattribcrossplot.h,v 1.7 2008-04-16 10:13:16 cvsbert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
class uiPosProvider;
class uiPosFilterSetSel;
class uiListBox;
class uiComboBox;
namespace Attrib { class DescSet; }
namespace PosInfo { class Line2DData; }


class uiAttribCrossPlot : public uiDialog
{
public:
			uiAttribCrossPlot(uiParent*,
					  const Attrib::DescSet&);
			~uiAttribCrossPlot();

    void		setDescSet(const Attrib::DescSet&);

protected:

    const Attrib::DescSet& ads_;
    PosInfo::Line2DData* l2ddata_;

    uiListBox*		attrsfld_;
    uiPosProvider*	posprovfld_;
    uiPosFilterSetSel*	posfiltfld_;
    uiComboBox*		lnmfld_;

    void		adsChg();
    void		lnmChg(CallBacker*);

    bool		acceptOK(CallBacker*);
};


#endif
