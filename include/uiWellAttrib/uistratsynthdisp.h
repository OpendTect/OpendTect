#ifndef uistratsynthdisp_h
#define uistratsynthdisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
 RCS:		$Id: uistratsynthdisp.h,v 1.1 2010-12-01 16:56:38 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class uiComboBox;
namespace Strat { class LayerModel; }


mClass uiStratSynthDisp : public uiGroup
{
public:

    			uiStratSynthDisp(uiParent*,const Strat::LayerModel&);
    			~uiStratSynthDisp();

    void		modelChanged();

protected:

    const Strat::LayerModel& lm_;

    uiComboBox*		wvltfld_;

    void		reDraw(CallBacker* cb=0);
    void		manWvlts(CallBacker*);
    bool		fillWvltField();

};


#endif
