#!/bin/csh
#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to define thirdparty library definition
# Author:       Nageswara
# Date:         August 2012
#RCS:           $Id$

#od_glxinfo lmhostid 
#Only for windows base package
#SET( SPECFILES .exec_prog .init_dtect .init_dtect_user install .lic_inst_common
#	       .lic_start_common mk_datadir .setappl.sh .start_dtect setup.od *.txt )

#Third party libraries
SET( LUXQTLIBS libQtCore.so.4 libQtGui.so.4 libQtOpenGL.so.4 libQtSql.so.4 libQtXml.so.4
	       libQtNetwork.so.4 libQt3Support.so.4 )
#SET( LUXCOINLIBS libCoin.so.6? libSoQt.so.2? libsimage.so.2? )
SET( LUXCOINLIBS libCoin.so.6 libSoQt.so.2 libsimage.so.2 )
SET( LUXOSGLIBS "" )

SET( MACQTLIBS libQtCore.4.dylib libQtGui.4.dylib libQtOpenGL.4.dylib libQtSql.4.dylib
	       libQtXml.4.dylib libQtNetwork.4.dylib libQt3Support.4.dylib )
SET( MACCOINLIBS libCoin.6?.dylib libsimage.2?.dylib libSoQt.2?.dylib libSimVoleon.4?.dylib )
SET( MACOSGLIBS "" )

SET( WINQTLIBS "" )
SET( WINCOINLIBS "" )
SET( WINOSGLIBS "" )
