#ifndef uiposfiltgroupstd_h
#define uiposfiltgroup_stdh

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiposfiltgroup.h"
class uiSpinBox;
class uiGenInput;


/*! \brief UI for Random Position filter */

mClass uiRandPosFiltGroup : public uiPosFiltGroup
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

    uiGenInput*		percpassfld_;

};


/*! \brief UI for Subsample Position filter */

mClass uiSubsampPosFiltGroup : public uiPosFiltGroup
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
