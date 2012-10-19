#ifndef uirockphysform_h
#define uirockphysform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "propertyref.h"
class uiComboBox;
class uiGenInput;
class uiLabel;
class uiPushButton;
class uiTextEdit;
class uiRockPhysCstFld;


mClass(uiTools) uiRockPhysForm : public uiGroup
{
public:

			uiRockPhysForm(uiParent*);
			uiRockPhysForm(uiParent*,PropertyRef::StdType);

    PropertyRef::StdType getType() const;
    void		setType(PropertyRef::StdType);
    				//!< only works when 1st constructor used
    const char*		formulaName() const;
    void		setFormulaName(const char*);

    bool		getFormulaInfo(BufferString&,BufferString&,
				       BufferString&,BufferStringSet&,
				       bool) const;
    BufferString	getText(bool usecstevals) const;
    const char*		errMsg() const		{ return errmsg_.buf(); }
    bool		isOK();

protected:

    uiComboBox*		typfld_;
    uiComboBox*		nmfld_;
    uiTextEdit*		formulafld_;
    uiTextEdit*		descriptionfld_;
    const PropertyRef::StdType fixedtype_;

    void		typSel(CallBacker*);
    void		nameSel(CallBacker*);

    void		createFlds(uiObject*);

    ObjectSet<uiRockPhysCstFld>	cstflds_;

    BufferString	errmsg_;
};


mClass(uiTools) uiRockPhysCstFld : public uiGroup
{
public:

			uiRockPhysCstFld(uiParent*);

    float		getCstVal() const;
    void		updField(BufferString,Interval<float>,BufferString,
	    			 float val = mUdf(float));

    const char*		cstnm_;


protected:

    void		descPush(CallBacker*);

    uiGenInput*		valfld_;
    uiLabel*		nmlbl_;
    uiLabel*		rangelbl_;
    uiPushButton*	descbutton_;
    BufferString	desc_;
};


#endif

