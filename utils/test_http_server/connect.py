from ipaddress import ip_address
from datetime import datetime
import socketserver  # import socketserver preinstalled module
import http.server
import argparse
import time
import json

HTTP_OK = "HTTP/1.0 200 OK\r\n"

#       IF YOU ADD SOME TEST:
#           add path to the test to TEST_PATHS list below
#           check example.txt if your json test is compatible with this test sctipt


# paths to json test files
TEST_PATHS = ["tests/lowlvl_gcode.json", 'tests/lowlvl_gcodes.json']

time_start = 0.00
next_delay = 5.00       # we can set a wait time before executing another test

test_cnt = 1            # test count
test_curr = 0           # current test
json_telemetry = {}     # telemetry json object loaded at the beggining of the test
json_obj = {}           # json object variable (currently loaded json test object)

# generic creation of the response header
def create_header(header_obj):
    ret = HTTP_OK
    # appends whatever header we currently want
    if 'token' is in header_obj:
        ret.append("Printer-Token: " + header_obj['token']'\r\n')
    if 'c-length' is in header_obj:
        ret.append("Content-Length: " + header_obj['c-length']'\r\n')
    if 'c-type' is in header_obj:
        ret.append("Content-Type: " + header_obj['c-type']'\r\n')
    if 'c-id' is in header_obj:
        ret.append("Command-Id: " + header_obj['c-id']'\r\n')
    ret.append('\r\n')
    return ret

def create_high_lvl(body_obj):
    pass

# creates low level requests like plain gcodes
# 
def create_low_lvl(body_obj):
    repeat = 1
    ret = ""
    if 'command' is in body_obj:
        if 'repeat' is in body_obj:
            repeat = body_obj['repeat']
        for x in range(0, repeat):
            ret.append(body_obj['command'])
            if x + 1 < repeat:
                ret.append('\n')

    if 'commands' is in body_obj:
        size = body_obj['commands'].size
        for x in range(0, size):
            ret.append(body_obj['commands'][x])
            if x + 1 < size:
                ret.append('\n')
    return ret

# generic creation of the test request
def create_request(json_obj):
    header_obj = json_obj['test']['request']['header']
    ret = create_header(header_obj)

    body_obj = json_obj['test']['request']['body']
    cmd_type = body_obj['type']

    # low level gcodes
    if 'low' is in cmd_type:
        ret.append(create_low_lvl(body_obj))
    # high level gcodes
    #elif 'high' is in cmd_type:
    #    ret.append(create_high_lvl(body_obj))
    else:
        pass
    
    return ret       

# loads test from json file
def test_load():
    global json_obj, test_curr, next_delay, test_name

    file_json = open(TEST_PATHS[test_curr], "r")
    json_obj = json.load(file_json)

    next_delay = 0.00 
    if 'delay' in json_obj['test']:
        next_delay = json_obj['test']['delay']

    ret_str = create_request(json_obj)

    if (test_curr + 1) >= test_cnt:
        test_curr = 0
    else:
        test_curr += 1

    return ret_str

# telemetry is tested in every cycle
# in case of failiure, error is logged in error output file "connect_tests_resutls.txt"
def test_telemetry(data):
    global json_telemetry
    for item in json_telemetry["result"]["header"]:
        if str(item) not in data:
            test_failed(data, "Telemetry")
            return
    for item in json_telemetry["result"]["body"]:
        if str(item) not in data:
            test_failed(data, "Telemetry")
            return

# if test fails it logs the info in error output file "connect_tests_results.txt"
def test_failed(data, name):
    now = datetime.now()
    file_results = open("connect_tests_results.txt", "a")
    if not file_results.mode == "a":
        print("Faild to write to \"connect_tests_res.txt\"")
    else:
        file_results.write(str(now) + " Wrong response data of test: " + name + "\n" + data + "\n\n")
        file_results.close()

# test the response from printer
def test_printers_response(data_str):
    global json_obj
    json_response = 0

    test_name = "Unknown"
    if 'name' in json_obj['test']:
        test_name = json_obj['test']['name']

    for item in json_obj['test']['result']['header']:
        if str(item) not in data_str:
            # test fail -> log info
            test_failed(data_str, test_name)
            return
        
    if 'body' in json_obj['test']:
        if json_response is 1:
    else:
        # only heafer response: success
        return

    # look if body is json structure
    if 'application/json' in data_str:
        json_response = 1            
        t = data_str.find('\r\n\r\n')
        if t is not -1:
            json_body = data_str[t + 4:]
            if len(json_body) < 1 or json_body[0] != '{' or json_body[-1] != '}':
                test_failed(data_str, test_name)
                # test failed: not proper json structure in the body
                return
            #json_obj = 


    
             
    

class TestTCPHandler(socketserver.BaseRequestHandler):
    def handle(self):
        # global vars
        global test_curr, next_delay, time_start
        # self.request is the TCP socket connected to the client
        self.data = self.request.recv(1024).strip()
        data_str = str(self.data)
        # some tests starts as a response to 
        if "/p/telemetry" in data_str:
            # testing telemetry in every cycle
            test_telemetry(data_str)

            time_point = time.perf_counter()
            # gcode tests require some time to complete
            # next_delay can be set in json file
            if time_point - time_start >= next_delay:
                # passes next test test's request
                ret_data = test_load()
                time_start = time_point
            else:
                ret_data = HTTP_OK + "\r\n"

            self.request.sendall(ret_data.encode('utf-8'))
        else:
            test_printers_response(data_str)

def main():
    
    global tests, test_cnt, test_desc, test_keywords, json_telemetry
    parser = argparse.ArgumentParser(description='starts http server for test')
    parser.add_argument('ip_address',
                    metavar="host_ip",
                    type=ip_address,
                    help='host ip address')
    args = parser.parse_args()

    time_start = time.perf_counter()

    # loads telemetry file
    file_telemetry = open('tests/telemetry.json', "r")
    json_telemetry = json.load(file_telemetry)

    # creates or overwrites error output txt file
    f2 = open("tests/connect_tests_results.txt", "w+")
    if not f2.mode == "w+":
        print("Failed to create \"connect_tests_res.txt\"")
    f2.write("")
    f2.close()

    HOST = args.ip_address
    PORT = 80

    print('IP address of server connected:' + str(HOST))
    httpd = socketserver.TCPServer((str(HOST), PORT), TestTCPHandler)

    httpd.serve_forever()

main()
