import os
import sys
import re
from types import *


def create_section_name(name, del_str):
    name = name.replace(del_str, "")
    name = re.sub("(.)([A-Z][a-z]+)", r"\1_\2", name)
    name = re.sub("([a-z0-9])([A-Z])", r"\1_\2", name).lower()

    return name


def get_attrs_without_len(obj):
    return [k for k, v in obj.stringify_attrs_ori()
            if k != "len" and k != "length"]
