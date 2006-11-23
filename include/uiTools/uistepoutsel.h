#ifndef uistepoutsel_h
#define uistepoutsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uistepoutsel.h,v 1.2 2006-11-23 12:55:40 cvsbert Exp $
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
			    , single_(false)
			    , couple_(false)		{}

	mDefSetupMemb(BufferString,seltxt)
	mDefSetupMemb(BufferString,lbl1)
	mDefSetupMemb(BufferString,lbl2)
	mDefSetupMemb(bool,single)
	mDefSetupMemb(bool,couple)
    };

			uiStepOutSel(uiParent*,const Setup&);
			~uiStepOutSel()		{}

    int			val(bool dir1) const;
    void		setVal(bool dir1,int);
    void		allowInp(bool dir1,bool yn);

    Notifier<uiStepOutSel> valueChanged;

    void		setVals(int); //!< similar to 2x setVal
    void		setLabel(bool dir1,const char*);
    void		set2D(bool); //!< Assumes inl/crl vs trace nr
    bool		dir2Active() const;
    void		setRowCol( const RowCol& rc )
    			{ setVal(true,rc.r()); setVal(false,rc.c()); }
    void		setBinID(const BinID&);
    				//!< Different from RowCol when no dir2 present:
				//!< then crl is used, not inl
    RowCol		getRowCol() const
			{ return RowCol( val(true), val(false) ); }
    BinID		getBinID() const; //!< Similar remark as setBinID()

protected:

    uiSpinBox*		fld1;
    uiSpinBox*		fld2;

    void		valChg(CallBacker*);

};


#endif
