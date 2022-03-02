#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert/Nanne
 Date:          Aug 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "uicombobox.h"
#include "uigroup.h"
#include "uitoolbar.h"
#include "flatview.h"
#include "uistring.h"

class uiAutoRangeClipDlg;
class uiColorTableCanvas;
class uiLineEdit;


namespace ColTab { class Sequence; class MapperSetup; }

mExpClass(uiTools) uiColorTableSel : public uiComboBox
{ mODTextTranslationClass(uiColorTableSel);
public:
			uiColorTableSel(uiParent*,const char* nm);
			~uiColorTableSel();

    void		update();
    void		setCurrent(const ColTab::Sequence&);
    void		setCurrent(const char* seqnm);
    const char*		getCurrent() const;

protected:
    void		seqChgCB(CallBacker*);
};


mExpClass(uiTools) uiColorTable : public CallBacker
{ mODTextTranslationClass(uiColorTable);
public:
    virtual		~uiColorTable();

    const ColTab::Sequence&	colTabSeq() const	{ return coltabseq_;}
    const ColTab::MapperSetup&	colTabMapperSetup() const { return mapsetup_; }

    void		setSequence(const char*,bool emitnotif=true);
    void		setSequence(const ColTab::Sequence*,bool allowedit,
				    bool emitnotif=true);
			/*!If ptr is null, sequence edit will be disabled. */
    void		setMapperSetup(const ColTab::MapperSetup*,
				       bool emitnotif=true);
			/*!If ptr is null, mapper edit will be disabled. */
    void		setHistogram(const TypeSet<float>*);
    void		setInterval(const Interval<float>&);
    void		enableTransparencyEdit(bool);
    void		commitInput();

    void		enableManage(bool yn)		{ enabmanage_ = yn; }
    void		enableClippingDlg(bool yn)	{ enabclipdlg_ = yn; }

    void		setDispPars(const FlatView::DataDispPars::VD&);
    void		getDispPars(FlatView::DataDispPars::VD&) const;

    void		setNumberFormat(char format,int precision);
    void		getNumberFormat(char& format,int& precision) const;

    virtual OD::Orientation getOrientation() const		= 0;
    void		orientationChgd(CallBacker*);

    Notifier<uiColorTable>	seqChanged;
    Notifier<uiColorTable>	scaleChanged;

protected:
			uiColorTable(const ColTab::Sequence&);

    void		createFields(uiParent*,OD::Orientation,bool withminmax);
    bool		enabmanage_;
    bool		enabclipdlg_;

    ColTab::MapperSetup& mapsetup_;
    ColTab::Sequence&	coltabseq_;

    TypeSet<float>	histogram_;
    uiParent*		parent_;
    uiColorTableCanvas*	canvas_;
    uiLineEdit*		minfld_;
    uiLineEdit*		maxfld_;
    uiColorTableSel*	selfld_;
    uiAutoRangeClipDlg*	scalingdlg_;

    bool		enabletrans_;

    void		updateRgFld();
    void		canvasreDraw(CallBacker*);
    void		canvasClick(CallBacker*);
    void		canvasDoubleClick(CallBacker*);
    void		tabSel(CallBacker*);
    void		tableAdded(CallBacker*);
    void		rangeEntered(CallBacker*);
    void		doManage(CallBacker*);
    void		doFlip(CallBacker*);
    void		setAsDefault(CallBacker*);
    void		editScaling(CallBacker*);
    void		makeSymmetrical(CallBacker*);
    void		colTabChgdCB(CallBacker*);
    void		colTabManChgd(CallBacker*);
    void		numberFormatChgdCB(CallBacker*);

    bool		isEditable() const	{ return maxfld_; }
};


mExpClass(uiTools) uiColorTableGroup : public uiGroup , public uiColorTable
{ mODTextTranslationClass(uiColorTableGroup);
public:
			uiColorTableGroup(uiParent*,const ColTab::Sequence&,
					  OD::Orientation orient=OD::Horizontal,
					  bool nominmax=true);
					  //nominmax=true hides min/max fields
			uiColorTableGroup(uiParent*,
					  OD::Orientation=OD::Horizontal,
					  bool nominmax=true);
					  //nominmax=true hides min/max fields
			~uiColorTableGroup();

    virtual OD::Orientation getOrientation() const;

private:
    void		init(OD::Orientation,bool nominmax);
    OD::Orientation	orientation_;
};


mExpClass(uiTools) uiColorTableToolBar : public uiToolBar , public uiColorTable
{ mODTextTranslationClass(uiColorTableToolBar);
public:
			uiColorTableToolBar(uiParent*,const ColTab::Sequence&,
					    bool newline=false);
			uiColorTableToolBar(uiParent*,bool newline=false);
			~uiColorTableToolBar();

    virtual OD::Orientation getOrientation() const;

private:
    void		init();
};

