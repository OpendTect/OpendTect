#pragma once

/*+
  ________________________________________________________________________

  (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno/Satyaki
Date:	       Aug 2010/March 2015
RCS:	       $Id: uiwell2dlinedlg.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uidialog.h"
#include "uistring.h"
#include "bufstringset.h"
class IOObj;
class uiCheckBox;
class uiGenInput;
class uiPushButton;
class uiSeisRandTo2DBase;
class uiWellSelGrp;

namespace Geometry { class RandomLine; }

mExpClass(uiWellAttrib) uiWellTo2DLineDlg : public uiDialog
{ mODTextTranslationClass(uiWellTo2DLineDlg)
public:
			uiWellTo2DLineDlg(uiParent*);
			~uiWellTo2DLineDlg();

    void		getCoordinates(TypeSet<Coord>&);
    Pos::GeomID		get2DLineID() const;
    const IOObj*	get2DDataSetObj() const;
    bool		dispOnCreation();

    Notifier<uiWellTo2DLineDlg> wantspreview_;

protected:

    uiGenInput*		extendfld_;
    uiGenInput*		linenmfld_;
    uiCheckBox*		dispfld_;

    uiPushButton*	previewbutton_;
    Geometry::RandomLine* rl_;
    uiSeisRandTo2DBase* randto2dlinefld_;
    uiWellSelGrp*	wellselgrp_;

    void		attachFields();
    void		createFields();
    void		extendLine(TypeSet<Coord>&);

    bool		acceptOK(CallBacker*) override;
    void		previewPush(CallBacker*);

};

