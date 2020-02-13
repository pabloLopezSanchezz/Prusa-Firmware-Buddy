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
    def __init__(self, timestamp, metric_name, value, tags):
        self.timestamp = timestamp
        self.metric_name = metric_name
        self.value = value
        self.tags = tags

    @property
    def is_error(self):
        return isinstance(self.value, MetricError)


class LineProtocolParser:
    def __init__(self):
        self.reset_session()

    def timestamp_diff_to_timestamp(self, timestamp_diff):
        if self.last_timestamp is None:
            self.last_timestamp = datetime.utcnow()
        else:
            self.last_timestamp += timedelta(milliseconds=timestamp_diff)
        return self.last_timestamp

    def reset_session(self):
        self.last_timestamp = None

    def parse_value(self, value, flag, tags=None):
        tags = tags or dict()

        if '&' in value:
            value, *tags_raw = value.split('&')
            tags = {t.split('=')[0]: t.split('=')[1] for t in tags_raw}
            print(value, tags_raw, tags)
        else:
            value, tags = value, (tags or dict())

        if flag == 's' and value.startswith('!'):
            return self.parse_value(value[2:], flag=value[1], tags=tags)
        elif flag == 'i':
            return int(value), tags
        elif flag == 'f':
            return float(value), tags
        elif flag == 's':
            return str(value), tags
        elif flag == 'e':
            return MetricError(str(value)), tags
        else:
            print('invalid value:', value)
            return None, None

    def parse_line(self, line):
        match = line_re.fullmatch(line)
        if not match:
            return
        timestamp_diff, metric_name, flag, value = match.group(1), match.group(
            2), match.group(3), match.group(4)
        value, tags = self.parse_value(value, flag)
        timestamp = self.timestamp_diff_to_timestamp(int(timestamp_diff))
        return Point(timestamp, metric_name, value, tags=tags)


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
        points = []
        last_sent = asyncio.get_event_loop().time()
        while True:
            try:
                line = (await self.line_queue.get()).strip()
                point = self.parser.parse_line(line)

                if point and not point.is_error:
                    influx_point = {
                        'time': point.timestamp,
                        'measurement': point.metric_name,
                        'tags': dict(**self.tags, **point.tags),
                        'fields': {
                            'value': point.value
                        },
                    }
                    points.append(influx_point)
                elif point and point.is_error:
                    print('error:', point)

                if asyncio.get_event_loop().time() - last_sent > 1:
                    print('writing', len(points), 'points')
                    await self.influx.write(points)
                    points = []
                    last_sent = asyncio.get_event_loop().time()
            except Exception as e:
                print('error: %s' % e)
                points = []
                last_sent = asyncio.get_event_loop().time()

    async def readline_task(self):
        while True:
            try:
                line = (await self.serial.readline_async()).decode('ascii')
                await self.line_queue.put(line)
            except UnicodeDecodeError:
                pass

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
