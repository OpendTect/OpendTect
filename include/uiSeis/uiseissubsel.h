#ifndef uiseissubsel_h
#define uiseissubsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2004
 RCS:           $Id: uiseissubsel.h,v 1.16 2006-09-21 17:47:52 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uicompoundparsel.h"
#include "uidialog.h"
#include "ranges.h"
class IOPar;
class IOObj;
class uiGenInput;
class HorSampling;
class CubeSampling;
class uiBinIDSubSel;
class uiSeis2DSubSel;
class BufferStringSet;


class uiSeisSubSel : public uiGroup
{
public:

    			uiSeisSubSel(uiParent*,bool for_new_entry=false,
				     bool withstep=true,bool multi2dlnes=false);

    bool		is2D() const		{ return is2d_; }
    void		set2D( bool yn )	{ is2d_ = yn; typChg(0); }

    void		clear();
    void		setInput(const HorSampling&);
    void		setInput(const StepInterval<float>&);
    void		setInput(const CubeSampling&);
    void		setInput(const IOObj&);
    bool		isAll() const;
    bool		getSampling(HorSampling&) const;
    bool		getZRange(StepInterval<float>&) const;

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;

    int			expectedNrSamples() const;
    int			expectedNrTraces() const;

    void		notifySing2DLineSel(const CallBack&);
    bool		isSing2DLine() const;
    const char*		selectedLine() const;
    void		setSelectedLine(const char*);

    uiSeis2DSubSel*	sel2D()			{ return sel2d; }
    			//!< Can be null
    uiBinIDSubSel*	sel3D()			{ return sel3d; }
    			//!< Can be null

protected:

    bool		is2d_;
    uiSeis2DSubSel*	sel2d;
    uiBinIDSubSel*	sel3d;

    void		typChg(CallBacker*);

};


class uiSeis2DSubSel : public uiCompoundParSel
{ 	
public:

			uiSeis2DSubSel(uiParent*,bool for_new_entry,bool mln);
			~uiSeis2DSubSel();

    class PosData
    {
    public:
				PosData()	{ clear(); }

	bool			isall_;
	StepInterval<int>	trcrg_;
	StepInterval<float>	zrg_;

	void			clear();
	void			fillPar(IOPar&) const;
	void			usePar(const IOPar&);
	int			expectedNrTraces() const;
	int 			expectedNrSamples() const;
    };

    void		clear();
    void		setInput(const PosData&);
    void		setInput(const IOObj&);
    const PosData&	getInput() const		{ return data_; }

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;

    Notifier<uiSeis2DSubSel> lineSel;
    Notifier<uiSeis2DSubSel> singLineSel;
    bool		isSingLine() const;
    const char*		selectedLine() const;
    void		setSelectedLine(const char*);

    const BufferStringSet& curLineNames() const		{ return curlnms_; }

protected:

    uiGenInput*		lnmfld;
    uiGenInput*		lnmsfld;

    PosData		data_;
    bool		multiln_;
    BufferStringSet&	curlnms_;

    void		lineChg(CallBacker*);
    void		singLineChg(CallBacker*);
    void		doDlg(CallBacker*);
    void		updSumm(CallBacker*)		{ updateSummary(); }

    BufferString	getSummary() const;

};


class uiSeis2DSubSelDlg : public uiDialog
{
public:

    			uiSeis2DSubSelDlg(uiParent*);
	
    void		setInput(const uiSeis2DSubSel::PosData&);
    const uiSeis2DSubSel::PosData& getInput() const;

protected:

    uiGenInput*		selfld;
    uiGenInput*		trcrgfld;
    uiGenInput*		zrgfld;
    mutable uiSeis2DSubSel::PosData data_;
    friend class	uiSeis2DSubSel;

    void		selChg(CallBacker* c=0);
    bool		acceptOK(CallBacker*);
};


#endif
