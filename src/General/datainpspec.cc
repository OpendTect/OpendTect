/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: datainpspec.cc,v 1.1 2001-05-01 09:30:40 bert Exp $
________________________________________________________________________

-*/

#include "datainpspec.h"

StringInp::StringInp( const char* s, int prefw )
    : DataInpSpec( stringTp ), str( s ), pw( prefw ) {}

FileNameInp::FileNameInp( const char* fname, int prefw )
    : StringInp( fname, prefw ) { setType( fileNmTp ); }
