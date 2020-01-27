#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uigroup.h"
namespace Pos { class Provider; }
class TrcKeyZSampling;
class uiPosProvSel;


/*!\brief Group to capture a user's position subselection wishes.

  The class is (of course) based on the Pos::Provider classes, but if you
  fill your IOPar, you can give that straight to other subselection classes,
  like Seis::Selection.

  Users can always choose to not subselect at all.

 */


mExpClass(uiIo) uiPosSubSel : public uiGroup
{ mODTextTranslationClass(uiPosSubSel)
public:

    struct Setup
    {
	enum ChoiceType	{ All, OnlySeisTypes, OnlyRanges, RangewithPolygon,
			  VolumeTypes };
			Setup( bool is_2d, bool with_z )
			    : seltxt_( is_2d	? tr("Trace subselection")
				   : ( with_z	? tr("Volume subselection")
						: tr("Area subselection")))
			    , is2d_(is_2d)
			    , withz_(with_z)
			    , withstep_(true)
			    , choicetype_(OnlyRanges)	{}
	mDefSetupMemb(uiString,seltxt)
	mDefSetupMemb(bool,is2d)
	mDefSetupMemb(bool,withz)
	mDefSetupMemb(bool,withstep)
	mDefSetupMemb(BufferString,zdomkey)
	mDefSetupMemb(ChoiceType,choicetype)
    };

			uiPosSubSel(uiParent*,const Setup&);

    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    Pos::Provider*	curProvider();
    const Pos::Provider* curProvider() const;

    const TrcKeyZSampling&	envelope() const;
    TrcKeyZSampling	inputLimit() const;
    void		setInput(const TrcKeyZSampling&,bool chgtype=true);
    void		setInput(const TrcKeyZSampling& initcs,
				 const TrcKeyZSampling& ioparcs);
    void		setInputLimit(const TrcKeyZSampling&);

    bool		isAll() const;
    void		setToAll();


    Notifier<uiPosSubSel> selChange;
    uiPosProvSel*	provSel()		{ return ps_; }

protected:

    uiPosProvSel*	ps_;

    void		selChg(CallBacker*);

};
