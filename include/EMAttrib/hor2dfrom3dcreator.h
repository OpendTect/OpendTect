#ifndef hor2dfrom3dcreator_h
#define hor2dfrom3dcreator_h
/*
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Satyaki Maitra
Date:          April 2010
RCS:           $Id: hor2dfrom3dcreator.h,v 1.1 2010-04-19 05:41:43 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "posinfo.h"

namespace EM { class Horizon3D; class Horizon2D; }
class BufferString;
class BufferStringSet;
class MultiID;

mClass Hor2DFrom3DCreatorGrp : public ExecutorGroup
{
public:
    				Hor2DFrom3DCreatorGrp(const EM::Horizon3D&,
						      EM::Horizon2D&);
				~Hor2DFrom3DCreatorGrp();
   void				init(const BufferStringSet&,const MultiID&);

protected:
    const EM::Horizon3D&	hor3d_;
    EM::Horizon2D&		hor2d_;
};


mClass Hor2DFrom3DCreator : public Executor
{
public:
    				Hor2DFrom3DCreator(const EM::Horizon3D&,
						   EM::Horizon2D&);

    bool			setCreator(const BufferString&,const MultiID&);
    virtual int			nextStep();
    virtual od_int64		nrDone() const		{ return nrdone_; }
    virtual od_int64		totalNr() const		{ return totalnr_; }

protected:
    const EM::Horizon3D&	hor3d_;
    EM::Horizon2D&		hor2d_;
    int				nrdone_;
    int				totalnr_;
    int				lineid_;
    
    PosInfo::Line2DData		posdata_;
};

#endif
