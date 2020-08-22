import boto3
import json
from urllib import request
import datetime
from dateutil.parser import parse

STEPS = 1024
# Solva, Pembrokeshire
BBC_URL = 'https://www.bbc.co.uk/weather/coast-and-sea/tide-tables/11/492a/'
BUCKET_NAME = "tidecache"
CACHE_KEY = "cache.json"

# A python lambda that scrapes the BBC for tide times and water heights
# Returns the number of steps around the 'clockface' directly


def lambda_handler(event, context):
    now = datetime.datetime.now().replace(tzinfo=datetime.timezone.utc)

    s3 = boto3.resource('s3')
    object = s3.Object(BUCKET_NAME, CACHE_KEY)

    try:
        expired = object.last_modified < now - datetime.timedelta(days=2)
    except:
        expired = True

    if expired:
        data = fetch_data()
        object.put(Body=data.encode('utf-8'))
    else:
        data = object.get()['Body'].read().decode('utf-8')

    data = json.loads(data)

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

    return {
        'isBase64Encoded': False,
        'headers': {},
        'multiValueHeaders': {},
        'statusCode': 200,
        'body': str(water_height_to_steps(height_now, height_min, height_max, rising))
    }


def water_height_to_steps(height_now, height_min, height_max, rising):
    position = (height_now - height_min) / (height_max - height_min)
    if rising:
        return round(STEPS + STEPS * position)
    return round(STEPS - STEPS * position)


def fetch_data():
    with request.urlopen(BBC_URL) as response:
        html = response.read()

    # Hacky, but avoids BS
    return html.decode('utf8').split('data-data-id="tides">')[1].split("</script>")[0]
