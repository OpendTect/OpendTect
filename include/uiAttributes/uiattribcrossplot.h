#ifndef uiattribcrossplot_h
#define uiattribcrossplot_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: uiattribcrossplot.h,v 1.2 2008-02-18 22:40:19 cvsbert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
#include "bufstringset.h"

namespace Attrib { class DescSet; }
class uiListBox;
class uiPosProvider;


class uiAttribCrossPlot : public uiDialog
{
public:
				uiAttribCrossPlot(uiParent*,
						  const Attrib::DescSet&);
				~uiAttribCrossPlot();

protected:

    uiListBox*		attrsfld_;
    uiPosProvider*	posprovfld_;

    bool		acceptOK(CallBacker*);

    const Attrib::DescSet& ads_;
    BufferStringSet	attrdefs_;
};


#endif
