#ifndef uiposfiltgroupstd_h
#define uiposfiltgroup_stdh

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uiposfiltgroupstd.h,v 1.1 2008-02-26 08:55:18 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiposfiltgroup.h"
class uiSpinBox;


/*! \brief UI for Random Position filter */

class uiRandPosFiltGroup : public uiPosFiltGroup
{
public:

			uiRandPosFiltGroup(uiParent*,const Setup&);

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;
    virtual void	getSummary(BufferString&) const;

    static uiPosFiltGroup* create( uiParent* p, const Setup& s )
    			{ return new uiRandPosFiltGroup(p,s); }
    static void		initClass();

protected:

    uiSpinBox*		percpassfld_;

};


/*! \brief UI for Subsample Position filter */

class uiSubsampPosFiltGroup : public uiPosFiltGroup
{
public:

			uiSubsampPosFiltGroup(uiParent*,const Setup&);

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;
    virtual void	getSummary(BufferString&) const;

    static uiPosFiltGroup* create( uiParent* p, const Setup& s )
    			{ return new uiSubsampPosFiltGroup(p,s); }
    static void		initClass();

protected:

    uiSpinBox*		eachfld_;

};


#endif
