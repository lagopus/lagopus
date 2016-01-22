# global
HOST = "127.0.0.1"
BUFSIZE = 4096
SH_DIR = "shell"
SUFFIXES = [".yml", ".yaml"]

# result string
RET_OK = "OK"
RET_ERROR = "ERROR"

# result file
RESULT_FILE = "result.txt"

LOG_FILE = "ds_test.log"
LAGOPUS_LOG = "lagopus.log"
RYU_LOG = "ryu.log"

# option
# default option values.
OPT_CONFIG_FILE = "./conf/ds_test.ini"
OPT_OUT_DIR = "./out"
OPT_LOG_LEVEL = "info"

# config items
# conf names.
CONF_SEC_SYS = "system"
CONF_SEC_SYS_DS_PORT = "ds_port"

CONF_SEC_LAGOPUS = "lagopus"
CONF_SEC_LAGOPUS_PATH = "lagopus_path"
CONF_SEC_LAGOPUS_CONF = "lagopus_conf"
CONF_SEC_LAGOPUS_OPTS = "lagopus_opts"

CONF_SEC_RYU = "ryu"
CONF_SEC_RYU_PATH = "ryu_path"
CONF_SEC_RYU_OPTS = "ryu_opts"
CONF_SEC_RYU_APP = "ryu_app"

CONF_SEC_TLS = "tls"
CONF_SEC_TLS_IS_ENABLE = "is_enable"
CONF_SEC_TLS_CERTFILE = "certfile"
CONF_SEC_TLS_KEYFILE = "keyfile"
CONF_SEC_TLS_CA_CERTS = "ca_certs"

# test scenario items
SCE_TESTCASES = "testcases"
SCE_TESTCASE = "testcase"
SCE_USE = "use"
SCE_MODE = "mode"
SCE_SETUP = "setup"
SCE_TEARDOWN = "teardown"
SCE_TEST = "test"
SCE_CMDS = "cmds"
SCE_CMD_TYPE = "cmd_type"
SCE_CMD = "cmd"
SCE_RESULT = "result"
SCE_REPETITION_COUNT = "repetition_count"
SCE_REPETITION_TIME = "repetition_time"
SCE_TIMEOUT = "timeout"

SCE_CMD_TYPE_DS = "ds"
SCE_CMD_TYPE_SH = "shell"

SCE_USE_LAGOPUS = "lagopus"
SCE_USE_RYU = "ryu"

SCE_REPETITION_COUNT_DEFAULT = 1
SCE_TIMEOUT_DEFAULT = 60

SCE_MODE_SYNC = "sync"
SCE_MODE_ASYNC = "async"
