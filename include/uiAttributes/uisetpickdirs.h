#ifndef uisetpickdirs_h
#define uisetpickdirs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Dec 2003
 RCS:           $Id$
________________________________________________________________________

-*/


#include "uiattributesmod.h"
#include "uidialog.h"
#include "attribdescid.h"

namespace Attrib { class DescSet; class SelInfo; };

class Coord3;
class NLAModel;
class CtxtIOObj;
class uiAttrSel;
class uiGenInput;
class DataPointSet;
class uiSteerCubeSel;
namespace Pick { class Set; }

/*! \brief */

mExpClass(uiAttributes) uiSetPickDirs : public uiDialog
{
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
    CtxtIOObj*			steerctio_;
    bool			usesteering_;

    uiAttrSel*			phifld_;
    uiAttrSel*			thetafld_;
    uiGenInput*			dirinpfld_;
    uiSteerCubeSel*		steerfld_;
    float			velocity_;

    void			dirinpSel(CallBacker*);

    bool			acceptOK(CallBacker*);
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


#endif

