#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          August 2010
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
		ImageResizer( const OD::RGBImage&, OD::RGBImage& );
		~ImageResizer();

    void	setLanczosSize(int);	//default is 2;

protected:

    od_int64			nrIterations() const override;
    bool			doPrepare(int) override;
    bool			doWork(od_int64,od_int64,int) override;
    bool			doFinish(bool) override;

    int				lanczossize_;

    const OD::RGBImage&		input_;
    const unsigned char*	inputimage_;
    bool			inputowner_;
    int				inputsize_[2];

    OD::RGBImage&		output_;
    unsigned char*		outputimage_;
    bool			outputowner_;
    int				outputsize_[2];

    double			support0_;
    double			support1_;
    double			scale0_;
    double			scale1_;
    double			factor0_;
    double			factor1_;
    char			nrcomponents_;

};


