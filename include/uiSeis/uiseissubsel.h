#ifndef ui2dseissubsel_h
#define ui2dseissubsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2004
 RCS:           $Id: uiseissubsel.h,v 1.1 2004-06-18 15:54:30 bert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "ranges.h"
class IOPar;
class uiGenInput;
class BufferStringSet;


class ui2DSeisSubSel : public uiGroup
{ 	
public:

			ui2DSeisSubSel(uiParent*,const BufferStringSet* lnms=0);

    void		setInput(const StepInterval<int>&);
    			//!< Trace number range
    void		setInput(const StepInterval<float>&);
    			//!< Z range
    void		setInput(const char* linename);
    void		setInput( const StepInterval<int>& tr,
	    			  const StepInterval<float>& zr )
			{ setInput( tr ); setInput( zr ); }

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;
    bool		getRange(StepInterval<int>&) const;
    bool		getZRange(StepInterval<float>&) const;

    void		setSelText(const char*);
    bool		isAll() const;

    int			expectedNrSamples() const;


protected:

    uiGenInput*		selfld;
    uiGenInput*		trcrgfld;
    uiGenInput*		zfld;
    uiGenInput*		lnmsfld;

    virtual void	selChg(CallBacker*);
    void		doFinalise(CallBacker*);

};


#endif
