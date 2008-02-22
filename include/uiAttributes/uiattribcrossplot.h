#ifndef uiattribcrossplot_h
#define uiattribcrossplot_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: uiattribcrossplot.h,v 1.3 2008-02-22 15:02:45 cvsbert Exp $
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

    const Attrib::DescSet& ads_;
    BufferStringSet	attrdefs_;
};


#endif
