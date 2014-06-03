#ifndef uiattremout_h
#define uiattremout_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          January 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "attribdescid.h"
#include "uidialog.h"

class NLAModel;
class uiAttrSel;
class uiBatchJobDispatcherSel;

namespace Attrib { class DescSet; }

/*! \brief
Brief Base class Earth Model Output Batch dialog.
Used for calculating attributes in relation with surfaces
*/


mExpClass(uiEMAttrib) uiAttrEMOut : public uiDialog
{ mODTextTranslationClass(uiAttrEMOut);
public:
    			uiAttrEMOut(uiParent*,const Attrib::DescSet&,
				    const NLAModel*,const MultiID&,const char*);
			~uiAttrEMOut()			{};

protected:

    virtual void	attribSel(CallBacker*)		=0;
    virtual bool	prepareProcessing();
    virtual bool	fillPar();
    bool		addNLA(Attrib::DescID&);
    void		fillOutPar(IOPar&,const char* outtyp,
	    			   const char* idlbl,const char* outid);
    bool		acceptOK(CallBacker*);

    Attrib::DescSet&	ads_;
    const MultiID&	nlaid_;
    Attrib::DescID	nladescid_;
    const NLAModel*	nlamodel_;

    uiAttrSel*		attrfld_;
    uiBatchJobDispatcherSel* batchfld_;
};

#endif

