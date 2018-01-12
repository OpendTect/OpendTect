#pragma once

/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		October 2016
___________________________________________________________________

-*/

#include "geometrymod.h"
#include "uistring.h"
#include "probe.h"
#include "emposid.h"


mExpClass(EarthModel) EMProbe : public Probe
{
public:

    typedef EM::ObjectID	ObjectID;

				~EMProbe();

    mDeclInstanceCreatedNotifierAccess(EMProbe);
    mDeclAbstractMonitorableAssignment(EMProbe);

    mImplSimpleMonitoredGet(	id,ObjectID,objid_)
    void			setID(const ObjectID&);
    void			updateAll();

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

protected:

				EMProbe(const ObjectID&);

    uiWord			gtDisplayName() const;

    ObjectID			objid_;

};


mExpClass(EarthModel) Horizon3DProbe : public EMProbe
{
public:

				Horizon3DProbe(const ObjectID& id=ObjectID(0));
				~Horizon3DProbe();
    mDeclInstanceCreatedNotifierAccess(Horizon3DProbe);
    mDeclMonitorableAssignment(Horizon3DProbe);

    mDeclRequiredProbeFns();

    virtual bool		isVertical() const	{ return false; }

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

};
