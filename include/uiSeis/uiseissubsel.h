#ifndef uiseissubsel_h
#define uiseissubsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2004
 RCS:           $Id: uiseissubsel.h,v 1.6 2004-08-23 09:50:12 bert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "ranges.h"
class IOPar;
class uiGenInput;
class HorSampling;
class uiBinIDSubSel;
class uiSeis2DSubSel;
class BufferStringSet;


class uiSeisSubSel : public uiGroup
{
public:

    			uiSeisSubSel(uiParent*);

    bool		is2D() const		{ return is2d_; }
    void		set2D( bool yn )	{ is2d_ = yn; typChg(0); }

    void		setInput(const HorSampling&);
    void		setInput(const StepInterval<float>&);
    bool		isAll() const;
    bool		getSampling(HorSampling&) const;
    bool		getZRange(Interval<float>&) const;

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;

    int			expectedNrSamples() const;
    int			expectedNrTraces() const;

protected:

    bool		is2d_;
    uiSeis2DSubSel*	sel2d;
    uiBinIDSubSel*	sel3d;

    void		typChg(CallBacker*);

};


class uiSeis2DSubSel : public uiGroup
{ 	
public:

			uiSeis2DSubSel(uiParent*,const BufferStringSet* lnms=0);

    void		setInput(const StepInterval<int>&);
    			//!< Trace number range
    void		setInput(const Interval<float>&);
    			//!< Z range
    void		setInput(const HorSampling&);
    			//!< crlrg converted to trace range
    void		setInput(const char* linename);
    void		setInput( const StepInterval<int>& tr,
	    			  const Interval<float>& zr )
			{ setInput( tr ); setInput( zr ); }

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;
    bool		getRange(StepInterval<int>&) const;
    bool		getZRange(Interval<float>&) const;

    bool		isAll() const;

    int			expectedNrSamples() const;
    int			expectedNrTraces() const;


protected:

    uiGenInput*		selfld;
    uiGenInput*		trcrgfld;
    uiGenInput*		zfld;
    uiGenInput*		lnmsfld;

    virtual void	selChg(CallBacker*);
    void		doFinalise(CallBacker*);

};


#endif
