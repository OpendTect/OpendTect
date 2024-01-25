#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"

#include "uigroup.h"

class TrcKeyZSampling;
class uiPosProvSel;
namespace Pos { class Provider; }
namespace ZDomain { class Info; }


/*!\brief Group to capture a user's position subselection wishes.

  The class is (of course) based on the Pos::Provider classes, but if you
  fill your IOPar, you can give that straight to other subselection classes,
  like Seis::Selection.

  Users can always choose to not subselect at all.

 */


mExpClass(uiIo) uiPosSubSel : public uiGroup
{
public:

    mExpClass(uiIo) Setup
    {
    public:
	enum ChoiceType	{ All, OnlySeisTypes, OnlyRanges, RangewithPolygon,
			  VolumeTypes };

			Setup(bool is_2d,bool with_z);
			~Setup();

	mDefSetupMemb(BufferString,seltxt)
	mDefSetupMemb(bool,is2d)
	mDefSetupMemb(bool,withz)
	mDefSetupMemb(bool,withstep)		// true
	mDefSetupMemb(ChoiceType,choicetype)	// OnlyRanges
	mDefSetupMemb(BufferString,zdomkey)
	mDefSetupMemb(BufferString,zunitstr)
    };

			uiPosSubSel(uiParent*,const Setup&);
			~uiPosSubSel();

    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    Pos::Provider*	curProvider();
    const Pos::Provider* curProvider() const;

    const ZDomain::Info* zDomain() const;
    const TrcKeyZSampling& envelope() const;
    const TrcKeyZSampling& inputLimit() const;
    void		setInput(const TrcKeyZSampling&,bool chgtype=true);
    void		setInput(const TrcKeyZSampling& initcs,
				 const TrcKeyZSampling& ioparcs);
    void		setInputLimit(const TrcKeyZSampling&);

    bool		isAll() const;
    void		setToAll();

    const uiPosProvSel* provSel() const		{ return ps_; }
    uiPosProvSel*	provSel()		{ return ps_; }

    Notifier<uiPosSubSel> selChange;

protected:

    void		selChg(CallBacker*);

    uiPosProvSel*	ps_;

};
