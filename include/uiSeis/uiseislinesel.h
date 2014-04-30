#ifndef uiseislinesel_h
#define uiseislinesel_h

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

#include "bufstring.h"
#include "bufstringset.h"
#include "multiid.h"
#include "ranges.h"
#include "posinfo2dsurv.h"

class uiComboBox;
class uiListBox;
class uiSeisSel;
class uiSelNrRange;
class uiSelZRange;

class CtxtIOObj;
class IOObj;


mExpClass(uiSeis) uiSeis2DLineSel : public uiCompoundParSel
{
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

    void		setInput(const MultiID&);
    void		setInput(const BufferStringSet& lnms);
    void		setInput(const TypeSet<Pos::GeomID>& geomid);

    void		clearSelection();
    void		setAll(bool yn);
    bool		isAll() const;
    int			nrSelected() const;
    
    Notifier<uiSeis2DLineSel>	selectionChanged;

protected:

    BufferStringSet	lnms_;
    TypeSet<Pos::GeomID>	geomids_;
    TypeSet<int>	selidxs_;
    bool		ismultisel_;
    
    BufferString	getSummary() const;

    virtual void	selPush(CallBacker*);
    virtual void	clearAll();
};


mExpClass(uiSeis) uiSeis2DLineNameSel : public uiGroup
{
public:

			uiSeis2DLineNameSel(uiParent*,bool forread);

    const char*		getInput() const;
    void		setInput(const char*);
    void		setDataSet(const MultiID&); //!< Only when forread
    void		fillWithAll();

    int			getLineIndex() const; //!< Only usable when forread

    Notifier<uiSeis2DLineNameSel>	nameChanged;

protected:

    uiComboBox*		fld_;
    bool		forread_;
    MultiID		dsid_;

    void		addLineNames(const MultiID&);
    void		selChg( CallBacker* )	{ nameChanged.trigger(); }
    void		fillAll(CallBacker*);

};


mExpClass(uiSeis) uiSeis2DMultiLineSel : public uiSeis2DLineSel
{
public:
			uiSeis2DMultiLineSel(uiParent*,const char* text=0,
					  bool withz=false,bool withstep=false);
			~uiSeis2DMultiLineSel();

    bool		isAll() const;

    const TypeSet<StepInterval<float> >& getZRanges() const;
    const TypeSet<StepInterval<int> >&	getTrcRanges() const;

    void		setSelLines(const BufferStringSet&);
    void		setAll(bool);
    void		setZRanges(const TypeSet<StepInterval<float> >&);
    void		setTrcRanges(const TypeSet<StepInterval<int> >&);

    bool		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

protected:

    TypeSet<StepInterval<float> > zrgs_;
    TypeSet<StepInterval<int> >	trcrgs_;

    bool		isall_;
    bool		withstep_;
    bool		withz_;


    void		selPush(CallBacker*);
};

#endif

