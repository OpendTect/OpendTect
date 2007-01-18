#ifndef uistepoutsel_h
#define uistepoutsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uistepoutsel.h,v 1.5 2007-01-18 08:52:58 cvshelene Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "position.h"
#include "rowcol.h"
class uiSpinBox;


/*! \brief allows selection of stepouts.
 
  Typically, the stepout is in inl/crl or row/col.
 
 */

class uiStepOutSel : public uiGroup
{
public:

    struct Setup
    {
			Setup()
			    : seltxt_("Stepout")
			    , lbl1_("inl")
			    , lbl2_("crl")
			    , single_(false)		{}

	mDefSetupMemb(BufferString,seltxt)
	mDefSetupMemb(BufferString,lbl1)
	mDefSetupMemb(BufferString,lbl2)
	mDefSetupMemb(bool,single)
    };

			uiStepOutSel(uiParent*,const Setup&,bool);
			~uiStepOutSel()		{}

    int			val(bool dir1) const;
    void		setVal(bool dir1,int);

    Notifier<uiStepOutSel> valueChanged;

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
    void		setMinValue(int);

protected:

    uiSpinBox*		fld1;
    uiSpinBox*		fld2;
    bool		is2d;

    void		valChg(CallBacker*);

};


#endif
