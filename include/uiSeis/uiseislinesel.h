
#ifndef uiseislinesel_h
#define uiseislinesel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Nov 2008
 RCS:		$Id: uiseislinesel.h,v 1.19 2009-09-15 09:46:51 cvsraman Exp $
________________________________________________________________________

-*/

#include "uicompoundparsel.h"
#include "uidialog.h"
#include "bufstring.h"
#include "bufstringset.h"
#include "ranges.h"
#include "multiid.h"

class uiCheckBox;
class uiComboBox;
class uiLabeledSpinBox;
class uiListBox;
class uiSeisSel;
class uiSelNrRange;
class uiSelZRange;

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
    				uiSeis2DLineSubSel(uiParent*,CtxtIOObj& lsctio,
						   bool withz,bool attr=false);
				~uiSeis2DLineSubSel()	{}

    BufferString		getSummary() const;

    void			setLineSet(const MultiID&);
    void			setAttrName(const char*);
    void			setSelLines(const BufferStringSet&);
    void			setTrcRange(const StepInterval<int>&,
	    				    const char* lnm);
    void			setZRange(const StepInterval<float>&);

    const char*			getAttrName() const;
    const BufferStringSet&	getSelLines() const { return sellines_;}
    StepInterval<int>		getTrcRange(const char* lnm) const;
    StepInterval<float>		getZRange() const;

protected:

    bool			withattr_;
    BufferStringSet 		sellines_;
    uiSeisSel*  		linesetfld_;
    uiListBox*  		lnmsfld_;
    uiCheckBox*			allfld_;
    uiSelNrRange*		trcrgfld_;
    uiSelZRange*		zfld_;
    CtxtIOObj&			lsctio_;

    TypeSet< StepInterval<int> > 	maxtrcrgs_;
    TypeSet< StepInterval<int> >	trcrgs_;

    void			finalised(CallBacker*);
    void 			lineSetSel(CallBacker*);
    void 			lineSel(CallBacker*);
    void 			lineChk(CallBacker*);
    void			trcChanged(CallBacker*);
    void			allSel(CallBacker*);

    virtual bool        	acceptOK(CallBacker*);
};


mClass uiSelection2DParSel : public uiCompoundParSel
{
public:
    				uiSelection2DParSel(uiParent*,bool withz=false,
						    bool withattr=false);
				~uiSelection2DParSel();

    BufferString                getSummary() const;
    void                        doDlg(CallBacker*);
    IOObj*                      getIOObj();
    void			setIOObj(const MultiID&);
    const BufferStringSet&      getSelLines() const     { return sellines_; }
    void 			fillPar(IOPar&) const;
    void			usePar(const IOPar&);

protected:

    BufferStringSet             sellines_;
    TypeSet< StepInterval<int> >        trcrgs_;

    CtxtIOObj*                  lsctio_;
    uiSeis2DLineSubSel*         linesel_;
};   

#endif
