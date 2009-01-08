#ifndef uiseisrandto2dline_h
#define uiseisrandto2dline_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		May 2008
 RCS:		$Id: uiseisrandto2dline.h,v 1.2 2009-01-08 08:31:03 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class IOObj;
class LineKey;
class uiSeisSel;
class uiGenInput;

namespace Geometry { class RandomLine; }

mClass uiSeisRandTo2DLineDlg : public uiDialog
{
public:
    			uiSeisRandTo2DLineDlg(uiParent*,
					      const Geometry::RandomLine&);
			~uiSeisRandTo2DLineDlg();

protected:

    CtxtIOObj&		inctio_;
    CtxtIOObj&		outctio_;

    const Geometry::RandomLine& randln_;

    uiSeisSel*		inpfld_;
    uiSeisSel*          outpfld_;
    uiGenInput*		linenmfld_;
    uiGenInput*		trcnrfld_;

    bool		acceptOK(CallBacker*);
};

#endif
