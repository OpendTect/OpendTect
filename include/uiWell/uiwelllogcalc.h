#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uidialog.h"

#include "bufstringset.h"
#include "mnemonics.h"
#include "multiid.h"
#include "welldata.h"

class Timer;
class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiMathFormula;
class uiToolButton;
class uiUnitSel;
namespace Math { class Formula; }
namespace Well { class D2TModel; class Log; class LogSet; class Track; }


/*! \brief Dialog for marker specifications */

mExpClass(uiWell) uiWellLogCalc : public uiDialog
{ mODTextTranslationClass(uiWellLogCalc);
public:
				uiWellLogCalc(uiParent*,const TypeSet<MultiID>&,
					      bool rockphysmode=false);
				~uiWellLogCalc();

    bool			haveNewLogs() const	{ return havenew_; }

    void			setOutputLogName(const char* nm);
    const char*			getOutputLogName() const;

    bool			updateWells(const TypeSet<MultiID>&);

    Notifier<uiWellLogCalc>	logschanged;

protected:

    uiMathFormula*		formfld_	= nullptr;
    uiGenInput*			nmfld_		= nullptr;
    uiGenInput*			srfld_		= nullptr;
    uiCheckBox*			ftbox_		= nullptr;
    uiUnitSel*			outunfld_	= nullptr;
    uiComboBox*			interppolfld_	= nullptr;
    uiToolButton*		viewlogbut_	= nullptr;

    RefObjectSet<Well::Data>	wds_;
    Math::Formula&		form_;
    float			zsampintv_	= mUdf(float);
    BufferStringSet		lognms_;
    MnemonicSelection		mnsel_;
    TypeSet<MultiID>		wellids_;
    bool			havenew_	= false;
    bool			rockphysmode_;
    Timer&			timer_;

    mClass(uiWell) InpData
    {
    public:
			InpData();
			~InpData();
			mOD_DisableCopy(InpData);

	BufferString	lognm_;
	const Well::Log* wl_		= nullptr;
	int		shift_		= 0;
	int		specidx_	= -1;
	bool		isconst_	= false;
	double		constval_	= 0.;
    };

    void		getAllLogs();
    bool		useForm();
    const Well::Log*	getFirstLog4InpIdx(const char* lognm) const;
    void		resetTimer();

    bool		getInpDatas(const MultiID& wid,ObjectSet<InpData>&,
				    Well::LoadReqs&,uiString&);
    bool		calcLog(const ObjectSet<InpData>&,
				const Well::Track&,const Well::D2TModel*,
				Well::Log&);

    void		afterPopupCB(CallBacker*);
    void		rockPhysReq(CallBacker*);
    void		formMnSet(CallBacker*);
    void		feetSel(CallBacker*);
    void		vwLog(CallBacker*);
    void		viewOutputCB(CallBacker*);
    void		releaseWDS(CallBacker*);

    bool		acceptOK(CallBacker*) override;
};
