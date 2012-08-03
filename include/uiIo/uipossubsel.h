#ifndef uipossubsel_h
#define uipossubsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uipossubsel.h,v 1.12 2012-08-03 13:01:01 cvskris Exp $
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uigroup.h"
namespace Pos { class Provider; }
class CubeSampling;
class uiPosProvSel;


/*!\brief Group to capture a user's position subselection wishes.

  The class is (of course) based on the Pos::Provider classes, but if you
  fill your IOPar, you can give that straight to other subselection classes,
  like Seis::Selection.

  Users can always choose to not subselect at all.
 
 */


mClass(uiIo) uiPosSubSel : public uiGroup
{
public:

    struct Setup
    {
	enum ChoiceType	{ All, OnlySeisTypes, OnlyRanges, RangewithPolygon };
			Setup( bool is_2d, bool with_z )
			    : seltxt_( is_2d	? "Trace subselection"
				   : ( with_z	? "Volume subselection"
						: "Area subselection"))
			    , is2d_(is_2d)
			    , withz_(with_z)
			    , withstep_(true)
			    , choicetype_(OnlyRanges)	{}
	mDefSetupMemb(BufferString,seltxt)
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

    const CubeSampling&	envelope() const;
    const CubeSampling&	inputLimit() const;
    void		setInput(const CubeSampling&,bool chgtype=true);
    void                setInput(const CubeSampling& initcs,
				 const CubeSampling& ioparcs);
    void                setInputLimit(const CubeSampling&);

    bool		isAll() const;
    void		setToAll();


    Notifier<uiPosSubSel> selChange;
    uiPosProvSel*	provSel()		{ return ps_; }

protected:

    uiPosProvSel*	ps_;

    void		selChg(CallBacker*);

};


#endif

