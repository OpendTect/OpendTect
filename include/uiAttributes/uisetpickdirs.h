#ifndef uisetpickdirs_h
#define uisetpickdirs_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Dec 2003
 RCS:           $Id: uisetpickdirs.h,v 1.1 2005-06-09 13:12:35 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uidialog.h"

namespace Attrib { class DescSet; class SelInfo; };

class Coord3;
class PickSet;
class NLAModel;
class CtxtIOObj;
class uiAttrSel;
class FeatureSet;
class uiGenInput;
class BinIDValueSet;
class uiSteerCubeSel;

/*! \brief */

class uiSetPickDirs : public uiDialog
{
public:
    				uiSetPickDirs(uiParent*,PickSet&,
					      const Attrib::DescSet* a=0,
					      const NLAModel* n=0);
				~uiSetPickDirs();

protected:

    PickSet&			ps;
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
    bool			getNLAIds(TypeSet<int>&);
    int				getAttribID(uiAttrSel*,const TypeSet<int>&);
    FeatureSet* 		calcAttribs(const BinIDValueSet&,
	    				    const BoolTypeSet&);

    float			calcPhi(float,float);
    float			calcTheta(float,float);
    void			wrapPhi(float&);
    void			wrapTheta(float&);

    bool			usesteering;
};


#endif
