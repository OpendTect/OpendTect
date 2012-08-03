#ifndef uiseissubsel_h
#define uiseissubsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          June 2004
 RCS:           $Id: uiseissubsel.h,v 1.33 2012-08-03 13:01:09 cvskris Exp $
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "bufstringset.h"
#include "seisselection.h"
#include "uidialog.h"
#include "uigroup.h"
#include "ranges.h"
#include "sets.h"
class IOPar;
class IOObj;
class MultiID;
class CtxtIOObj;
class HorSampling;
class CubeSampling;

class uiCompoundParSel;
class uiCheckBox;
class uiLineSel;
class uiListBox;
class uiPosSubSel;
class uiSeis2DSubSel;
class uiSeisSel;
class uiSelSubline;
class uiSeis2DLineNameSel;


mClass(uiSeis) uiSeisSubSel : public uiGroup
{
public:

    static uiSeisSubSel* get(uiParent*,const Seis::SelSetup&);
    virtual		~uiSeisSubSel()					{}

    bool		isAll() const;
    void		getSampling(CubeSampling&) const;
    void		getSampling(HorSampling&) const;
    void		getZRange(StepInterval<float>&) const;

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    virtual void	clear();
    virtual void	setInput(const IOObj&)				= 0;
    void		setInput(const HorSampling&);
    void		setInput(const MultiID&);
    void		setInput(const StepInterval<float>& zrg);
    void		setInput(const CubeSampling&);

    int			expectedNrSamples() const;
    int			expectedNrTraces() const;

    virtual uiCompoundParSel*	compoundParSel();

protected:

    			uiSeisSubSel(uiParent*,const Seis::SelSetup&);

    uiPosSubSel*	selfld_;

};


mClass(uiSeis) uiSeis3DSubSel : public uiSeisSubSel
{
public:

    			uiSeis3DSubSel( uiParent* p, const Seis::SelSetup& ss )
			    : uiSeisSubSel(p,ss)		{}

    void		setInput(const IOObj&);

};


mClass(uiSeis) uiSeis2DSubSel : public uiSeisSubSel
{ 	
public:

			uiSeis2DSubSel(uiParent*,const Seis::SelSetup&);
			~uiSeis2DSubSel();

    virtual void	clear();
    bool		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
    void		setInput(const IOObj&);
    void		setInputWithAttrib(const IOObj&,const char* attribnm);

    Notifier<uiSeis2DSubSel> lineSel;
    Notifier<uiSeis2DSubSel> singLineSel;
    bool		isSingLine() const;
    const char*		selectedLine() const;
    void		setSelectedLine(const char*);

    const BufferStringSet& curLineNames() const		{ return curlnms_; }

protected:

    uiSeis2DLineNameSel* lnmfld_;
    uiCheckBox*		onelnbox_;

    bool		multiln_;
    BufferStringSet&	curlnms_;

    TypeSet<StepInterval<int> >		trcrgs_;
    TypeSet<StepInterval<float> >	zrgs_;

    void		lineChg(CallBacker*);
    void		singLineChg(CallBacker*);

};

#endif

