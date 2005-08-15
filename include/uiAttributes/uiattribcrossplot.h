#ifndef uiattribcrossplot_h
#define uiattribcrossplot_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: uiattribcrossplot.h,v 1.1 2005-08-15 15:50:25 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
#include "bufstringset.h"

namespace Attrib { class DescSet; }
class uiLabeledListBox;


class uiAttribCrossPlot : public uiDialog
{
public:
				uiAttribCrossPlot(uiParent*,
						  const Attrib::DescSet&);
				~uiAttribCrossPlot();

protected:

    uiLabeledListBox*		attrsfld;
    uiLabeledListBox*		pssfld;

    bool			acceptOK(CallBacker*);
    void			saveData(CallBacker*);

    const Attrib::DescSet&	ads_;
    BufferStringSet		attrdefs_;
    BufferStringSet		psdefs_;
};


#endif
