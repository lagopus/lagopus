import os

# global
BUFSIZE = 4096

TEST_HOME = os.path.dirname((os.path.abspath(os.path.dirname(__file__))))
SHELL_HOME = os.path.join(TEST_HOME, "shell")
LIB_HOME = os.path.join(TEST_HOME, "lib")

DEFAULT_TIMEOUT = 30
