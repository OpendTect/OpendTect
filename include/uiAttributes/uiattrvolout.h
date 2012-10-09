#ifndef uiattrvolout_h
#define uiattrvolout_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id$
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

mClass uiAttrVolOut : public uiFullBatchDialog
{
public:
			uiAttrVolOut(uiParent*,const Attrib::DescSet&,
				     const NLAModel* n=0,MultiID i=0);
			~uiAttrVolOut();

    const IOPar&	subSelPar() const		{ return subselpar; }
    const Attrib::CurrentSel& outputSelection() const	{ return sel; }

    static const char*  sKeyMaxCrlRg();
    static const char*  sKeyMaxInlRg();

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

    TypeSet<int>	seloutputs;
    BufferStringSet	seloutnms;

    bool		prepareProcessing();
    bool		fillPar(IOPar&);

    void		attrSel(CallBacker*);
    void		singLineSel(CallBacker*);
    void		addNLA(Attrib::DescID&);

private:

    CtxtIOObj&		mkCtxtIOObj(const Attrib::DescSet&);

};

#endif
