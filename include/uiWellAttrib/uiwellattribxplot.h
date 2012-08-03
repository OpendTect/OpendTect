#ifndef uiwellattribxplot_h
#define uiwellattribxplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: uiwellattribxplot.h,v 1.17 2012-08-03 13:01:22 cvskris Exp $
________________________________________________________________________

-*/


#include "uiwellattribmod.h"
#include "uidialog.h"

class DataPointSetDisplayMgr;

class uiWellLogExtractGrp;
namespace Attrib { class DescSet; }


mClass(uiWellAttrib) uiWellAttribCrossPlot : public uiDialog
{
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

    bool				acceptOK(CallBacker*);
};


#endif

