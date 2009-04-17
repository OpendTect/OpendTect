
#ifndef uiseislinesel_h
#define uiseislinesel_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Nov 2008
 RCS:		$Id: uiseislinesel.h,v 1.16 2009-04-17 13:18:47 cvsbert Exp $
________________________________________________________________________

-*/

#include "uicompoundparsel.h"
#include "uidialog.h"
#include "bufstring.h"
#include "bufstringset.h"
#include "ranges.h"
#include "multiid.h"

class uiLabeledSpinBox;
class uiListBox;
class uiComboBox;
class uiSeisSel;
class uiSelNrRange;

class CtxtIOObj;
class IOObj;


mClass uiSeis2DLineSel : public uiCompoundParSel
{
public:

    			uiSeis2DLineSel(uiParent*,const char* lsnm=0);

    const char*		lineName() const	{ return lnm_; }
    const char*		lineSetName() const	{ return lsnm_; }
    MultiID		lineSetID() const;
    void		set(const char* lsnm,const char* lnm=0);

protected:

    BufferString	lnm_;
    BufferString	lsnm_;
    bool		fixedlsname_;

    BufferString	getSummary() const;

    void		selPush(CallBacker*);
};


mClass uiSeis2DLineNameSel : public uiGroup
{
public:

    			uiSeis2DLineNameSel(uiParent*,bool forread);

    const char*		getInput() const;
    void		setInput(const char*);
    void		setLineSet(const MultiID&);
    void		fillWithAll();

    int			getLineIndex() const; //!< Only usable when forread

    Notifier<uiSeis2DLineNameSel>	nameChanged;

protected:

    uiComboBox*		fld_;
    bool		forread_;
    MultiID		lsid_;

    void		addLineNames(const MultiID&);
    void		selChg( CallBacker* )	{ nameChanged.trigger(); }
    void		fillAll(CallBacker*);

};


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
