"""Module initialization

Copyright (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
  * AUTHOR   : A. Huck
  * DATE     : Jun. 2024

"""

import os

from .odbind import *
__path__ = [os.path.join(os.path.dirname(__file__), 'odbind')]
