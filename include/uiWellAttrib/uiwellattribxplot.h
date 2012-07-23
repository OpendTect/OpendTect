#ifndef uiwellattribxplot_h
#define uiwellattribxplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: uiwellattribxplot.h,v 1.16 2012-07-23 09:32:25 cvssatyaki Exp $
________________________________________________________________________

-*/


#include "uidialog.h"

class DataPointSetDisplayMgr;

class uiWellLogExtractGrp;
namespace Attrib { class DescSet; }


mClass uiWellAttribCrossPlot : public uiDialog
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
