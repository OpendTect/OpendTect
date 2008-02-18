#ifndef uiposprovider_h
#define uiposprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uiposprovider.h,v 1.6 2008-02-18 16:32:17 cvsbert Exp $
________________________________________________________________________

-*/

#include "uicompoundparsel.h"
#include "iopar.h"
class uiGenInput;
namespace Pos { class Provider; }
class uiPosProvGroup;
class CubeSampling;

/*! \brief lets user choose a way to provide positions */

class uiPosProvider : public uiGroup
{
public:

    struct Setup
    {
	enum ChoiceType	{ All, OnlySeisTypes, OnlyRanges };

			Setup( const char* txt, bool with_z )
			    : seltxt_(txt)
			    , withz_(with_z)
			    , is2d_(false)
			    , allownone_(false)
			    , choicetype_(OnlyRanges)	{}
	virtual	~Setup()				{}
	mDefSetupMemb(BufferString,seltxt)
	mDefSetupMemb(bool,is2d)
	mDefSetupMemb(bool,withz)
	mDefSetupMemb(ChoiceType,choicetype)
	mDefSetupMemb(bool,allownone)
    };

    			uiPosProvider(uiParent*,const Setup&);

    void		usePar(const IOPar&);
    bool		fillPar(IOPar&) const;

    Pos::Provider*	createProvider() const;

    bool		isAll() const		{ return !curGrp(); }

protected:

    uiGenInput*			selfld_;
    ObjectSet<uiPosProvGroup>	grps_;
    Setup			setup_;

    void			selChg(CallBacker*);
    uiPosProvGroup*		curGrp() const;
};


/*!\brief CompoundParSel to capture a user's Pos::Provider wishes */


class uiPosProvSel : public uiCompoundParSel
{
public:

    typedef uiPosProvider::Setup Setup;

    			uiPosProvSel(uiParent*,const Setup&);
    			~uiPosProvSel();

    void		usePar(const IOPar&);
    void		fillPar( IOPar& iop ) const	{ iop.merge(iop_); }

    Pos::Provider*	curProvider()			{ return prov_; }
    const Pos::Provider* curProvider() const		{ return prov_; }

    const CubeSampling&	envelope() const		{ return cs_; }
    void		setInput(const CubeSampling&,bool chgtype=true);

    bool		isAll() const;
    void		setToAll();

protected:

    Setup		setup_;
    IOPar		iop_;
    Pos::Provider*	prov_;
    mutable CubeSampling& cs_;

    void		doDlg(CallBacker*);
    BufferString	getSummary() const;
    void		setCSToAll() const;
    void		setProvFromCS();
    void		mkNewProv(bool updsumm=true);

};


#endif
