#ifndef uiiosurface_h
#define uiiosurface_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurface.h,v 1.8 2003-10-29 17:28:48 nanne Exp $
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
class uiObject;
class BufferStringSet;

namespace EM { class Horizon; class SurfaceIODataSelection; };


/*! \brief Base group for Surface input and output */

class uiIOSurface : public uiGroup
{
public:
			~uiIOSurface();

    IOObj*		selIOObj() const;
    void		getSelection(EM::SurfaceIODataSelection&);

    virtual bool	processInput()		{ return true; };

protected:
			uiIOSurface(uiParent*);

    void		fillFields(const MultiID&);
    void		fillPatchFld(const BufferStringSet&);
    void		fillAttribFld(const BufferStringSet&);
    void		fillRangeFld(const BinIDSampler&);

    void		mkAttribFld();
    void		mkPatchFld(bool);
    void		mkRangeFld();
    void		mkObjFld(const char*,bool);

    void		deSelect(CallBacker*);
    void		objSel(CallBacker*);

    uiLabeledListBox*	patchfld;
    uiLabeledListBox*	attribfld;
    uiBinIDSubSel*	rgfld;
    uiIOObjSel*		objfld;

    CtxtIOObj&		ctio;
};


class uiSurfaceWrite : public uiIOSurface
{
public:
			uiSurfaceWrite(uiParent*,const EM::Horizon&);

    const char*		auxDataName() const;
    bool		saveAuxDataOnly() const;
    bool		surfaceOnly() const;
    bool		surfaceAndData() const;

    virtual bool	processInput();

protected:
    void		savePush(CallBacker*);

    uiGenInput*		attrnmfld;
    uiGenInput*		savefld;
};


class uiSurfaceRead : public uiIOSurface
{
public:
    			uiSurfaceRead(uiParent*,bool showattribfld=true);
			~uiSurfaceRead();

    virtual bool	processInput();

protected:


};


#endif
