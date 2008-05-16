#ifndef uiseisrandto2dline_h
#define uiseisrandto2dline_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		May 2008
 RCS:		$Id: uiseisrandto2dline.h,v 1.1 2008-05-16 11:36:36 cvsraman Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class IOObj;
class LineKey;
class uiSeisSel;
class uiGenInput;

namespace Geometry { class RandomLine; }

class uiSeisRandTo2DLineDlg : public uiDialog
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
