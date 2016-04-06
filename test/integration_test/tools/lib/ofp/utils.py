import os
import sys
import re
import base64


def create_section_name(name, del_str):
    name = name.replace(del_str, "")
    name = re.sub("(.)([A-Z][a-z]+)", r"\1_\2", name)
    name = re.sub("([a-z0-9])([A-Z])", r"\1_\2", name).lower()

    return name


def get_attrs_without_len(obj):
    return [k for k, v in obj.stringify_attrs_ori()
            if k not in ["len", "length", "actions_len"]]


def create_byte_data(data):
    return base64.b16decode(data.replace(" ", "").upper())
