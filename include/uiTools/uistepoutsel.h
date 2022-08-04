#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "position.h"
#include "rowcol.h"
#include "uistrings.h"
class uiSpinBox;


/*! \brief allows selection of stepouts.

  Typically, the stepout is in inl/crl or row/col.

 */

mExpClass(uiTools) uiStepOutSel : public uiGroup
{ mODTextTranslationClass(uiStepOutSel);
public:

    struct Setup
    {
			Setup( bool singl=false )
			    : seltxt_(tr("Stepout"))
			    , lbl1_(singl?tr("nr"):tr("inl"))
			    , lbl2_(tr("crl"))
			    , single_(singl)
			    , allowneg_(false)	{}

	mDefSetupMemb(uiString,seltxt)
	mDefSetupMemb(uiString,lbl1)
	mDefSetupMemb(uiString,lbl2)
	mDefSetupMemb(bool,single)	//!< Typically used for 2D
	mDefSetupMemb(bool,allowneg)
    };

			uiStepOutSel(uiParent*,const Setup&);
			uiStepOutSel(uiParent*,bool single=false,
				     const uiString& seltxt=tr("Stepout"));
			~uiStepOutSel();

    int			val(bool dir1) const;
    void		setVal(bool dir1,int);

    Notifier<uiStepOutSel> valueChanged;
    Notifier<uiStepOutSel> valueChanging;

    virtual void	setVals(int); //!< similar to 2x setVal
    bool		dir2Active() const;
    void		setRowCol( const RowCol& rc )
			{ setVal(true,rc.row()); setVal(false,rc.col()); }
    void		setBinID(const BinID&);
				//!< Different from RowCol when no dir2 present:
				//!< then crl is used, not inl
    RowCol		getRowCol() const
			{ return RowCol( val(true), val(false) ); }
    BinID		getBinID() const; //!< Similar remark as setBinID()
    void		setInterval(StepInterval<int> inlrg,
				StepInterval<int> crlrg);

    void		setFieldNames(const char* nm1,const char* nm2=0);
    void		set3D(bool yn);

protected:

    uiSpinBox*		fld1_;
    uiSpinBox*		fld2_;

    void		valChanged(CallBacker*);
    void		valChanging(CallBacker*);

private:

    void		init(const Setup&);
    uiStepOutSel::Setup	sosetup_;

};


/*! \brief allows selection of stepouts, extension to third direction (Z). */

mExpClass(uiTools) uiStepout3DSel : public uiStepOutSel
{ mODTextTranslationClass(uiStepout3DSel)
public:

			uiStepout3DSel(uiParent*,const uiStepOutSel::Setup&);
			uiStepout3DSel(uiParent*,bool single=false,
				  const uiString& seltxt=uiStrings::sStepout());
			~uiStepout3DSel() {}

    int			getZVal() const; //nr samples
    int			val(int dir) const;

    void		setZVal(int);
    void		setVals(int,int,int);
    void		setVals(int) override; //!< similar to 3x setVal

    void                setZInterval(StepInterval<int> zrg); //nr samples

    void		setZFieldName(const char*);

protected:

    uiSpinBox*		fld3_;

};


