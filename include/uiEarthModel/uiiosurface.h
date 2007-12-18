#ifndef uiiosurface_h
#define uiiosurface_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurface.h,v 1.21 2007-12-18 14:58:16 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class BufferStringSet;
class CtxtIOObj;
class HorSampling;
class IODirEntryList;
class IOObj;
class MultiID;

class uiBinIDSubSel;
class uiGenInput;
class uiIOObjSel;
class uiLabeledListBox;
class uiCheckBox;


namespace EM { class Surface; class SurfaceIODataSelection; };


/*! \brief Base group for Surface input and output */

class uiIOSurface : public uiGroup
{
public:
			~uiIOSurface();

    IOObj*		selIOObj() const;
    void		getSelection(EM::SurfaceIODataSelection&) const;

    virtual bool	processInput()		{ return true; };

    Notifier<uiIOSurface> attrSelChange;
    bool		haveAttrSel() const;

protected:
			uiIOSurface(uiParent*,bool forread,
				    const char* type);

    void		fillFields(const MultiID&);
    void		fillSectionFld(const BufferStringSet&);
    void		fillAttribFld(const BufferStringSet&);
    void		fillRangeFld(const HorSampling&);

    void		mkAttribFld();
    void		mkSectionFld(bool);
    void		mkRangeFld();
    void		mkObjFld(const char*);

    void		objSel(CallBacker*);
    void		attrSel(CallBacker*);
    virtual void	ioDataSelChg(CallBacker*)			{};

    uiLabeledListBox*	sectionfld;
    uiLabeledListBox*	attribfld;
    uiBinIDSubSel*	rgfld;
    uiIOObjSel*		objfld;

    CtxtIOObj&		ctio;
    bool		forread;
};


class uiSurfaceWrite : public uiIOSurface
{
public:
			uiSurfaceWrite(uiParent*,const EM::Surface&,
				       const char* type);

    virtual bool	processInput();
    bool		replaceInTree()	const;

protected:
    void 		ioDataSelChg(CallBacker*);
    uiCheckBox*		replacefld;
};


class uiSurfaceRead : public uiIOSurface
{
public:
    			uiSurfaceRead(uiParent*,const char* type,
				      bool showattribfld=true);

    virtual bool	processInput();
    void		setIOObj(const MultiID&);

protected:


};


#endif
