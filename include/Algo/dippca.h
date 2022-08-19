#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "algomod.h"
#include "enums.h"
#include "factory.h"
#include "odmemory.h"

template <class T> class Array2D;
template <class T> class Array3D;

class Dip2DCalculator;
class Dip3DCalculator;
class TaskRunner;


/*!
\brief Base class to calculate Dip/Azimuth using the method of PCA.

  Example of use:

  Dip3D d3d( your data )
  d3d.setSetup( your setup )
  d3d.compute();
*/

mExpClass(Algo) DipPCA
{
public:

    virtual			~DipPCA()			{}

    mStruct(Algo) Setup
    {				Setup();
	Setup&			operator=(const Setup&);

	float			threshold_;
	bool			isabove_; //Flt value is above threshold or not
	int			boxlength_;
	int			boxwidth_;
	int			boxheight_;
	StepInterval<int>	thetarg_; //dip angle in positive degree
	StepInterval<int>	alpharg_; //azimuth angle in positive degree
    };

    void			setSetup(Setup nsetup)	{ setup_ = nsetup; }
    virtual bool		compute(TaskRunner* tr=0) { return true; }

protected:

    Setup			setup_;
};


/*!
\brief To calculate Dip/Azimuth for 2D datasets using the method of PCA.
*/

mExpClass(Algo) Dip2D : public DipPCA
{
public:
				Dip2D(const Array2D<float>& input,
					   float xdist,float ydist);
				~Dip2D();

    bool			compute(TaskRunner* =nullptr) override;

    enum Output			{ Dip=0, Dilation=1, Thinning=2 };
    const Array2D<float>*	get(Output) const;

protected:

    friend class		Dip2DCalculator;
    bool			fillGap();


    const Array2D<float>&	input_;

    Array2D<float>*		dip_; //The fault dip
    Array2D<float>*		dilation_;
				//The fault dip after perform dilation to dip
    Array2D<float>*		dthinner_;
				//The fault dip after thining to dilation

    const int			xsz_;
    const int			ysz_;
    const float			xdist_;
    const float			ydist_;
};


/*!
\brief To calculate Dip/Azimuth for 3D datasets using the method of PCA.
*/

mExpClass(Algo) Dip3D : public DipPCA
{
public:
				Dip3D(const Array3D<float>& input,
					float xdist,float ydist,float zdist);
				~Dip3D();

    bool			compute(TaskRunner* =nullptr) override;

    enum Output			{ AbsDip=0, InlDip=1, CrlDip=2, Azimuth=3 };
    const Array3D<float>*	get(Output) const;

protected:

    friend class		Dip3DCalculator;
    const Array3D<float>&	input_;

    Array3D<float>*		absdip_;
    Array3D<float>*		inldip_;
    Array3D<float>*		crldip_;
    Array3D<float>*		azimuth_;

    const int			xsz_;
    const int			ysz_;
    const int			zsz_;
    const float			xdist_;
    const float			ydist_;
    const float			zdist_;
};
