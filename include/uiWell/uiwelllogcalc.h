#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2009
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uidialog.h"

#include "bufstringset.h"
#include "mnemonics.h"
#include "multiid.h"

class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiMathExpressionVariable;
class uiMathFormula;
class uiToolButton;
class uiUnitSel;
namespace Math { class Formula; }
namespace Well { class D2TModel; class Log; class LogSet; class Track;}


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

    Well::LogSet&		superwls_;
    Math::Formula&		form_;
    float			zsampintv_;
    BufferStringSet		lognms_;
    MnemonicSelection		mnsel_;
    TypeSet<MultiID>		wellids_;
    bool			havenew_	= false;

    struct InpData
    {
			InpData( const Well::Log* w=nullptr )
			    : wl_(w)				{}
	bool		operator ==( const InpData& id ) const
			{ return wl_ == id.wl_; }
	const Well::Log* wl_;
	int		shift_ = 0;
	int		specidx_ = -1;
	bool		isconst_ = false;
	double		constval_ = 0.;
    };

    void		getAllLogs();
    bool		useForm();
    Well::Log*		getLog4InpIdx(Well::LogSet&,const char* lognm);
    void		setUnits4Log(const char* lognm,
				     uiMathExpressionVariable&);
    void		fillSRFld(const char* lognm);

    bool		getInpDatas(Well::LogSet&,TypeSet<InpData>&);
    Well::Log*		getInpLog(Well::LogSet&,int);
    bool		calcLog(Well::Log&,const TypeSet<InpData>&,
				Well::Track&,Well::D2TModel*);
    void		deleteLog(TypeSet<InpData>&);

    void		initWin(CallBacker*);
    void		rockPhysReq(CallBacker*);
    void		inpSel(CallBacker*);
    void		formMnSet(CallBacker*);
    void		formUnitSel(CallBacker*);
    void		feetSel(CallBacker*);
    void		vwLog(CallBacker*);
    void		viewOutputCB(CallBacker*);

    bool		acceptOK(CallBacker*);
};

