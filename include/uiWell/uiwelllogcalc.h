#ifndef uiwelllogcalc_h
#define uiwelllogcalc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2009
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uidialog.h"
#include "bufstringset.h"
#include "multiid.h"
#include "propertyref.h"

class uiUnitSel;
class uiGenInput;
class uiCheckBox;
class uiComboBox;
class uiMathExpression;
class uiWellLogCalcInpData;
namespace Math { class Formula; }
namespace Well { class D2TModel; class Log; class LogSet; class Track;}


/*! \brief Dialog for marker specifications */

mExpClass(uiWell) uiWellLogCalc : public uiDialog
{
public:
				uiWellLogCalc(uiParent*,const TypeSet<MultiID>&,
					      bool rockphysmode=false);
				~uiWellLogCalc();

    bool			haveNewLogs() const	{ return havenew_; }

    void			setOutputLogName(const char* nm);
    const char*			getOutputLogName() const;

protected:

    uiMathExpression*		formfld_;
    uiGenInput*			nmfld_;
    uiGenInput*			srfld_;
    uiCheckBox*			ftbox_;
    uiUnitSel*			formulaunfld_;
    uiUnitSel*			outunfld_;
    ObjectSet<uiWellLogCalcInpData> inpdataflds_;
    bool			mywelllogs_;

    Math::Formula&		form_;
    bool			havenew_;
    float			zsampintv_;
    BufferStringSet		lognms_;
    Well::LogSet&		wls_;
    const TypeSet<MultiID>	wellids_;
    BufferStringSet		inputunits_;
    TypeSet<PropertyRef::StdType> inputtypes_;

    friend class		uiWellLogCalcInpData;

    struct InpData
    {
			InpData( const Well::Log* w=0, int s=0, bool n=false )
			    : wl_(w), shift_(s), noudf_(n), specidx_(-1)
			    , isconst_(false), constval_(0)	{}
	bool		operator ==( const InpData& id ) const
			{ return shift_ == id.shift_ && wl_ == id.wl_; }
	const Well::Log* wl_;
	int		shift_;
	bool		noudf_;
	int		specidx_;
	bool		isconst_;
	float		constval_;
    };

    bool		checkValidNrInputs(const Math::Formula&) const;
    bool		updateForm(Math::Formula&) const;
    bool		useForm(const Math::Formula&,
				const TypeSet<PropertyRef::StdType>* t=0);
    void		getAllLogs();
    void		setCurWls(const Well::LogSet&);
    bool		getInpData(const Math::Formula&,
				   TypeSet<InpData>&);
    bool		getRecInfo(Math::Formula&);
    bool		calcLog(Well::Log&,const Math::Formula&,
				const TypeSet<InpData>&,
				Well::Track&,Well::D2TModel*);

    void		initWin(CallBacker*);
    void		rockPhysReq(CallBacker*);
    void		feetSel(CallBacker*);
    void		formSet(CallBacker*);
    void		formUnitSel(CallBacker*);
    void		inpSel(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif
