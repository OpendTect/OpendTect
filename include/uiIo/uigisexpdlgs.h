#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"

#include "uidialog.h"
#include "uigisexp.h"

#include "pickset.h"

class SurveyInfo;
class uiGenInput;
class uiGISExpStdFld;
class uiIOObjSelGrp;
class uiListBox;
class uiSelLineStyle;


mExpClass(uiIo) uiGISExportSurvey : public uiDialog
{
mODTextTranslationClass(uiGISExportSurvey);
public:

			uiGISExportSurvey(uiParent*,const SurveyInfo&);
			~uiGISExportSurvey();

    const SurveyInfo&	getSI() const	{ return si_; }

private:

    bool		acceptOK(CallBacker*) override;

    uiSelLineStyle*	lsfld_;
    uiGenInput*		hghtfld_;
    uiGISExpStdFld*	expfld_;

    const SurveyInfo&	si_;
};


mExpClass(uiIo) uiGISExportDlg : public uiDialog
{
mODTextTranslationClass(uiGISExportDlg);
public:

    enum class Type	{ PointSet, Polygon, RandomLine, Line2D };
			mDeclareEnumUtils(Type);

			uiGISExportDlg(uiParent*,Type,
				       const ObjectSet<const Pick::Set>&);
			~uiGISExportDlg();

    void		set(const ObjectSet<const Pick::Set>&);

    static const char*	sKeyIsOn()	{ return "IsOn"; }

private:

    void		fillNames();
    bool		acceptOK(CallBacker*) override;

    uiListBox*		selfld_;
    uiSelLineStyle*	lsfld_		= nullptr;
    uiGISExpStdFld*	expfld_;

    const Type		typ_;
    RefObjectSet<const Pick::Set> data_;

    static uiString	getDlgTitle(Type);
    static int		getHelpKey(Type);
};
