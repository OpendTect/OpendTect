#ifndef uiviscolortabed_h
#define uiviscolortabed_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		24-01-2003
 RCS:		$Id: uiviscoltabed.h,v 1.9 2006-08-14 09:17:06 cvsnanne Exp $
________________________________________________________________________


-*/

#include "colortab.h"
#include "uigroup.h"
#include "uidialog.h"

namespace visBase { class VisColorTab; }
class ColorTableEditor;

/*!\brief

*/

class uiVisColTabEd : public uiGroup
{
public:
    				uiVisColTabEd(uiParent*,bool vert=true);
				~uiVisColTabEd();

    int				getColTab() const;
    void			setColTab(int coltabid);
    void			setHistogram(const TypeSet<float>*);
    void			setPrefHeight(int);

    Notifier<uiVisColTabEd>	sequenceChange;
    Notifier<uiVisColTabEd>	coltabChange;

protected:

    void			colTabEdChangedCB(CallBacker*);
    void			colTabChangedCB(CallBacker*);
    void			delColTabCB(CallBacker*);
    void			updateEditor();
    void			enableCallBacks();
    void			disableCallBacks();

    const CallBack		coltabcb;

    visBase::VisColorTab*	coltab_;
    ColorTableEditor*		coltabed_;

    ColorTable			colseq_;
    Interval<float>		coltabinterval_;
    bool			coltabautoscale_;
    float			coltabcliprate_;
};


class uiColorBarDialog :  public uiDialog
{
public:
    				uiColorBarDialog( uiParent* , int coltabid,
						  const char* title);

    void			setColTab( int id );
    Notifier<uiColorBarDialog>	winClosing;

protected:
    bool			closeOK();
    uiVisColTabEd*		coltabed_;
};


#endif
