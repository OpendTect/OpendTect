#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uigroup.h"

namespace EM { class Horizon; class SurfaceIODataSelection; }

class uiCheckBox;
class uiGenInput;
class uiIOObjSel;
class uiPosSubSel;

/*!\brief save or overwrite horizon field set up. It will create new horizon
    based on given horizon, if the old horizon is not given, you can read it
    from memory. You can also call saveHorizon() to save horizon based on your
    choice of as new or overwrite. */

mExpClass(uiEarthModel) uiHorSaveFieldGrp : public uiGroup
{
mODTextTranslationClass(uiHorSaveFieldGrp);
public:
				uiHorSaveFieldGrp(uiParent*,EM::Horizon*,
						  bool is2d=false);
				uiHorSaveFieldGrp(uiParent*,EM::Horizon*,
						  bool is2d, bool wthsubsel);
				~uiHorSaveFieldGrp();

    void			setSaveFieldName(const char*);
    bool			displayNewHorizon() const;
    bool			overwriteHorizon() const;
    void			allowOverWrite(bool);
    EM::Horizon*		getNewHorizon() const	{ return newhorizon_; }

    EM::Horizon*		readHorizon(const MultiID&);
    bool			saveHorizon();

    void			setHorRange(const Interval<int>& newinlrg,
					    const Interval<int>& newcrlrg);
    void			setFullSurveyArray(bool yn);
    bool			needsFullSurveyArray() const;
    bool			acceptOK(CallBacker*);

protected:

    uiGenInput*			savefld_;
    uiCheckBox*			addnewfld_;
    uiIOObjSel*			outputfld_;
    uiPosSubSel*		rgfld_;

    EM::Horizon*		horizon_;
    EM::Horizon*		newhorizon_;
    bool			usefullsurvey_;
    bool			is2d_;

    EM::SurfaceIODataSelection	getSelection(bool) const;
    bool			createNewHorizon();
    void			saveCB(CallBacker*);
    void			expandToFullSurveyArray();
    void			init(bool);
};
