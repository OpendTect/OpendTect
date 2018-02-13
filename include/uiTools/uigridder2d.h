#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          March 2008
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidlggroup.h"

#include "arrayndalgo.h"
#include "factory.h"

class Gridder2D;
class uiGridder2DGrp;
class InverseDistanceGridder2D;
class TriangulatedGridder2D;
class uiGenInput;


mExpClass(uiTools) uiGridder2DSel : public uiDlgGroup
{ mODTextTranslationClass(uiGridder2DSel);
public:
				uiGridder2DSel(uiParent*,const Gridder2D*,
					      PolyTrend::Order=PolyTrend::None);
				~uiGridder2DSel();

    bool			usePar(const IOPar&);
    void			fillPar(IOPar&,bool withprefix=false) const;

    uiString			message() const		{ return msg_; }

protected:

    void			selChangeCB(CallBacker*);
    const uiGridder2DGrp*	getSel() const;
    const Gridder2D*		original_;
    uiGenInput*			griddingsel_;

    ObjectSet<uiGridder2DGrp>	griddinggrps_;
    mutable uiString		msg_;
};


mExpClass(uiTools) uiGridder2DGrp : public uiDlgGroup
{ mODTextTranslationClass(uiGridder2DGrp)
public:

    mDefineFactory2ParamInClass( uiGridder2DGrp, uiParent*,
				 const BufferString&, factory );

				~uiGridder2DGrp();

    virtual bool		usePar(const IOPar&);
    virtual bool		fillPar(IOPar&) const;

    virtual bool		rejectOK()	{ return revertChanges(); }
    bool			revertChanges();

    virtual uiString		errMsg() const	{ return msg_; }

protected:

				uiGridder2DGrp(uiParent*,const uiString&,
				      const BufferString&,bool withtrend=false);

    virtual void		getFromScreen() const	{}
    virtual void		putToScreen()		{}

    uiGenInput*			trendFld() const	{ return trendfld_; }

    Gridder2D*			gridder_;
    IOPar			initialstate_;
    mutable uiString		msg_;

private:

    uiGenInput*			trendfld_;

};


mExpClass(uiTools) uiInverseDistanceGridder2D : public uiGridder2DGrp
{ mODTextTranslationClass(uiInverseDistanceGridder2D)
public:
    static void			initClass();
    static uiGridder2DGrp*	create(uiParent*,const BufferString&);

				uiInverseDistanceGridder2D(uiParent*,
							   const BufferString&);
protected:

    virtual void		getFromScreen() const;
    virtual void		putToScreen();

    uiGenInput*			searchradiusfld_;
};


mExpClass(uiTools) uiTriangulatedGridder2D : public uiGridder2DGrp
{ mODTextTranslationClass(uiTriangulatedGridder2D)
public:
    static void			initClass();
    static uiGridder2DGrp*	create(uiParent*,const BufferString&);

				uiTriangulatedGridder2D(uiParent*,
							const BufferString&);

};
