import socket
import re
from datetime import datetime, timedelta
import aioserial
import asyncio
import click
import aioinflux

line_re = re.compile(r'\[([^:]*):([^:]*):([^:]*):(.*)]')


class MetricError(Exception):
    def __init__(self, message):
        self.message = message
        super().__init__(message)


class Point:
    def __init__(self, timestamp, metric_name, value):
        self.timestamp = timestamp
        self.metric_name = metric_name
        self.value = value

    @property
    def is_error(self):
        return isinstance(self.value, MetricError)


class LineProtocolParser:
    def __init__(self):
        self.last_timestamp = None

    def timestamp_diff_to_timestamp(self, timestamp_diff):
        if self.last_timestamp is None:
            self.last_timestamp = datetime.utcnow()
        else:
            self.last_timestamp += timedelta(milliseconds=timestamp_diff)
        return self.last_timestamp

    def parse_line(self, line):
        match = line_re.fullmatch(line)
        if not match:
            return
        timestamp_diff, metric_name, flag, value = match.group(1), match.group(
            2), match.group(3), match.group(4)
        if flag == 'i':
            value = int(value)
        elif flag == 'f':
            value = float(value)
        elif flag == 's':
            value = str(value)
        elif flag == 'e':
            value = ''
        elif flag == 'e':
            value = MetricError(str(value))
        else:
            print('invalid line:', line)
            return
        timestamp = self.timestamp_diff_to_timestamp(int(timestamp_diff))
        return Point(timestamp, metric_name, value)


class Application:
    def __init__(self, serial, influx, tags=None):
        self.serial: aioserial.AioSerial = serial
        self.influx: aioinflux.InfluxDBClient = influx
        self.line_queue = asyncio.Queue()
        self.parser = LineProtocolParser()
        self.tags = tags or self.get_session_tags()

    def get_session_tags(self):
        return {
            'hostname': socket.gethostname(),
            'port': self.serial.port,
            'session_start_time': datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        }

    async def process_task(self):
        while True:
            try:
                line = (await self.line_queue.get()).strip()
                point = self.parser.parse_line(line)
                if point and not point.is_error:
                    influx_point = {
                        'time': point.timestamp,
                        'measurement': point.metric_name,
                        'tags': self.tags,
                        'fields': {
                            'value': point.value
                        },
                    }
                    await self.influx.write(influx_point)
                elif point and point.is_error:
                    print('error:', point)
            except Exception as e:
                print('error: %s' % e)

    async def readline_task(self):
        while True:
            line = (await self.serial.readline_async()).decode('ascii')
            await self.line_queue.put(line)

    async def run(self):
        await asyncio.gather(self.process_task(), self.readline_task())


async def main(port, database):
    serial = aioserial.AioSerial(port, baudrate=115200)
    async with aioinflux.InfluxDBClient(db=database) as influx:
        await influx.create_database(db=database)
        app = Application(serial, influx)
        await app.run()


@click.command()
@click.argument('port')
@click.option('--database', default='buddy')
def cmd(port, database):
    asyncio.run(main(port, database))


if __name__ == '__main__':
    cmd()
