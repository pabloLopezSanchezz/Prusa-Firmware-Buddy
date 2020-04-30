import time
from datetime import datetime
import json
from ipaddress import ip_address
import logging
import sys

HTTP_OK = "HTTP/1.0 200 OK\r\n"

time_start = 0.00
next_delay = 5.00       # we can set a wait time before executing another test

test_cnt = 0            # test count
test_curr = 0           # current test
json_obj = {}           # json object variable (currently loaded json test object)
json_test = {}          # current json test
off_test_cnt = 0        # number of swichted off tests

def generate_response():
    global time_start, next_delay
    time_point = time.perf_counter()
    ret_data = ""
    # gcode tests require some time to complete
    # next_delay can be set in json file
    if time_point - time_start >= next_delay:
        # passes next test test's request
        test_str = test_load()
        if len(test_str) == 0:
            ret_data = HTTP_OK + "\r\n"
        else:
            ret_data = test_str
        time_start = time_point
    else:
        ret_data = HTTP_OK + "\r\n"
    return ret_data

def tests_init():
    global time_start, json_obj, test_cnt

    time_start = time.perf_counter()
    # loads telemetry file
    file_json = open('connect_tests/tests.json', "r")
    json_whole_obj = json.load(file_json)
    file_json.close()

    # set json pointers
    json_obj = json_whole_obj['tests']
    test_cnt = len(json_obj)

    # logging
    logging.basicConfig(filename='connect_tests/errors.log', filemode='w', level=logging.ERROR)

def find_json_structure(data_str):
    t = data_str.find('{')
    if t is -1:
        return {}

    json_body_str = data_str[t:]
    if len(json_body_str) < 2 or json_body_str[0] != '{' or json_body_str[-1] != '}':
        return {}
    json_body_dic = json.loads(json_body_str)
    return json_body_dic

# generic creation of the response header
def create_header(header_obj):
    ret = []
    ret.append(HTTP_OK)
    # appends whatever header we currently want
    if 'token' in header_obj:
        ret.append("Printer-Token: " + str(header_obj['token']) + '\r\n')
    if 'c-length' in header_obj:
        ret.append("Content-Length: " + str(header_obj['c-length']) + '\r\n')
    if 'c-type' in header_obj:
        ret.append("Content-Type: " + header_obj['c-type'] + '\r\n')
    if 'c-id' in header_obj:
        ret.append("Command-Id: " + str(header_obj['c-id']) + '\r\n')
    ret.append('\r\n')
    ret_fin = ''.join(ret)
    return ret_fin

def create_high_lvl(body_obj):
    pass

# creates low level requests like plain gcodes
def create_low_lvl(body_obj):
    repeat = 1
    ret = []
    if 'command' in body_obj:
        if 'repeat' in body_obj:
            repeat = body_obj['repeat']
        for x in range(0, repeat):
            ret.append(body_obj['command'])
            if x + 1 < repeat:
                ret.append('\n')

    elif 'commands' in body_obj:
        size = len(body_obj['commands'])
        for x in range(0, size):
            ret.append(body_obj['commands'][x])
            if x + 1 < size:
                ret.append('\n')
    ret_fin = ''.join(ret)
    return ret_fin

# generic creation of the test request
def create_request(test):
    header_obj = test['request']['header']
    ret_list = []
    ret_list.append(create_header(header_obj))

    body_obj = test['request']['body']
    cmd_type = body_obj['type']

    # low level gcodes
    if 'low' in cmd_type:
        ret_list.append(create_low_lvl(body_obj))
    # high level gcodes
    # elif 'high' is in cmd_type:
    #    ret.append(create_high_lvl(body_obj))
    else:
        pass
    
    ret = ''.join(ret_list)
    return ret    

# loads test from json file
def test_load():
    global json_obj, json_test, next_delay, test_curr, off_test_cnt, test_cnt

    next_delay = 0.00

    if off_test_cnt == test_cnt:
        return ""

    loaded = 0
    while not loaded:
        if 'switch' in json_obj[test_curr]:
            if 'off' in json_obj[test_curr]['switch']:
                off_test_cnt += 1
            else:
                json_test = json_obj[test_curr]
                loaded = 1
        else:
            json_test = json_obj[test_curr]
            loaded = 1

        if off_test_cnt == test_cnt:
            print("All tests switched off")
            return ""
        
        if (test_curr + 1) >= test_cnt :
            test_curr = 0
            off_test_cnt = 0
        else:
            test_curr += 1
 
    if 'delay' in json_test:
        next_delay = json_test['delay']

    ret_str = create_request(json_test)

    return ret_str

# telemetry is tested in every cycle
# in case of failiure, error is logged in error output file "connect_tests_resutls.txt"
def test_telemetry(data):
    telemetry_keywords = ["POST", "/p/telemetry", "HTTP/1.0", "01234567899876543210", "application/json"]
    for item in telemetry_keywords:
        if item not in data:
            test_failed(data, "Telemetry")
            return

    response_dic = find_json_structure(data)
    if len(response_dic) == 0:
        test_failed(data, "Telemetry")
        return

    if not isinstance(response_dic['temp_nozzle'], int):
        test_failed(data, "Telemetry")
        return
    if not isinstance(response_dic['temp_bed'], int):
        test_failed(data, "Telemetry")
        return
    if not isinstance(response_dic['material'], str):
        test_failed(data, "Telemetry")
        return
    if not isinstance(response_dic['pos_z_mm'], float):
        test_failed(data, "Telemetry")
        return
    if not isinstance(response_dic['printing_speed'], int):
        test_failed(data, "Telemetry")
        return
    if not isinstance(response_dic['flow_factor'], int):
        test_failed(data, "Telemetry")
        return

# if test fails it logs the info in error output file "connect_tests_results.txt"
def test_failed(data, name):
    now = datetime.now()
    logging.error(str(now) + " :: Test " + name + " failed:\n" + data + "\n")

# testing json structure decoded from printer's response
#   res_body = decoded json structure in dictionary
def test_json_body(res_body):
    # loaded json test structure
    global json_test
    test_body = json_test['result']['body']
    if 'event' in test_body:
        if not isinstance(res_body['event'], str) or test_body['event'] not in res_body['event']:
            return 1
    if 'command_id' in test_body:
        if not isinstance(res_body['command_id'], int) or test_body['commmand_id'] not in res_body['command_id']:
            return 1
    
    # ADD ANOTHER TEST
    
    return 0

# test the response from printer
def test_printers_response(data_str):
    global json_test
    json_response = 0

    test_name = "Unknown"
    if 'name' in json_test:
        test_name = json_test['name']

    for item in json_test['result']['header']:
        if str(item) not in data_str:
            # test fail: keyword not found -> log info
            test_failed(data_str, test_name)
            return

    if 'body' not in json_test:
        # test success: only header response expected
        return

    # look if body is json structure
    if 'application/json' in data_str:      
            json_body_dic = find_json_structure(data_str)
            # test decoded json structure -> log info
            if len(json_body_dic) == 0 or test_json_body(json_body_dic):
                # test fail: Something wrong with json structure -> log info
                test_failed(data_str, test_name)
                return
    else:
        # no plain text body responses yet
        # TODO
        return