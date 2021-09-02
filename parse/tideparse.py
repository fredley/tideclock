import json
from urllib import request
import json
import datetime
from dateutil.parser import parse
import os
import time


STEPS = 1024

LOCATIONS = {
    'SOLVA': {
        'url': 'https://www.bbc.co.uk/weather/coast-and-sea/tide-tables/11/492a/',
        'path': 'tide',
        'enabled': False,
        'parser': 'bbc',
    },
    'ITCHENOR': {
        'url': 'https://www.bbc.co.uk/weather/coast-and-sea/tide-tables/8/68c',
        'path': 'itchenor',
        'enabled': False,
        'parser': 'bbc',
    },
    'SEAMILLS': {
        'url': 'https://www.bbc.co.uk/weather/coast-and-sea/tide-tables/12/523b',
        'path': 'seamills',
        'enabled': False,
        'parser': 'bbc',
    },
    'FORT_DENISON': {
        'url': 'https://tides.willyweather.com.au/graphs/data.json?startDate={date}&graph=outlook:5,location:17797,series=order:0,id:sunrisesunset,type:forecast,series=order:1,id:tides,type:forecast',
        'path': 'fortdenison',
        'enabled': True,
        'parser': 'willy',
    }
}


def main():
    for location, info in LOCATIONS.items():
        if info['enabled']:
          fetch(location)

def fetch(location):
    cache_path = f'/tmp/tides/{location}.cache'
    os.makedirs(os.path.dirname(cache_path), exist_ok=True)

    try:
        modified_time = os.path.getmtime(cache_path)
        expired = modified_time < time.time() - (60 * 60 * 24 * 2)
    except FileNotFoundError:
        expired = True

    if expired:
        data = fetch_data(location)
        with open(cache_path, 'w') as f:
            f.write(data)
    else:
        with open(cache_path) as f:
            data = f.read()

    parser = LOCATIONS[location]['parser']
    if parser == 'bbc':
        angle = parse_bbc(data)
    else:
        angle = parse_willy(data)

    return angle


def parse_bbc(data):
    data = json.loads(data.split('data-data-id="tides">')[1].split("</script>")[0])

    next_tide = None
    now = datetime.datetime.now().replace(tzinfo=datetime.timezone.utc)

    heights = []
    source_tide = None

    for tide in data['tides']:
        for extreme in tide['extremes']:
            if parse(extreme['timestamp']) > now:
                next_tide = extreme
                if source_tide:
                    heights = source_tide['heightAboveChartDatum']
                heights += tide['heightAboveChartDatum']
                break
            prev_tide = extreme
        source_tide = tide
        if next_tide:
            break

    height_max = max(prev_tide['mmAboveChartDatum'], next_tide['mmAboveChartDatum'])
    height_min = min(prev_tide['mmAboveChartDatum'], next_tide['mmAboveChartDatum'])
    rising = prev_tide['type'] == "Low"


    for d in heights:
        if parse(d['time']) > now:
            break
        height_now = d['mm']

    return str(water_height(height_now, height_min, height_max, rising))


def parse_willy(raw):
    data = json.loads(raw)
    ds = []
    for group in data['data']['forecastGraphs']['tides']['dataConfig']['series']['groups']:
        ds += group['points']
    ds = sorted(ds, key=lambda d: d['x'])
    time_now = int(time.time())
    offset = data['data']['location']['timeZoneOffset']

    prev = None
    curr = None
    last_inflection = None
    next_inflection = None

    for d in ds:
        if d['x'] >= time_now + offset:
            curr = d
        if d['description']:
            if curr:
                next_inflection = d
            else:
                last_inflection = d
        if curr and next_inflection:
            break
        if not curr:
            prev = d

    slope = (curr['y'] - prev['y']) / (curr['x'] - prev['x'])
    height_min = min(last_inflection['y'], next_inflection['y'])
    height_max = max(last_inflection['y'], next_inflection['y'])
    rising = last_inflection['description'] == 'low'
    height_now = prev['y'] + slope * (time_now + offset - prev['x'])

    return str(water_height(height_now, height_min, height_max, rising))


def water_height(height_now, height_min, height_max, rising):
    # print(f"min: {height_min}, max: {height_max}, now: {height_now}, {'rising' if rising else 'falling'}")
    position = (height_now - height_min) / (height_max - height_min)
    if rising:
        return round(STEPS + STEPS * position) % (2 * STEPS)
    return round(STEPS - STEPS * position) % (2 * STEPS)


def fetch_data(location):
    dct = LOCATIONS[location]
    if dct['parser'] == 'willy':
        url = dct['url'].format(date=(datetime.date.today() - datetime.timedelta(days=1)).isoformat())
    else:
        url = dct['url']
    with request.urlopen(url) as response:
        html = response.read()

    return html.decode('utf8')


if __name__ == '__main__':
    print(main())
