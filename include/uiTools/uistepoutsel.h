#ifndef uistepoutsel_h
#define uistepoutsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uistepoutsel.h,v 1.11 2009-07-22 16:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "position.h"
#include "rowcol.h"
class uiSpinBox;


/*! \brief allows selection of stepouts.
 
  Typically, the stepout is in inl/crl or row/col.
 
 */

mClass uiStepOutSel : public uiGroup
{
public:

    struct Setup
    {
			Setup( bool singl=false )
			    : seltxt_("Stepout")
			    , lbl1_(singl?"nr":"inl")
			    , lbl2_("crl")
			    , single_(singl)
			    , allowneg_(false)	{}

	mDefSetupMemb(BufferString,seltxt)
	mDefSetupMemb(BufferString,lbl1)
	mDefSetupMemb(BufferString,lbl2)
	mDefSetupMemb(bool,single)	//!< Typically used for 2D
	mDefSetupMemb(bool,allowneg)
    };

			uiStepOutSel(uiParent*,const Setup&);
			uiStepOutSel(uiParent*,bool single=false,
					       const char* seltxt="Stepout");
			~uiStepOutSel()		{}

    int			val(bool dir1) const;
    void		setVal(bool dir1,int);

    Notifier<uiStepOutSel> valueChanged;
    Notifier<uiStepOutSel> valueChanging;

    void		setVals(int); //!< similar to 2x setVal
    bool		dir2Active() const;
    void		setRowCol( const RowCol& rc )
    			{ setVal(true,rc.r()); setVal(false,rc.c()); }
    void		setBinID(const BinID&);
    				//!< Different from RowCol when no dir2 present:
				//!< then crl is used, not inl
    RowCol		getRowCol() const
			{ return RowCol( val(true), val(false) ); }
    BinID		getBinID() const; //!< Similar remark as setBinID()
    void		setInterval(StepInterval<int> inlrg,
	    			StepInterval<int> crlrg);

    void		setFieldNames(const char* nm1,const char* nm2=0);

protected:

    uiSpinBox*		fld1;
    uiSpinBox*		fld2;
    bool		is2d;

    void		valChanged(CallBacker*);
    void		valChanging(CallBacker*);

private:

    void		init(const Setup&);

};


#endif
