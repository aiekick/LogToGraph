import signal_handler
import re

def parse_line(line):
    pattern = r"(.*)\[(.*)\]\s+(\S*)\s+(\S*)\s+(.*)::(.*):(.*)"
    match = re.match(pattern, line)
    if match:
        _test_name = match.group(1)
        _time_stamp = match.group(2)
        _test1 = match.group(3)
        _test2 = match.group(4)
        _class = match.group(5)
        _func = match.group(6)
        _msg = match.group(7)
        print(f"test_name : {_test_name or 'Null'}")
        print(f"time_stamp : {_time_stamp or 'Null'}")
        print(f"test1 : {_test1 or 'Null'}")
        print(f"test2 : {_test2 or 'Null'}")
        print(f"class : {_class or 'Null'}")
        print(f"func : {_func or 'Null'}")
        print(f"msg : {_msg or 'Null'}")
        signal_handler.addSignalValue(1)
