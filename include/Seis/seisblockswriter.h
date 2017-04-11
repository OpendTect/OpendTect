#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

*/

#include "seiscommon.h"
#include "filepath.h"
#include "uistring.h"

class Scaler;
class SeisTrc;
namespace Survey { class Geometry3D; }


namespace Seis
{
namespace Blocks
{

class Data;

/*!\brief Writes provided data into Block Storage */

mExpClass(Seis) Writer
{ mODTextTranslationClass(Seis::Blocks::Writer);
public:

			Writer(const Survey::Geometry3D* sg=0);
			~Writer();

    void		setBasePath(const File::Path&);

    uiRetVal		add(const SeisTrc&);

protected:

    const Survey::Geometry3D& survgeom_;
    File::Path		basepath_;
    Scaler*		scaler_;

    bool		needreset_;
    uiRetVal		reset();

};


} // namespace Blocks

} // namespace Seis
