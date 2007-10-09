#ifndef uiviscolortabed_h
#define uiviscolortabed_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		24-01-2003
 RCS:		$Id: uiviscoltabed.h,v 1.12 2007-10-09 07:35:02 cvsnanne Exp $
________________________________________________________________________


-*/

#include "colortab.h"
#include "uidialog.h"

namespace visBase { class VisColorTab; }
class ColorTableEditor;
class uiGroup;

/*!\brief

*/

class uiVisColTabEd : public CallBacker
{
public:
    				uiVisColTabEd(uiParent*,bool vert=true);
				~uiVisColTabEd();

    int				getColTab() const;
    void			setColTab(int coltabid);
    void			setHistogram(const TypeSet<float>*);
    void			setPrefHeight(int);
    void			updateColTabList();
    uiGroup*			colTabGrp()	{ return (uiGroup*)coltabed_; }

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
    bool			coltabsymmetry_;
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
