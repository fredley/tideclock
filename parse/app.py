from flask import Flask

from tideparse import LOCATIONS, fetch

app = Flask(__name__)
valid_locations = {loc['path']: key for key, loc in LOCATIONS.items()}


@app.route("/<path:location>")
def get_tide(location):
    location = location.split('/')[0]
    if location not in valid_locations.keys():
        return "Invalid location", 400
    return fetch(valid_locations[location])
