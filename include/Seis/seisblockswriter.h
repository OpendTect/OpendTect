#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

*/

#include "seisblocksdata.h"
#include "seistrc.h"
#include "filepath.h"
#include "uistring.h"

class Scaler;
class SeisTrc;


namespace Seis
{
namespace Blocks
{

class Data;

/*!\brief Writes provided data into Block Storage */

mExpClass(Seis) Writer : public DataStorage
{ mODTextTranslationClass(Seis::Blocks::Writer);
public:

			Writer(const Survey::Geometry3D* sg=0);
			~Writer();

    void		setBasePath(const File::Path&);
    void		setCubeName(const char*);
    void		setComponent(int);

    uiRetVal		add(const SeisTrc&);

protected:

    File::Path		basepath_;
    BufferString	cubename_;
    int			component_;
    Scaler*		scaler_;

    bool		needreset_;
    uiRetVal		reset();

};


} // namespace Blocks

} // namespace Seis
