#ifndef uiseislinesel_h
#define uiseislinesel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Nov 2008
 RCS:		$Id: uiseislinesel.h,v 1.25 2010/07/28 10:42:24 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uicompoundparsel.h"
#include "uidialog.h"
#include "bufstring.h"
#include "bufstringset.h"
#include "ranges.h"
#include "multiid.h"

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


mClass uiSeis2DMultiLineSel : public uiCompoundParSel
{
public:

    struct Setup
    {
				Setup( const char* txt=0 )
				    : lbltxt_(txt)
				    , withlinesetsel_(true)
				    , withz_(false)
				    , withattr_(false)
				    , allattribs_(true)
				    , steering_(false)
				    , withstep_(false)
				    , filldef_(true)	{}

	mDefSetupMemb(BufferString,lbltxt)
	mDefSetupMemb(bool,withlinesetsel)
	mDefSetupMemb(bool,withz)
	mDefSetupMemb(bool,withattr)
	mDefSetupMemb(bool,allattribs)
	mDefSetupMemb(bool,steering)
	mDefSetupMemb(bool,withstep)
	mDefSetupMemb(bool,filldef)
    };

    				uiSeis2DMultiLineSel(uiParent*,const Setup&);
				~uiSeis2DMultiLineSel();

    BufferString		getSummary() const;
    IOObj*			getIOObj();		// becomes yours
    const IOObj*		ioObj() const;
    BufferString		getAttribName() const;
    const BufferStringSet&	getSelLines() const;
    bool			isAll() const;
    const StepInterval<float>&	getZRange() const;
    const TypeSet<StepInterval<int> >&	getTrcRgs() const;

    void			setLineSet(const MultiID&,const char* attr=0);
    void			setSelLines(const BufferStringSet&);
    void			setAll(bool);
    void			setZRange(const StepInterval<float>&);
    void			setTrcRgs(const TypeSet<StepInterval<int> >&);

    bool			fillPar(IOPar&) const;
    void			usePar(const IOPar&);

protected:

    Setup&			setup_;

    bool			isall_;
    CtxtIOObj&			ctio_;
    BufferString		attrnm_;
    BufferStringSet		sellines_;
    StepInterval<float>		zrg_;
    TypeSet<StepInterval<int> >	trcrgs_;

    void			doDlg(CallBacker*);
    void			updateFromLineset();
};


mClass uiSeis2DMultiLineSelDlg : public uiDialog
{
public:
    				uiSeis2DMultiLineSelDlg(uiParent*,CtxtIOObj&,
					const uiSeis2DMultiLineSel::Setup&);
				~uiSeis2DMultiLineSelDlg()	{}

    BufferString		getSummary() const;

    IOObj*			getIOObj();
    const char*			getAttribName() const;
    void			getSelLines(BufferStringSet&) const;
    bool			isAll() const;
    void			getZRange(StepInterval<float>&) const;
    void			getTrcRgs(TypeSet<StepInterval<int> >&) const;

    void			setLineSet(const MultiID&,const char* attr=0);
    void			setSelection(const BufferStringSet&,
	    			       const TypeSet<StepInterval<int> >* rg=0);
    void			setAll(bool);
    void			setZRange(const StepInterval<float>&);

protected:

    const uiSeis2DMultiLineSel::Setup&	setup_;

    CtxtIOObj&			ctio_;
    uiSeisSel*			linesetfld_;
    uiListBox*			lnmsfld_;
    uiSelNrRange*		trcrgfld_;
    uiSelZRange*		zrgfld_;

    TypeSet<StepInterval<int> >	maxtrcrgs_;
    TypeSet<StepInterval<int> >	trcrgs_;

    void			finalised(CallBacker*);
    void			lineSetSel(CallBacker*);
    void			lineSel(CallBacker*);
    void			trcRgChanged(CallBacker*);

    virtual bool		acceptOK(CallBacker*);
};

#endif
