#ifndef hor2dfrom3dcreator_h
#define hor2dfrom3dcreator_h
/*
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Satyaki Maitra
Date:          April 2010
RCS:           $Id$
________________________________________________________________________

-*/

#include "emattribmod.h"
#include "executor.h"
#include "posinfo2d.h"
#include "surv2dgeom.h"

namespace EM { class Horizon3D; class Horizon2D; }
class BufferString;
class BufferStringSet;
class MultiID;

/*!
\brief ExecutorGroup to create EM::Horizon2D from EM::Horizon3D.
*/

mClass(EMAttrib) Hor2DFrom3DCreatorGrp : public ExecutorGroup
{
public:
    				Hor2DFrom3DCreatorGrp(const EM::Horizon3D&,
						      EM::Horizon2D&);
				~Hor2DFrom3DCreatorGrp();
   void				init(const BufferStringSet&,const char* lsnm);

protected:
    const EM::Horizon3D&	hor3d_;
    EM::Horizon2D&		hor2d_;
};


/*!
\brief Executor to create EM::Horizon2D from EM::Horizon3D.
*/

mClass(EMAttrib) Hor2DFrom3DCreator : public Executor
{
public:
    				Hor2DFrom3DCreator(const EM::Horizon3D&,
						   EM::Horizon2D&);

    bool			setCreator(const char* linenm,const char* lsnm);
    virtual int			nextStep();
    virtual od_int64		nrDone() const		{ return nrdone_; }
    virtual od_int64		totalNr() const		{ return totalnr_; }

protected:
    const EM::Horizon3D&	hor3d_;
    EM::Horizon2D&		hor2d_;
    int				nrdone_;
    int				totalnr_;
    PosInfo::GeomID		geomid_;
    
    PosInfo::Line2DData		posdata_;
};

#endif

