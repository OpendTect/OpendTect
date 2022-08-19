#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uidialog.h"
#include "uistring.h"

class DataPointSetDisplayMgr;

class uiWellLogExtractGrp;
namespace Attrib { class DescSet; }


mExpClass(uiWellAttrib) uiWellAttribCrossPlot : public uiDialog
{ mODTextTranslationClass(uiWellAttribCrossPlot);
public:
					uiWellAttribCrossPlot(uiParent*,
						const Attrib::DescSet*);
					~uiWellAttribCrossPlot();

    void				setDescSet(const Attrib::DescSet*);
    void				setDisplayMgr(
	    					DataPointSetDisplayMgr* mgr)
					{ dpsdispmgr_ = mgr; }

protected:

    uiWellLogExtractGrp*		wellextractgrp_;
    DataPointSetDisplayMgr* 		dpsdispmgr_;

    bool				acceptOK(CallBacker*) override;
};
