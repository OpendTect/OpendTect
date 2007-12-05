#ifndef uiseissubsel_h
#define uiseissubsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2004
 RCS:           $Id: uiseissubsel.h,v 1.19 2007-12-05 11:55:49 cvsbert Exp $
________________________________________________________________________

-*/

#include "uicompoundparsel.h"
#include "seisselection.h"
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


class uiSeisSubSel
{
public:

    static uiSeisSubSel* get(uiParent*,const Seis::SelSetup&);
    virtual		~uiSeisSubSel()					{}

    virtual bool	isAll() const					= 0;
    virtual void	getSampling(HorSampling&) const			= 0;
    virtual void	getZRange(StepInterval<float>&) const		= 0;
    virtual bool	fillPar(IOPar&) const				= 0;
    virtual int		expectedNrSamples() const			= 0;
    virtual int		expectedNrTraces() const			= 0;

    virtual void	clear()						= 0;

    virtual void	setInput(const IOObj&)				= 0;
    virtual void	setInput(const HorSampling&)			= 0;
    virtual void	setInput(const StepInterval<float>& zrg)	= 0;
    virtual void	setInput(const CubeSampling&);
    virtual void	usePar(const IOPar&)				= 0;

    virtual uiCompoundParSel*	compoundParSel()			= 0;
    virtual uiObject*		attachObj()				= 0;

};


class uiSeis3DSubSel : public uiGroup
		     , public uiSeisSubSel
{
public:

    			uiSeis3DSubSel(uiParent*,const Seis::SelSetup&);

    void		clear();
    void		setInput(const HorSampling&);
    void		setInput(const StepInterval<float>&);
    void		setInput(const CubeSampling&);
    void		setInput(const IOObj&);
    bool		isAll() const;
    void		getSampling(HorSampling&) const;
    void		getZRange(StepInterval<float>&) const;

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;

    int			expectedNrSamples() const;
    int			expectedNrTraces() const;

    uiCompoundParSel*	compoundParSel();
    uiObject*		attachObj()		{ return uiGroup::attachObj(); }

protected:

    uiBinIDSubSel*	selfld;

};


class uiSeis2DSubSel : public uiCompoundParSel
		     , public uiSeisSubSel
{ 	
public:

			uiSeis2DSubSel(uiParent*,const Seis::SelSetup&);
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
    void		setInput(const StepInterval<float>&);
    void		setInput(const HorSampling&);
    const PosData&	getInput() const		{ return data_; }
    void		getSampling(HorSampling&) const;
    void		getZRange( StepInterval<float>& zrg ) const
							{ zrg = data_.zrg_; }
    bool		isAll() const			{ return data_.isall_; }
    int			expectedNrSamples() const
    			{ return data_.expectedNrSamples(); }
    int			expectedNrTraces() const
    			{ return data_.expectedNrTraces(); }

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;

    Notifier<uiSeis2DSubSel> lineSel;
    Notifier<uiSeis2DSubSel> singLineSel;
    bool		isSingLine() const;
    const char*		selectedLine() const;
    void		setSelectedLine(const char*);

    const BufferStringSet& curLineNames() const		{ return curlnms_; }

    uiCompoundParSel*	compoundParSel()		{ return this; }
    uiObject*		attachObj()
    				{ return uiCompoundParSel::attachObj(); }

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
