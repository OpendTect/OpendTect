#ifndef uiiosurface_h
#define uiiosurface_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurface.h,v 1.2 2003-07-29 13:03:19 nanne Exp $
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


/*! \brief Base group for Surface input and output */

class uiIOSurface : public uiGroup
{
public:
			uiIOSurface(uiParent*);
			~uiIOSurface();

    IOObj*		selIOObj() const;
    void		getSelection(EM::SurfaceIODataSelection&);

protected:

    uiLabeledListBox*	patchfld;
    uiLabeledListBox*	attrlistfld;
    uiBinIDSubSel*	rgfld;

    CtxtIOObj&		ctio;

    void		fillFields(const MultiID&);
    void		fillPatchFld(ObjectSet<BufferString>);
    void		fillAttribFld(ObjectSet<BufferString>);
    void		fillRangeFld(const BinIDSampler&);

    void		mkAttribFld();
    void		mkPatchFld();
    void		mkRangeFld();

};


class uiSurfaceOutSel : public uiIOSurface
{
public:
			uiSurfaceOutSel(uiParent*,const EM::Horizon&);

    void		processInput();
    const char*		auxDataName() const;
    bool		saveAuxDataOnly() const;

protected:
    void		savePush(CallBacker*);

    uiGenInput*		attrnmfld;
    uiGenInput*		savefld;
    uiIOObjSel*		outfld;
};


class uiSurfaceAuxSel : public uiIOSurface
{
public:
    			uiSurfaceAuxSel(uiParent*,const MultiID&);
};


class uiSurfaceSel : public uiIOSurface
{
public:
    			uiSurfaceSel(uiParent*);
			~uiSurfaceSel();

protected:
    void		selChg(CallBacker*);
    bool		anyHorWithPatches();

    IODirEntryList*	entrylist;
    uiLabeledListBox*	objlistfld;

};


#endif
