#ifndef uiviscolortabed_h
#define uiviscolortabed_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		24-01-2003
 RCS:		$Id: uiviscoltabed.h,v 1.2 2003-05-22 15:02:36 nanne Exp $
________________________________________________________________________


-*/

#include "colortab.h"
#include "uigroup.h"

namespace visBase { class VisColorTab; }
class ColorTableEditor;

/*!\brief

*/

class uiVisColTabEd : public uiGroup
{
public:
    				uiVisColTabEd(uiParent*);
				~uiVisColTabEd();

    void			setColTab(int coltabid);
    void			setHistogram(const TypeSet<float>&);
    void			setPrefHeight(int);

protected:

    void			colTabEdChangedCB(CallBacker*);
    void			colTabChangedCB(CallBacker*);
    void			delColTabCB(CallBacker*);
    void			updateEditor();
    void			enableCallBacks();
    void			disableCallBacks();

    const CallBack		coltabcb;

    visBase::VisColorTab*	coltab;
    ColorTableEditor*		coltabed;

    ColorTable			colseq;
    Interval<float>		coltabinterval;
    bool			coltabautoscale;
    float			coltabcliprate;
};


#endif
