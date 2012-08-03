#ifndef uiscenecolorbarmgr_h
#define uiscenecolorbarmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Oct 2009
 RCS:           $Id: uiscenecolorbarmgr.h,v 1.2 2012-08-03 13:01:18 cvskris Exp $
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"

class uiGenInput;
class uiSpinBox;
namespace visBase{ class SceneColTab; }

mClass(uiVis) uiSceneColorbarMgr : public uiDialog
{
public:
			uiSceneColorbarMgr(uiParent*,visBase::SceneColTab*);
			~uiSceneColorbarMgr();

protected:
    visBase::SceneColTab* scenecoltab_;
    uiSpinBox*		widthfld_;
    uiSpinBox*		heightfld_;
    uiGenInput*		posfld_;

    void		sizeChangedCB(CallBacker*);
    void		posChangedCB(CallBacker*);
};

#endif


