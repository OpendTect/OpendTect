#ifndef uiiosurface_h
#define uiiosurface_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurface.h,v 1.1 2003-07-16 09:56:15 nanne Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class uiBinIDSubSel;
class uiGenInput;
class uiIOObjSel;
class uiLabeledListBox;
class BinIDSampler;
class CtxtIOObj;
class IODirEntryList;
class IOObj;
class MultiID;

namespace EM { class Horizon; class SurfaceIODataSelection; };


/*! \brief Dialog for horizon export */

class uiIOSurface : public uiGroup
{
public:
			uiIOSurface(uiParent*,CtxtIOObj&);
			uiIOSurface(uiParent*,const EM::Horizon*);
			~uiIOSurface();

    IOObj*		selIOObj() const;
    void		getSelection(EM::SurfaceIODataSelection&);

protected:

    uiLabeledListBox*	objlistfld;
    uiLabeledListBox*	patchfld;
    uiLabeledListBox*	attrlistfld;
    uiBinIDSubSel*	rgfld;
    uiIOObjSel*		outfld;
    uiGenInput*		attrnmfld;
    uiGenInput*		savefld;
    IODirEntryList*	entrylist;

    uiGroup*		readgrp;
    uiGroup*		writegrp;

    CtxtIOObj&		ctio;
    const EM::Horizon*	hor;


    bool		anyHorWithPatches();
    void		createSharedFields(bool);
    void		fillFields(const MultiID&);
    void		fillPatchField(ObjectSet<BufferString>);
    void		fillAttrField(ObjectSet<BufferString>);
    void		fillRangeField(const BinIDSampler&);
    void		selChg(CallBacker*);
};


#endif
