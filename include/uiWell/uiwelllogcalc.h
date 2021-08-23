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

class uiUnitSel;
class uiGenInput;
class uiCheckBox;
class uiComboBox;
class uiMathFormula;
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

protected:

    uiMathFormula*		formfld_;
    uiGenInput*			nmfld_;
    uiGenInput*			srfld_;
    uiCheckBox*			ftbox_;
    uiUnitSel*			outunfld_;
    uiComboBox*			interppolfld_;

    Well::LogSet&		superwls_;
    Math::Formula&		form_;
    float			zsampintv_;
    BufferStringSet		lognms_;
    const TypeSet<MultiID>	wellids_;
    bool			havenew_;

    struct InpData
    {
			InpData( const Well::Log* w=0, bool n=false )
			    : wl_(w), shift_(0), specidx_(-1)
			    , isconst_(false), constval_(0)	{}
	bool		operator ==( const InpData& id ) const
			{ return wl_ == id.wl_; }
	const Well::Log* wl_;
	int		shift_;
	int		specidx_;
	bool		isconst_;
	float		constval_;
    };

    void		getAllLogs();
    bool		useForm(const TypeSet<Mnemonic::StdType>* t=nullptr);
    Well::Log*		getLog4InpIdx(Well::LogSet&,int varnr);
    void		setUnits4Log(int);
    void		fillSRFld(int);

    bool		getInpDatas(Well::LogSet&,TypeSet<InpData>&);
    Well::Log*		getInpLog(Well::LogSet&,int,bool);
    bool		calcLog(Well::Log&,const TypeSet<InpData>&,
				Well::Track&,Well::D2TModel*);

    void		initWin(CallBacker*);
    void		rockPhysReq(CallBacker*);
    void		feetSel(CallBacker*);
    void		formSet(CallBacker*);
    void		formUnitSel(CallBacker*);
    void		inpSel(CallBacker*);
    void		vwLog(CallBacker*);

    bool		acceptOK(CallBacker*);

};


