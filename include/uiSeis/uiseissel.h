#ifndef uiseissel_h
#define uiseissel_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2001
 RCS:           $Id: uiseissel.h,v 1.27 2008-01-22 15:04:17 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiioobjsel.h"
#include "seistype.h"


class uiSeisSel : public uiIOObjSel
{
public:

    struct Setup
    {
			Setup( Seis::GeomType gt )
			    : geom_(gt)
			    , selattr_(true)
			    , withclear_(false)
			    , seltxt_(0)	{}
			Setup( bool is2d, bool isps )
			    : geom_(Seis::geomTypeOf(is2d,isps))
			    , selattr_(true)
			    , withclear_(false)
			    , seltxt_(0)	{}

	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(bool,selattr)
	mDefSetupMemb(bool,withclear)
	mDefSetupMemb(const char*,seltxt)
    };

			uiSeisSel(uiParent*,CtxtIOObj&,const Setup&);
			~uiSeisSel();

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    inline Seis::GeomType geomType() const { return setup_.geom_; }
    inline bool		is2D() const	{ return Seis::is2D(setup_.geom_); }
    inline bool		isPS() const	{ return Seis::isPS(setup_.geom_); }

    void		setAttrNm(const char*);
    const char*		attrNm() const	{ return attrnm.buf(); }
    virtual void	processInput();
    virtual bool	existingTyped() const;
    virtual void	updateInput();

protected:

    Setup		setup_;
    BufferString	orgkeyvals;
    BufferString	attrnm;
    mutable BufferString curusrnm;
    IOPar&		dlgiopar;

    virtual void	newSelection(uiIOObjRetDlg*);
    virtual const char*	userNameFromKey(const char*) const;
    virtual uiIOObjRetDlg* mkDlg();
};


class uiSeisSelDlg : public uiIOObjSelDlg
{
public:

			uiSeisSelDlg(uiParent*,const CtxtIOObj&,
				     const uiSeisSel::Setup&);

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    static const char*	standardTranslSel(Seis::GeomType,bool);

protected:

    uiGenInput*		attrfld_;
    bool		allowcnstrsabsent_;	//2D only

    void		entrySel(CallBacker*);
    void		filter2DStoredNames(BufferStringSet&) const;
};


#endif
