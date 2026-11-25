def pytest_addoption(parser):
    parser.addoption(
        '--datadir', action='store', default=None,
        help='OpendTect data root, passed as --datadir=PATH. '
             'Default: the data root from the user settings'
    )
    parser.addoption(
        '--survey', action='store', default='F3_Demo_2023',
        help='OpendTect survey to use for running tests'
    )
    parser.addoption(
        '--config', action='store', choices=['Release', 'Debug'],
        help='OpendTect build configuration to load. '
             'Default: Release if that library exists, otherwise Debug'
    )
