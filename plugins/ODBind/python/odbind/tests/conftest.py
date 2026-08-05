import pytest
from odbind.survey import Survey

def pytest_addoption(parser):
    parser.addoption(
        '--survey', action='store', default='F3_Demo_2020', help='OpendTect survey to use for running tests'
    )

@pytest.fixture
def survey(request):
    return Survey(request.config.getoption('--survey'))
