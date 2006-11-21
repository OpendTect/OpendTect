#ifndef uistepoutsel_h
#define uistepoutsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uistepoutsel.h,v 1.1 2006-11-21 17:47:25 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "position.h"
class uiGenInput;


/*! \brief UI element for selection of stepouts. */

class uiStepOutSel : public uiGroup
{
public:
			uiStepOutSel(uiParent*,const char* seltxt=0,
				     bool identical_inl_crl=false);
					
			~uiStepOutSel()		{}

    void		set2D(bool);
    bool		is2D() const		{ return is2d; }
    int			val(bool inl) const;
    void		setVal(bool inl,int);
    void		setBinID( const BinID& b )
			{ setVal(true,b.inl); setVal(false,b.crl); }
    BinID		binID() const
    			{ return BinID(val(true),val(false)); }

    virtual void	display(bool yn=true,bool shrk=false,bool maximz=false);

    Notifier<uiStepOutSel> valueChanged;

protected:

    uiGenInput*		inp2dfld;
    uiGenInput*		inp3dfld;

    bool		is2d;
    bool		single;
    bool		shown;

    void		update(CallBacker*);
    void		valChg(CallBacker*);

};


#endif
