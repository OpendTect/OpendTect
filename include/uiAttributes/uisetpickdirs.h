#ifndef uisetpickdirs_h
#define uisetpickdirs_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Dec 2003
 RCS:           $Id: uisetpickdirs.h,v 1.4 2006-05-08 16:50:19 cvsbert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
#include "attribdescid.h"

namespace Attrib { class DescSet; class SelInfo; };

class Coord3;
class NLAModel;
class CtxtIOObj;
class uiAttrSel;
class uiGenInput;
class BinIDValueSet;
class uiSteerCubeSel;
namespace Pick { class Set; }

/*! \brief */

class uiSetPickDirs : public uiDialog
{
public:
    				uiSetPickDirs(uiParent*,Pick::Set&,
					      const Attrib::DescSet* a=0,
					      const NLAModel* n=0);
				~uiSetPickDirs();

protected:

    Pick::Set&			ps;
    const Attrib::DescSet*	ads;
    Attrib::DescSet*		createdset;
    const NLAModel*		nlamdl;
    CtxtIOObj*			steerctio;

    uiAttrSel*			phifld;
    uiAttrSel*			thetafld;
    uiAttrSel*			rfld;
    uiGenInput*			dirinpfld;
    uiSteerCubeSel*		steerfld;

    void			dirinpSel(CallBacker*);

    bool			acceptOK(CallBacker*);
    bool			getAttribSelection(BoolTypeSet&,TypeSet<int>&);
    bool			getNLAIds(TypeSet<Attrib::DescID>&);
    Attrib::DescID		getAttribID(uiAttrSel*,
	    				    const TypeSet<Attrib::DescID>&);
    void			calcAttribs(const BinIDValueSet&,
	    				    const BoolTypeSet&);

    float			calcPhi(float,float);
    float			calcTheta(float,float);
    void			wrapPhi(float&);
    void			wrapTheta(float&);

    bool			usesteering;
};


#endif
