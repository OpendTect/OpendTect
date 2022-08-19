#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "paralleltask.h"

namespace OD { class RGBImage; };


/*!
\brief Resizes Image.
*/

mClass(Algo) ImageResizer : public ParallelTask
{
public:
		ImageResizer(const OD::RGBImage&,OD::RGBImage&);
		~ImageResizer();

    void	setLanczosSize(int);	//default is 2;

protected:

    od_int64			nrIterations() const override;
    bool			doPrepare(int) override;
    bool			doWork(od_int64,od_int64,int) override;
    bool			doFinish(bool) override;

    int				lanczossize_ = 2;

    const OD::RGBImage&		input_;
    const unsigned char*	inputimage_;
    bool			inputowner_ = false;
    int				inputsize_[2];

    OD::RGBImage&		output_;
    unsigned char*		outputimage_;
    bool			outputowner_ = false;
    int				outputsize_[2];

    double			support0_ = mUdf(double);
    double			support1_ = mUdf(double);
    double			scale0_ = mUdf(double);
    double			scale1_ = mUdf(double);
    double			factor0_ = mUdf(double);
    double			factor1_ = mUdf(double);
    char			nrcomponents_ = mUdf(char);

};
