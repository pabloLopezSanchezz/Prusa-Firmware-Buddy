import socketserver  # import socketserver preinstalled module
import http.server
import argparse
from ipaddress import ip_address

#HOST = '192.168.1.111'
PORT = 8000
HTTP_OK = "HTTP/1.1 200 OK\r\n\r\n"
HTTP_LOWLVL_GCODE = "HTTP/1.1 200 OK\r\nContent-Type: text/x.gcode\r\nContent-Length: 6\r\nCommand-Id: 1254\r\n\r\nG28 XY"
HTTP_RESPONSE_ACC = "POST /p/events HTTP/1.0\r\nPrinter-Token: 01234567899876543210\r\nContent-Type: application/json\r\n\r\n{\"event\":\"ACCEPTED\",\"command_id\":1254}"
HTTP_LOWLVL_GCODES = "HTTP/1.1 200 OK\r\nContent-Type: text/x.gcode\r\nContent-Length: 23\r\nCommand-Id: 23\r\n\r\nG1 X160\nG1 X180\nG1 X170"
HTTP_BAD_GCODE = "HTTP/1.1 200 OK\r\nContent-Type: text/x.gcode\r\nContent-Length: 1\r\nCommand-Id: 404\r\n\r\nG"
TEST_COUNT = 3

telemetry_count = 0
success_count = 0
fail_count = 0

test_phase_info = ["Single valid lowlvl gcode request with ACCEPTED event", "Multi valid lowlvl gcode request with ACCEPTED event", "Bad lowlvl gcode request with REJECTED event"]
test_response_events = ["ACCEPTED", "ACCEPTED", "REJECTED"]
test_response_check = ["\"command_id\":1254", "\"command_id\":23", "\"command_id\":404"]
test_phase = 0
test_repetions = 10


parser = argparse.ArgumentParser(description='starts http server for test')
parser.add_argument('ip_address',
                    metavar="host_ip",
                    type=ip_address,
                    help='host ip address')
args = parser.parse_args()

def evaluate():
    global test_phase, success_count, telemetry_count, fail_count, test_repetions
    print("PHASE " + str(test_phase + 1) + " COMPLETED")
    print("Phase description: " + test_phase_info[test_phase])
    print("Successful tests: " + str(success_count) + "/" + str(telemetry_count))
    print("Failed tests: " + str(fail_count) + "/" + str(telemetry_count))
    fail_count = success_count = 0
    test_phase += 1
    test_repetions = 10
    telemetry_count = 0

class TestTCPHandler(socketserver.BaseRequestHandler):
    def handle(self):
        # set global variables
        global telemetry_count, success_count, fail_count
        global test_phase, test_repetions
        ret_data = ""
        # self.request is the TCP socket connected to the client
        self.data = self.request.recv(1024).strip()
        if "POST /p/telemetry" in str(self.data):
            telemetry_count += 1
            if test_phase is 0:
                ret_data = HTTP_LOWLVL_GCODE
            elif test_phase is 1:
                ret_data = HTTP_LOWLVL_GCODES
            elif test_phase is 2:
                ret_data = HTTP_BAD_GCODE
            if test_phase < TEST_COUNT:
                self.request.sendall(ret_data.encode('utf-8'))
            test_repetions -= 1
        elif test_response_events[test_phase] in str(self.data) and test_response_check[test_phase] in str(self.data):
            success_count += 1
            if test_repetions is 0:
                evaluate()
        else:
            print("Unkown message: " + str(self.data))
            fail_count += 1
            if test_repetions is 0:
                evaluate()


HOST = args.ip_address
print('IP address of server connected:' + str(HOST))
# standard http server
#httpd = socketserver.TCPServer((str(HOST), PORT), http.server.SimpleHTTPRequestHandler)
#custom server
httpd = socketserver.TCPServer((str(HOST), PORT), TestTCPHandler)
httpd.serve_forever()
