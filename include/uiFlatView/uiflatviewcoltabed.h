#ifndef uiflatviewcoltabed_h
#define uiflatviewcoltabed_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		May 2008
 RCS:		$Id: uiflatviewcoltabed.h,v 1.2 2008-08-07 03:51:21 cvsnanne Exp $
________________________________________________________________________


-*/

#include "callback.h"
#include "flatview.h"

namespace ColTab { class Sequence; }
class uiColorTable;
class uiGroup;
class uiParent;


class uiFlatViewColTabEd : public CallBacker
{
public:
    				uiFlatViewColTabEd(uiParent*,FlatView::Viewer&);
				~uiFlatViewColTabEd();

    uiGroup*			colTabGrp()	{ return (uiGroup*)uicoltab_; }
    void			setColTab();

    Notifier<uiFlatViewColTabEd> colTabChgd;

protected:

    FlatView::DataDispPars&	ddpars_;
    ColTab::Sequence&		colseq_;
    uiColorTable*		uicoltab_;

    void			colTabChanged(CallBacker*);
};

#endif
