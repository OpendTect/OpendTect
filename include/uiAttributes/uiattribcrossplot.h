#ifndef uiattribcrossplot_h
#define uiattribcrossplot_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: uiattribcrossplot.h,v 1.6 2008-04-07 11:02:08 cvsbert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"

namespace Attrib { class DescSet; }
class uiListBox;
class uiPosProvider;
class uiPosFilterSetSel;


class uiAttribCrossPlot : public uiDialog
{
public:
			uiAttribCrossPlot(uiParent*,
					  const Attrib::DescSet&);
			~uiAttribCrossPlot();

    void		setDescSet(const Attrib::DescSet&);

protected:

    uiListBox*		attrsfld_;
    uiPosProvider*	posprovfld_;
    uiPosFilterSetSel*	posfiltfld_;

    bool		acceptOK(CallBacker*);
    void		adsChg();
    const Attrib::DescSet& ads_;
};


#endif
