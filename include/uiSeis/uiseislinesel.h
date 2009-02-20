
#ifndef uiseislinesel_h
#define uiseislinesel_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Nov 2008
 RCS:		$Id: uiseislinesel.h,v 1.14 2009-02-20 08:47:04 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uicompoundparsel.h"
#include "uidialog.h"
#include "bufstring.h"
#include "bufstringset.h"
#include "ranges.h"

class uiLabeledSpinBox;
class uiListBox;
class uiSeisSel;
class uiSelNrRange;

class CtxtIOObj;
class IOObj;


mClass uiSeis2DLineSubSel : public uiDialog
{
public:
    					uiSeis2DLineSubSel(uiParent*,
						           CtxtIOObj& lsctio);
					~uiSeis2DLineSubSel()	{}

    BufferString			getSummary() const;

    void				setSelLines(const BufferStringSet&);
    const BufferStringSet&		getSelLines() const { return sellines_;}
    Interval<int>			getTrcRange(const char* lnm) const;

protected:

    BufferStringSet 			sellines_;
    uiSeisSel*  			linesetfld_;
    uiListBox*  			lnmsfld_;
    uiSelNrRange*			trcrgfld_;
    CtxtIOObj&				lsctio_;

    TypeSet< StepInterval<int> > 	maxtrcrgs_;
    TypeSet< StepInterval<int> >	trcrgs_;

    void				finalised(CallBacker*);
    void 				lineSetSel(CallBacker*);
    void 				lineSel(CallBacker*);
    void				trcChanged(CallBacker*);

    virtual bool        		acceptOK(CallBacker*);
};


mClass uiSelection2DParSel : public uiCompoundParSel
{
public:
    				uiSelection2DParSel(uiParent*);
				~uiSelection2DParSel();

    BufferString                getSummary() const;
    void                        doDlg(CallBacker*);
    IOObj*                      getIOObj();
    const BufferStringSet&      getSelLines() const     { return sellines_; }
    void 			fillPar(IOPar &);

protected:

    BufferStringSet             sellines_;
    CtxtIOObj*                  lsctio_;
    uiSeis2DLineSubSel*         linesel_;
};   

#endif
