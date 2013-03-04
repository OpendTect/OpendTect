#ifndef uiflatviewcoltabed_h
#define uiflatviewcoltabed_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		May 2008
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uiflatviewmod.h"
#include "callback.h"
#include "flatview.h"

namespace ColTab { class Sequence; }
class uiColorTable;
class uiGroup;
class uiParent;

/*!
\brief FlatView color table editor.
*/

mExpClass(uiFlatView) uiFlatViewColTabEd : public CallBacker
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

