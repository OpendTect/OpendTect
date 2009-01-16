#ifndef uiviscolortabed_h
#define uiviscolortabed_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		24-01-2003
 RCS:		$Id: uiviscoltabed.h,v 1.20 2009-01-16 03:35:53 cvssatyaki Exp $
________________________________________________________________________


-*/

#include "uidialog.h"

namespace visBase { class VisColorTab; }
namespace ColTab { class Sequence; class MapperSetup; }
class uiColorTable;
class uiGroup;
class IOPar;


mClass uiVisColTabEd : public CallBacker
{
public:
    				uiVisColTabEd(uiParent*,bool vert=true);
				~uiVisColTabEd();

    void			setColTab(const ColTab::Sequence*,
	    				  bool editseq,
	    				  const ColTab::MapperSetup*);
    const ColTab::Sequence&	getColTabSequence() const;
    const ColTab::MapperSetup&	getColTabMapperSetup() const;

    NotifierAccess&		seqChange();
    NotifierAccess&		mapperChange();
    
    void			setHistogram(const TypeSet<float>*);
    void			setPrefHeight(int);
    void			setPrefWidth(int);
    uiGroup*			colTabGrp()	{ return (uiGroup*)uicoltab_; }
    uiColorTable*		colTab()	{ return uicoltab_; }

    bool			usePar(const IOPar&);
    void                        fillPar(IOPar&);
    void			setDefaultColTab();

    static const char*          sKeyColorSeq();
    static const char*          sKeyScaleFactor();
    static const char*          sKeyClipRate();
    static const char*          sKeyAutoScale();
    static const char*          sKeySymmetry();
    static const char*          sKeySymMidval();
    void			colseqChanged(CallBacker*);
    void			colorTabChgdCB(CallBacker*);
    void			clipperChanged(CallBacker*);

protected:

    uiColorTable*		uicoltab_;
};


mClass uiColorBarDialog :  public uiDialog
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
