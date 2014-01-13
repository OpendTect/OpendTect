#!/bin/csh 
#
# Copyright (C): dGB Beheer B. V.
#


svn status | awk '/^?/{print $2}'
