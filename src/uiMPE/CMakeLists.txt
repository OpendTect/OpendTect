#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

set(OD_MODULE_DEPS uiAttributes uiFlatView MPEEngine )
set(OD_MODULE_SOURCES
	inituimpe.cc
	mpef3dflatvieweditor.cc
	mpefssflatvieweditor.cc
	uihorizontracksetup.cc
	uimpe.cc
	uimpecorrelationgrp.cc
	uimpeeventgrp.cc
	uimpepartserv.cc
	uimpepreviewgrp.cc
	horflatvieweditor2d.cc
	horflatvieweditor3d.cc
)

OD_INIT_MODULE()
