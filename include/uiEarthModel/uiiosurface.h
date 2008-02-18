#ifndef uiiosurface_h
#define uiiosurface_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurface.h,v 1.23 2008-02-18 11:00:47 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class BufferStringSet;
class CtxtIOObj;
class HorSampling;
class IODirEntryList;
class IOObj;
class MultiID;

class uiPosSubSel;
class uiColorInput;
class uiGenInput;
class uiIOObjSel;
class uiLabeledListBox;
class uiCheckBox;
class uiStratLevelSel;


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
    uiPosSubSel*	rgfld;
    uiIOObjSel*		objfld;

    CtxtIOObj&		ctio;
    bool		forread;
};


class uiSurfaceWrite : public uiIOSurface
{
public:

    class Setup
    {
    public:

			Setup( const char* typ )
			: typ_(typ)
			, withsubsel_(false)
			, withcolorfld_(false)
			, withstratfld_(false)
			, withdisplayfld_(false)
			, dispaytext_("Replace in tree")
			{}

	mDefSetupMemb(bool,withsubsel)
	mDefSetupMemb(bool,withcolorfld)
	mDefSetupMemb(bool,withstratfld)
	mDefSetupMemb(bool,withdisplayfld)
	mDefSetupMemb(BufferString,dispaytext)
	mDefSetupMemb(BufferString,typ)

    };

			uiSurfaceWrite(uiParent*,const EM::Surface&,
				       const uiSurfaceWrite::Setup& setup);
			uiSurfaceWrite(uiParent*,
				       const uiSurfaceWrite::Setup& setup);

    virtual bool	processInput();
    const char*		getStratLevelName() const;
    Color		getColor() const;
    bool		replaceInTree()	const;

protected:
    void		stratLvlChg(CallBacker*);
    void 		ioDataSelChg(CallBacker*);
    uiCheckBox*		displayfld_;
    uiColorInput*       colbut_;
    uiStratLevelSel*    stratlvlfld_;
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
