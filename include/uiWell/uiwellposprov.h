#ifndef uiwellposprov_h
#define uiwellposprov_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uiwellposprov.h,v 1.3 2012/02/24 23:13:20 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiposprovgroup.h"
#include "multiid.h"

class uiIOObjSelGrp;
class uiStepOutSel;
class uiSelZRange;
class uiWellParSel;


/*! \brief UI for WellPosProvider */

mClass uiWellPosProvGroup : public uiPosProvGroup
{
public:
			uiWellPosProvGroup(uiParent*,
					   const uiPosProvGroup::Setup&);
			~uiWellPosProvGroup();

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;
    void		getSummary(BufferString&) const;

    void		setExtractionDefaults();

    bool		getIDs(TypeSet<MultiID>&) const;
    void		getZRange(StepInterval<float>&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvGroup::Setup& s)
    			{ return new uiWellPosProvGroup(p,s); }
    static void		initClass();

protected:

    uiWellParSel*	wellfld_;
    uiStepOutSel*	stepoutfld_;
    uiSelZRange*	zrgfld_;

};

#endif
