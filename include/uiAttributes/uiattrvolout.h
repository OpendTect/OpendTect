#ifndef uiattrvolout_h
#define uiattrvolout_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiattrvolout.h,v 1.2 2005-07-29 13:08:11 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"
#include "multiid.h"

namespace Attrib { class CurrentSel; class DescID; class DescSet; };
class IOObj;
class IOPar;
class NLAModel;
class CtxtIOObj;
class uiSeisSel;
class uiAttrSel;
class uiIOObjSel;
class uiSeisTransfer;


/*! \brief Dialog for creating volume output */

class uiAttrVolOut : public uiFullBatchDialog
{
public:
			uiAttrVolOut(uiParent*,const Attrib::DescSet&,
				     const NLAModel* n=0,MultiID i=0);
			~uiAttrVolOut();

    const IOPar&	subSelPar() const		{ return subselpar; }
    const Attrib::CurrentSel& outputSelection() const	{ return sel; }

protected:

    CtxtIOObj&		ctio;
    Attrib::CurrentSel&	sel;
    IOPar&		subselpar;
    Attrib::DescSet& 	ads;
    MultiID		nlaid;
    const NLAModel*	nlamodel;

    uiAttrSel*		todofld;
    uiSeisTransfer*	transffld;
    uiSeisSel*		objfld;

    bool		prepareProcessing();
    bool		fillPar(IOPar&);

    void		attrSel(CallBacker*);
    void		singLineSel(CallBacker*);
    void		addNLA(Attrib::DescID&);

private:

    CtxtIOObj&		mkCtxtIOObj();

};

#endif
