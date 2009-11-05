#ifndef uihorsavefieldgrp_h
#define uihorsavefieldgrp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Y. Liu
 Date:          Nov 2009
 RCS:           $Id: uihorsavefieldgrp.h,v 1.2 2009-11-05 19:49:48 cvsyuancheng Exp $
________________________________________________________________________

-*/


#include "uigroup.h"

namespace EM { class Horizon3D; }

class MultiID;
class uiGenInput;
class uiIOObjSel;

/*!\brief save or overwrite horizon field set up. It will create new horizon 
    based on given horizon, if the old horizon is not given, you can read it 
    from memory. You can also call saveHorizon() to save horizon based on your
    choice of as new or overwrite. */

mClass uiHorSaveFieldGrp : public uiGroup
{
public:
				uiHorSaveFieldGrp(uiParent*,EM::Horizon3D*);
				~uiHorSaveFieldGrp();

    void			setSaveFieldName(const char*);
    bool			displayNewHorizon() const;
    bool			overwriteHorizon() const;
    EM::Horizon3D*		getNewHorizon() const	{ return newhorizon_; }

    EM::Horizon3D*		readHorizon(const MultiID&);
    bool			saveHorizon();
    
    void			setFullSurveyArray(bool yn);
    bool			needsFullSurveyArray() const;
    bool			acceptOK(CallBacker*);

protected:

    uiGenInput*			savefld_;
    uiGenInput*			addnewfld_;
    uiIOObjSel*			outputfld_;

    EM::Horizon3D*		horizon_;
    EM::Horizon3D*		newhorizon_;
    bool			usefullsurvey_;

    bool			createNewHorizon();
    void			saveCB(CallBacker*);
    void			expandToFullSurveyArray();
};


#endif
