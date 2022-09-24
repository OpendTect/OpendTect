#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"
#include "attribdescid.h"

namespace Attrib { class DescSet; };

class NLAModel;
class uiAttrSel;
class uiGenInput;
class DataPointSet;
class uiSteerAttrSel;
namespace Pick { class Set; }

/*! \brief */

mExpClass(uiAttributes) uiSetPickDirs : public uiDialog
{ mODTextTranslationClass(uiSetPickDirs);
public:
    				uiSetPickDirs(uiParent*,Pick::Set&,
					      const Attrib::DescSet* a=0,
					      const NLAModel* n=0,
					      float vel=0);
				~uiSetPickDirs();

protected:

    Pick::Set&			ps_;
    const Attrib::DescSet*	ads_;
    Attrib::DescSet*		createdset_;
    const NLAModel*		nlamdl_;
    bool			usesteering_;

    uiAttrSel*			phifld_;
    uiAttrSel*			thetafld_;
    uiGenInput*			dirinpfld_;
    uiSteerAttrSel*		steerfld_;
    float			velocity_;

    void			dirinpSel(CallBacker*);

    bool			acceptOK(CallBacker*) override;
    bool			getAndCheckAttribSelection(DataPointSet&);
    bool			extractDipOrAngl(DataPointSet&);
    void			createSteeringDesc(int,const Attrib::DescID&);
    bool			getNLAIds(TypeSet<Attrib::DescID>&);
    Attrib::DescID		getAttribID(uiAttrSel*,
	    				    const TypeSet<Attrib::DescID>&);

    float			calcPhi(float,float);
    float			calcTheta(float,float);
    void			wrapPhi(float&);
    void			wrapTheta(float&);
};
