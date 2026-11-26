import pytest
from odbind.survey import Survey

@pytest.fixture
def datadir(request):
    return request.config.getoption('--datadir')

@pytest.fixture
def survey(request, datadir):
    return Survey(request.config.getoption('--survey'), datadir)
