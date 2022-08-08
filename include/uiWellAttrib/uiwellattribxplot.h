#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          June 2005
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


