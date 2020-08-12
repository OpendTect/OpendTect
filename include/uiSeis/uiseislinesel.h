#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Nov 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uicompoundparsel.h"
#include "uidialog.h"
#include "uistring.h"
#include "bufstring.h"
#include "bufstringset.h"
#include "multiid.h"
#include "ranges.h"
#include "posinfo2dsurv.h"

class uiComboBox;
class uiGenInput;
class uiIOObjInserter;
class uiListBox;
class uiListBoxChoiceIO;
class uiListBoxFilter;
class uiSeisSel;
class uiSelNrRange;
class uiSelZRange;
class uiSeis2DLineSel;

class CtxtIOObj;
class IOObj;


mExpClass(uiSeis) uiSeis2DLineChoose : public uiGroup
{ mODTextTranslationClass(uiSeis2DLineChoose);
public:

			uiSeis2DLineChoose(uiParent*,
					OD::ChoiceMode cm=OD::ChooseOnlyOne);
			~uiSeis2DLineChoose();

    void		getChosen(TypeSet<Pos::GeomID>&) const;
    void		getChosen(BufferStringSet&) const;
    void		setChosen(const TypeSet<Pos::GeomID>&);
    void		setChosen(const BufferStringSet&);
    void		chooseAll(bool yn=true);

protected:

    BufferStringSet	lnms_;
    TypeSet<Pos::GeomID> geomids_;

    uiListBox*		listfld_;
    uiListBoxFilter*	filtfld_;
    uiListBoxChoiceIO*	lbchoiceio_;

    ObjectSet<uiIOObjInserter>	inserters_;
    ObjectSet<uiButton>		insertbuts_;

    void		readChoiceDone(CallBacker*);
    void		writeChoiceReq(CallBacker*);
    void		objInserted(CallBacker*);

    friend class	uiSeis2DLineSel;
			uiSeis2DLineChoose(uiParent*,OD::ChoiceMode,
			    const BufferStringSet&,const TypeSet<Pos::GeomID>&);

    void		init(OD::ChoiceMode);
};


mExpClass(uiSeis) uiSeis2DLineSel : public uiCompoundParSel
{ mODTextTranslationClass(uiSeis2DLineSel);

public:

			uiSeis2DLineSel(uiParent*,bool multisel=false);

    bool		inputOK(bool emit_uimsg=true) const;

    const char*		lineName() const;
    Pos::GeomID		geomID() const;
    void		setSelLine(const char* lnm);
    void		setSelLine(Pos::GeomID);

    void		getSelGeomIDs(TypeSet<Pos::GeomID>&) const;
    void		getSelLineNames(BufferStringSet&) const;

    void		setSelGeomIDs(const TypeSet<Pos::GeomID>&);
    void		setSelLineNames(const BufferStringSet&);

    virtual void	setInput(const MultiID&);
    virtual void	setInput(const BufferStringSet& lnms);
    virtual void	setInput(const TypeSet<Pos::GeomID>& geomid);

    void		clearSelection();
    int			nrSelected() const;

    virtual void	setAll(bool yn);
    virtual bool	isAll() const;

    Notifier<uiSeis2DLineSel>	selectionChanged;

protected:

    BufferStringSet	lnms_;
    TypeSet<Pos::GeomID> geomids_;
    TypeSet<int>	selidxs_;
    bool		ismultisel_;

    BufferString	getSummary() const;

    virtual void	selPush(CallBacker*);
    virtual void	clearAll();
};


mExpClass(uiSeis) uiSeis2DLineNameSel : public uiGroup
{ mODTextTranslationClass(uiSeis2DLineNameSel);
public:

			uiSeis2DLineNameSel(uiParent*,bool forread);

    const char*		getInput() const;
    Pos::GeomID		getInputGeomID() const;
    void		setInput(const char*);
    void		setDataSet(const MultiID&); //!< Only when forread
    void		fillWithAll();

    Notifier<uiSeis2DLineNameSel>	nameChanged;

protected:

    uiComboBox*		fld_;
    bool		forread_;
    MultiID		dsid_;

    void		addLineNames(const MultiID&);
    void		selChg( CallBacker* )	{ nameChanged.trigger(); }

};


mExpClass(uiSeis) uiSeis2DMultiLineSel : public uiSeis2DLineSel
{ mODTextTranslationClass(uiSeis2DMultiLineSel);
public:
			uiSeis2DMultiLineSel(uiParent*,
				 const uiString& text=uiStrings::sEmptyString(),
				 bool withz=false,bool withstep=false);
			~uiSeis2DMultiLineSel();

    bool		isAll() const;

    void		getZRanges(TypeSet<StepInterval<float> >&) const;
    void		getTrcRanges(TypeSet<StepInterval<int> >&) const;
    StepInterval<float> getZRange(Pos::GeomID) const;
    StepInterval<int>	getTrcRange(Pos::GeomID) const;

    void		setInput(const MultiID&);
    void		setInput(const BufferStringSet& lnms);
    void		setInput(const TypeSet<Pos::GeomID>& geomid);

    void		setSelLines(const BufferStringSet&);
    void		setAll(bool);
    void		setZRanges(const TypeSet<StepInterval<float> >&);
    void		setTrcRanges(const TypeSet<StepInterval<int> >&);
    void		setSingleLine(bool);

    bool		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

protected:

    TypeSet<StepInterval<float> > zrgs_;
    TypeSet<StepInterval<int> >	trcrgs_;

    TypeSet<StepInterval<float> > maxzrgs_;
    TypeSet<StepInterval<int> > maxtrcrgs_;

    bool		isall_;
    bool		withstep_;
    bool		withz_;

    void		clearAll();
    void		initRanges(const MultiID* datasetid=0);

    void		selPush(CallBacker*);
};

