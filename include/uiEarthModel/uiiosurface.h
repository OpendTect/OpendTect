#ifndef uiiosurface_h
#define uiiosurface_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurface.h,v 1.14 2004-12-17 12:31:09 bert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class uiBinIDSubSel;
class uiGenInput;
class uiIOObjSel;
class uiLabeledListBox;
class CtxtIOObj;
class HorSampling;
class IODirEntryList;
class IOObj;
class MultiID;
class uiObject;
class BufferStringSet;

namespace EM { class Surface; class SurfaceIODataSelection; };


/*! \brief Base group for Surface input and output */

class uiIOSurface : public uiGroup
{
public:
			~uiIOSurface();

    IOObj*		selIOObj() const;
    void		getSelection(EM::SurfaceIODataSelection&);

    virtual bool	processInput()		{ return true; };

    Notifier<uiIOSurface> attrSelChange;
    bool		haveAttrSel() const;

protected:
			uiIOSurface(uiParent*,bool ishor);

    void		fillFields(const MultiID&);
    void		fillSectionFld(const BufferStringSet&);
    void		fillAttribFld(const BufferStringSet&);
    void		fillRangeFld(const HorSampling&);

    void		mkAttribFld();
    void		mkSectionFld(bool);
    void		mkRangeFld();
    void		mkObjFld(const char*,bool);

    void		objSel(CallBacker*);
    void		attrSel(CallBacker*);

    uiLabeledListBox*	sectionfld;
    uiLabeledListBox*	attribfld;
    uiBinIDSubSel*	rgfld;
    uiIOObjSel*		objfld;

    CtxtIOObj&		ctio;
};


class uiSurfaceWrite : public uiIOSurface
{
public:
			uiSurfaceWrite(uiParent*,const EM::Surface&,bool);

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
    			uiSurfaceRead(uiParent*,bool,bool showattribfld=true);
			~uiSurfaceRead();

    virtual bool	processInput();

protected:


};


#endif
