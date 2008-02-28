#ifndef uiattribcrossplot_h
#define uiattribcrossplot_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: uiattribcrossplot.h,v 1.4 2008-02-28 15:54:29 cvshelene Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
#include "bufstringset.h"

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

protected:

    uiListBox*		attrsfld_;
    uiPosProvider*	posprovfld_;
    uiPosFilterSetSel*	posfiltfld_;

    bool		acceptOK(CallBacker*);
    int			prepareStoredDesc(int);

    const Attrib::DescSet&	ads_;
    BufferStringSet	attrdefs_;
};


#endif
