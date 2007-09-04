#ifndef marchingcubeseditor_h
#define marchingcubeseditor_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          August 2007
 RCS:           $Id: marchingcubeseditor.h,v 1.1 2007-09-04 18:02:28 cvskris Exp $
________________________________________________________________________

-*/

#include "basictask.h"
#include "callback.h"

class MarchingCubesSurface;
template <class T> class Interval;
template <class T> class Array3D;

/*!
Editor for MarchingCubesSurfaces. It operates by converting a part of the
MarchingCubesSurface to an implicit represetation (a scalar field and a
threshold), modifies that scalar field and re-generates the
MarchingCubesSurfaces.

The modification of the implicit representation is guided by:

NewImplicitShape = OriginalShape * Kernel * factor

The Kernel is a scalar field (values between 0 and 255) represents the shape of 
the movement, and the factor is set by from the outside.

*/


class MarchingCubesSurfaceEditor : private ParallelTask, public CallBacker
{
public:
    			MarchingCubesSurfaceEditor(MarchingCubesSurface&);
    virtual		~MarchingCubesSurfaceEditor();

    bool		setFactor(int); //!<a value of 255 is one voxel if
    					//!<the kernel is 255
    int			getFactor() const { return factor_; }

    virtual void	affectedVolume(Interval<int>& xrg,
	    			       Interval<int>& yrg,
				       Interval<int>& zrg) const;

    Notifier<MarchingCubesSurfaceEditor>	shapeChange;
    
protected:
    void			reportShapeChange(bool kernelchange);
    				/*!<Should be called by inheriting class
				    if it has changed kernel size or
				    kernel position. */

    virtual Interval<int>	kernelXRange() const			= 0;
    virtual Interval<int>	kernelYRange() const			= 0;
    virtual Interval<int>	kernelZRange() const			= 0;

    virtual bool		computeKernel(Array3D<unsigned char>&) const =0;

    MarchingCubesSurface&	surface_;
    int				factor_;
    int				prevfactor_;
    Array3D<unsigned char>*	kernel_;
    Array3D<int>*		implicitsurface_;
    Array3D<int>*		originalsurface_;
    float			threshold_;

private:
    int				nrTimes() const;
    bool			doPrepare(int);
    bool			doWork(int,int,int);
    bool			doFinish(bool);
};

#endif
