#pragma once

/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		October 2016
___________________________________________________________________

-*/

#include "emcommon.h"
#include "uistring.h"
#include "probe.h"


mExpClass(EarthModel) EMProbe : public Probe
{
public:
				~EMProbe();

    mDeclInstanceCreatedNotifierAccess(EMProbe);
    mDeclAbstractMonitorableAssignment(EMProbe);

    mImplSimpleMonitoredGet(	id,EM::ObjID,objid_)
    void			setID(const EM::ObjID&);
    void			updateAll();

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

protected:

				EMProbe(const DBKey&);

    uiWord			gtDisplayName() const;

    EM::ObjID			objid_;

};


mExpClass(EarthModel) Horizon3DProbe : public EMProbe
{
public:

				Horizon3DProbe(const DBKey&);
				~Horizon3DProbe();
    mDeclInstanceCreatedNotifierAccess(Horizon3DProbe);
    mDeclMonitorableAssignment(Horizon3DProbe);

    mDeclRequiredProbeFns();

    virtual bool		isVertical() const	{ return false; }

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

};
