#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          January 2008
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "attribdescid.h"
#include "uibatchprocdlg.h"
#include "dbkey.h"

class NLAModel;
class uiAttrSel;
class uiBatchJobDispatcherSel;

namespace Attrib { class DescSet; }

/*! \brief
Brief Base class Earth Model Output Batch dialog.
Used for calculating attributes in relation with surfaces
*/


mExpClass(uiEMAttrib) uiAttrEMOut : public uiBatchProcDlg
{ mODTextTranslationClass(uiAttrEMOut)
public:
			~uiAttrEMOut();

    void		updateAttributes(const Attrib::DescSet& descset,
					 const NLAModel* nlamodel,
					 const DBKey& nlaid );
    void		getDescNames(BufferStringSet&) const;

protected:
    			uiAttrEMOut(uiParent*,const Attrib::DescSet&,
				    const NLAModel*,const DBKey&,const char*);

    virtual void	attribSel(CallBacker*)		=0;
    virtual bool	prepareProcessing();
    virtual bool	fillPar(IOPar&);
    bool		addNLA(Attrib::DescID&);
    void		fillOutPar(IOPar&,const char* outtyp,
	    			   const char* idlbl,const char* outid);
    Attrib::DescSet*	getTargetDescSet(TypeSet<Attrib::DescID>&,
					 const char* outputnm);

    Attrib::DescSet*	ads_;
    DBKey		nlaid_;
    Attrib::DescID	nladescid_;
    const NLAModel*	nlamodel_;
    BufferStringSet	outdescnms_;
    TypeSet<Attrib::DescID> outdescids_;

    uiAttrSel*		attrfld_;
};
