#ifndef uiflatviewcoltabed_h
#define uiflatviewcoltabed_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		May 2008
 RCS:		$Id: uiflatviewcoltabed.h,v 1.3 2008-10-27 11:21:08 cvssatyaki Exp $
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
    void			setColTab( const FlatView::Viewer& );

    Notifier<uiFlatViewColTabEd> colTabChgd;

protected:

    FlatView::DataDispPars&	ddpars_;
    ColTab::Sequence&		colseq_;
    uiColorTable*		uicoltab_;
    FlatView::Viewer*		vwr_;

    void			colTabChanged(CallBacker*);
};

#endif
