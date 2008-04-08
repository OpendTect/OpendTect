#ifndef uiviscolortabed_h
#define uiviscolortabed_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		24-01-2003
 RCS:		$Id: uiviscoltabed.h,v 1.15 2008-04-08 05:05:07 cvssatyaki Exp $
________________________________________________________________________


-*/

#include "uidialog.h"

namespace visBase { class VisColorTab; }
namespace ColTab { class Sequence; }
class uiColorTable;
class uiGroup;
class IOPar;


class uiVisColTabEd : public CallBacker
{
public:
    				uiVisColTabEd(uiParent*,bool vert=true);
				~uiVisColTabEd();

    int				getColTab() const;
    visBase::VisColorTab*	getColTab() 		{ return viscoltab_; }
    void			setColTab(int coltabid);
    void			setHistogram(const TypeSet<float>*);
    void			setPrefHeight(int);
    void			setPrefWidth(int);
    void			updateColTabList();
    uiGroup*			colTabGrp()	{ return (uiGroup*)uicoltab_; }

    bool			usePar(const IOPar&);
    void                        fillPar(IOPar&);
    void			setDefaultColTab();

    Notifier<uiVisColTabEd>	sequenceChange;
    Notifier<uiVisColTabEd>	coltabChange;

    static const char*          sKeyColorSeq();
    static const char*          sKeyScaleFactor();
    static const char*          sKeyClipRate();
    static const char*          sKeyAutoScale();
    static const char*          sKeySymmetry();
    void			colTabEdChangedCB(CallBacker*);
    void			colseqChanged(CallBacker*);
    void			colorTabChgdCB(CallBacker*);
    void			colTabChangedCB(CallBacker*);
    void			clipperChanged(CallBacker*);

protected:

    void			delColTabCB(CallBacker*);
    void			updateEditor();
    void			enableCallBacks();
    void			disableCallBacks();

    const CallBack		coltabcb;

    visBase::VisColorTab*	viscoltab_;
    uiColorTable*		uicoltab_;

    ColTab::Sequence&		colseq_;
    Interval<float>		coltabinterval_;
    bool			coltabautoscale_;
    float			coltabcliprate_;
    float			ctsymidval_;
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
